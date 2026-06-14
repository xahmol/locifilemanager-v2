// recurse.c - Iterative recursive directory walker for locifilemanager v2
// See recurse.h for the public API and stack-safety rationale.

#include <string.h>
#include "loci.h"
#include "dir.h"
#include "recurse.h"

// Per-depth walk state. Each frame holds its own LociDir handle (loci_opendir
// returns a pointer to a single shared static struct, so it must be copied
// out by value) so ancestor directories stay open while a subdirectory is
// being walked.
typedef struct
{
    LociDir dirhandle;
    uint8_t pathlen;
} RecurseFrame;

static RecurseFrame stack[RECURSE_MAX_DEPTH];
static char walkpath[256];

uint8_t recurse_truncated;

/**
 * Close every open directory handle from stack[0] up to and including
 * stack[depth]. Used to unwind cleanly when a callback returns
 * RECURSE_ABORT partway through a walk.
 *
 * @param depth Deepest stack index (inclusive) whose handle must be closed.
 * @return      (none)
 */
static void recurse_close_all(uint8_t depth)
{
    int8_t d;
    for (d = (int8_t)depth; d >= 0; d--)
        loci_closedir(&stack[d].dirhandle);
}

/**
 * Iteratively walk the contents of rootpath depth-first, invoking cb for
 * each file and directory encountered. rootpath itself is not reported.
 * Directories are reported twice: RECURSE_ENTER_DIR before descending and
 * RECURSE_LEAVE_DIR after all of its entries (and subdirectories) have been
 * processed. If RECURSE_MAX_DEPTH is reached, a subdirectory is reported as
 * already left (without descending) and recurse_truncated is set to 1.
 *
 * @param rootpath Directory to walk the contents of (drive-prefixed path).
 * @param cb       Callback invoked for each ENTER_DIR/FILE/LEAVE_DIR event;
 *                 returning RECURSE_ABORT stops the walk early.
 * @param userdata Opaque pointer passed through unchanged to every cb call.
 * @return         RECURSE_CONTINUE (0) on completion, RECURSE_ABORT (1) if
 *                  cb requested an early stop, or -1 if rootpath could not
 *                  be opened.
 */
int8_t recurse_walk(const char *rootpath, RecurseCallback cb, void *userdata)
{
    LociDir *opened;
    uint8_t  depth;

    recurse_truncated = 0;

    strncpy(walkpath, rootpath, sizeof(walkpath) - 1);
    walkpath[sizeof(walkpath) - 1] = '\0';

    opened = loci_opendir(walkpath);
    if (!opened) return -1;
    stack[0].dirhandle = *opened;
    stack[0].pathlen   = (uint8_t)strlen(walkpath);
    depth = 0;

    for (;;)
    {
        LociDirent *entry = loci_readdir(&stack[depth].dirhandle);

        if (!entry || entry->d_name[0] == 0)
        {
            loci_closedir(&stack[depth].dirhandle);
            walkpath[stack[depth].pathlen] = '\0';

            if (depth == 0) return RECURSE_CONTINUE;

            depth--;
            if (cb(RECURSE_LEAVE_DIR, nullptr, walkpath, userdata) == RECURSE_ABORT)
            {
                recurse_close_all(depth);
                return RECURSE_ABORT;
            }
            walkpath[stack[depth].pathlen] = '\0';
            continue;
        }

        if (entry->d_attrib & DIR_ATTR_DIR)
        {
            dir_build_path(walkpath, sizeof(walkpath), walkpath, entry->d_name);

            // loci_readdir() returns directory names without a trailing
            // slash, but every path elsewhere in this app (presentdir[].path,
            // and the pathlen recorded below for truncate-on-pop) follows the
            // "directories end in /" convention -- append it so children
            // built via dir_build_path(walkpath, ..., walkpath, childname)
            // get the separator.
            if (strlen(walkpath) < sizeof(walkpath) - 1)
                strcat(walkpath, "/");

            if (cb(RECURSE_ENTER_DIR, entry, walkpath, userdata) == RECURSE_ABORT)
            {
                recurse_close_all(depth);
                return RECURSE_ABORT;
            }

            opened = (depth + 1 < RECURSE_MAX_DEPTH) ? loci_opendir(walkpath) : nullptr;

            if (opened)
            {
                depth++;
                stack[depth].dirhandle = *opened;
                stack[depth].pathlen   = (uint8_t)strlen(walkpath);
            }
            else
            {
                // Depth limit reached, or subdirectory could not be opened:
                // report it as already left, without descending.
                recurse_truncated = 1;
                if (cb(RECURSE_LEAVE_DIR, nullptr, walkpath, userdata) == RECURSE_ABORT)
                {
                    recurse_close_all(depth);
                    return RECURSE_ABORT;
                }
                walkpath[stack[depth].pathlen] = '\0';
            }
        }
        else
        {
            dir_build_path(walkpath, sizeof(walkpath), walkpath, entry->d_name);

            if (cb(RECURSE_FILE, entry, walkpath, userdata) == RECURSE_ABORT)
            {
                recurse_close_all(depth);
                return RECURSE_ABORT;
            }
            walkpath[stack[depth].pathlen] = '\0';
        }
    }
}
