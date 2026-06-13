// file.c - File operations (copy/move/delete/rename/tape browse) for
// locifilemanager v2.
// Based on v1 locifilemanager file.c by Xander Mol, 2025 — local reference at
// locifilemanager/src/file.c. See file.h for adaptation notes.

#include <string.h>
#include "oric.h"
#include "charwin.h"
#include "loci.h"
#include "menu.h"
#include "dir.h"
#include "drive.h"
#include "recurse.h"
#include "strings.h"
#include "file.h"

// Shared state for a recursive directory copy/move, passed through
// recurse_walk()'s userdata to file_copy_move_cb().
typedef struct
{
    uint8_t      srcrootlen; // strlen() of the source root path (pathbuffer2)
    uint8_t      move;       // 0 = copy, 1 = move
    uint8_t      confirmed;  // "overwrite all" already confirmed this op
    OricCharWin *popup;      // progress popup (rows 5-7 reused for status)
} CopyMoveCtx;

// recurse_walk() callback for a recursive directory copy/move.
//
// pathbuffer  holds the destination root (dstroot), pathbuffer2 the source
// root (srcroot) for the whole walk -- both set up by the caller and left
// untouched here. fullpath (= recurse.c's walkpath) is always the SOURCE
// path of the current entry; the corresponding destination path is built
// into pathbuffer3 as dstroot + (fullpath without the srcroot prefix).
static int8_t file_copy_move_cb(RecurseEvent ev, const LociDirent *entry,
                                 const char *fullpath, void *userdata)
{
    CopyMoveCtx *ctx = (CopyMoveCtx *)userdata;
    const char  *relpath = fullpath + ctx->srcrootlen;
    LociDir     *d;

    if (ev == RECURSE_LEAVE_DIR)
    {
        // Subdirectory fully processed: if moving, its source copy is now
        // empty, so remove it. Non-fatal -- continue the walk either way.
        if (ctx->move)
            loci_unlink(fullpath);
        return RECURSE_CONTINUE;
    }

    // Check destination path length before building it
    if (strlen(pathbuffer) + strlen(relpath) >= sizeof(pathbuffer3))
    {
        cwin_fill_rect(ctx->popup, 0, 6, 36, 1, CH_SPACE);
        cwin_putat_string(ctx->popup, 0, 6, MSG_FILE_PATH_TOO_LONG);
        return RECURSE_CONTINUE;
    }
    dir_build_path(pathbuffer3, sizeof(pathbuffer3), pathbuffer, relpath);

    if (ev == RECURSE_ENTER_DIR)
    {
        // Create the matching destination subdirectory (merge semantics: if
        // it already exists, proceed without error).
        d = loci_opendir(pathbuffer3);
        if (d)
        {
            loci_closedir(d);
        }
        else
        {
            // loci_mkdir() wants the new dir name without a trailing '/',
            // matching dir_newdir()'s usage.
            uint8_t len = (uint8_t)strlen(pathbuffer3);
            if (len)
                pathbuffer3[len - 1] = '\0';

            if (loci_mkdir(pathbuffer3) < 0)
            {
                cwin_fill_rect(ctx->popup, 0, 6, 36, 1, CH_SPACE);
                cwin_putat_string(ctx->popup, 0, 6, MSG_FILE_MKDIR_FAILED);
                cwin_putat_string(ctx->popup, 0, 7, MSG_MENU_PRESSAKEY);
                cwin_getch();
                return RECURSE_ABORT;
            }
        }
        return RECURSE_CONTINUE;
    }

    // RECURSE_FILE
    cwin_fill_rect(ctx->popup, 0, 5, 36, 1, CH_SPACE);
    cwin_putat_string(ctx->popup, 0, 5, entry->d_name);

    // Check if destination file exists
    if (file_exists(pathbuffer3))
    {
        if (confirm || !ctx->confirmed)
        {
            if (!menu_confirm_file(MSG_FILE_OVERWRITE_Q, entry->d_name))
                return RECURSE_ABORT;
            ctx->confirmed = 1;
        }
        // Delete file if user selects overwrite
        if (loci_unlink(pathbuffer3) < 0)
        {
            menu_fileerrormessage();
            return RECURSE_ABORT;
        }
    }

    // Copy file with progress bar on row 14, columns 2..33
    switch (file_copy_progress(pathbuffer3, fullpath, 2, 14, 32))
    {
        case 0:
            break;
        case -2:
            cwin_fill_rect(ctx->popup, 0, 6, 36, 1, CH_SPACE);
            cwin_putat_string(ctx->popup, 0, 6, MSG_FILE_COPY_CANCELLED);
            return RECURSE_ABORT;
        default:
            menu_fileerrormessage();
            return RECURSE_ABORT;
    }

    // Delete source file if move is selected
    if (ctx->move)
    {
        if (loci_unlink(fullpath) < 0)
        {
            menu_fileerrormessage();
            return RECURSE_ABORT;
        }
    }

    // Check for ESC to cancel
    if (keyb_check() == KEY_ESC)
        return RECURSE_ABORT;

    return RECURSE_CONTINUE;
}

// Recursively copy or move the directory srcpath (in pathbuffer2) to dstpath
// (in pathbuffer). Returns RECURSE_ABORT if the operation stopped early
// (error, decline, ESC, or mkdir failure), RECURSE_CONTINUE otherwise.
// *confirmed carries the "overwrite all" state into and out of the walk.
static int8_t file_copy_move_dir(const char *srcpath, const char *dstpath, uint8_t move,
                                  OricCharWin *popup, uint8_t *confirmed)
{
    CopyMoveCtx ctx;
    LociDir    *d;
    int8_t      result;

    // Create the destination root directory if it doesn't already exist
    // (merge semantics: if it does, proceed without error).
    d = loci_opendir(dstpath);
    if (d)
    {
        loci_closedir(d);
    }
    else
    {
        uint8_t len;

        strncpy(pathbuffer3, dstpath, sizeof(pathbuffer3) - 1);
        pathbuffer3[sizeof(pathbuffer3) - 1] = '\0';
        len = (uint8_t)strlen(pathbuffer3);
        if (len)
            pathbuffer3[len - 1] = '\0';

        if (loci_mkdir(pathbuffer3) < 0)
        {
            cwin_fill_rect(popup, 0, 6, 36, 1, CH_SPACE);
            cwin_putat_string(popup, 0, 6, MSG_FILE_MKDIR_FAILED);
            cwin_putat_string(popup, 0, 7, MSG_MENU_PRESSAKEY);
            cwin_getch();
            return RECURSE_ABORT;
        }
    }

    ctx.srcrootlen = (uint8_t)strlen(srcpath);
    ctx.move       = move;
    ctx.confirmed  = *confirmed;
    ctx.popup      = popup;

    result = recurse_walk(srcpath, file_copy_move_cb, &ctx);

    *confirmed = ctx.confirmed;

    if (result == RECURSE_ABORT)
        return RECURSE_ABORT;

    if (recurse_truncated)
    {
        cwin_fill_rect(popup, 0, 6, 36, 1, CH_SPACE);
        cwin_putat_string(popup, 0, 6, MSG_FILE_RECURSE_TRUNCATED);
    }
    else if (move)
    {
        // Source root is now empty -- remove it (mirrors the single-file
        // move's loci_unlink of the source file).
        loci_unlink(srcpath);
    }

    return RECURSE_CONTINUE;
}

void file_copy_move_selected(uint8_t move)
// Copy or move selected files from active pane to non-active pane
{
    OricCharWin popup;
    uint8_t  confirmed = 0;
    uint8_t  target = !activepane;
    uint8_t  count = 0;
    uint16_t element;
    int16_t  copyresult;
    uint8_t  roomleft = (uint8_t)(255 - strlen(presentdir[target].path));

    if (!strcmp(presentdir[target].path, presentdir[activepane].path))
    {
        menu_messagepopup(MSG_FILE_TARGET_SAME);
        return;
    }

    if (presentdir[activepane].firstelement && !insidetape[activepane])
    {
        menu_popup_open(0, 8, 15);
        cwin_init(&popup, 2, 8, 38, 15, A_FWBLACK, A_BGWHITE);

        // If nothing is selected: select single entry (file or dir) on cursor pos
        if (!selection[activepane])
        {
            presentdirelement.meta.select = 1;
            dir_save_element(present);
            selection[activepane]++;
        }

        cwin_putat_printf(&popup, 0, 1, MSG_FILE_COPYMOVE_FMT,
                          move ? MSG_FILE_VAL_MOVE : MSG_FILE_VAL_COPY,
                          selection[activepane]);
        cwin_putat_string(&popup, 0, 2, presentdir[target].path);

        element = presentdir[activepane].firstelement;

        cwin_putat_string(&popup, 0, 4, MSG_FILE_PROCESSING);
        cwin_putat_string(&popup, 0, 7, MSG_FILE_ESC_CANCEL);

        // Loop through all elements to copy/move all selected files
        do
        {
            dir_get_element(element);

            if (presentdirelement.meta.select)
            {
                count++;
                cwin_fill_rect(&popup, 0, 5, 36, 1, CH_SPACE);
                cwin_putat_string(&popup, 0, 5, presentdirelement.name);

                // Check if path gets too long
                if (roomleft < strlen(presentdirelement.name))
                {
                    cwin_putat_string(&popup, 0, 6, MSG_FILE_PATH_TOO_LONG);
                    break;
                }

                // Derive full target and source paths
                dir_build_path(pathbuffer, sizeof(pathbuffer), presentdir[target].path, presentdirelement.name);
                dir_build_path(pathbuffer2, sizeof(pathbuffer2), presentdir[activepane].path, presentdirelement.name);

                if (presentdirelement.meta.type == 1)
                {
                    // Directory: recursive copy/move (merges into an
                    // existing destination directory if present)
                    if (file_copy_move_dir(pathbuffer2, pathbuffer, move, &popup, &confirmed) == RECURSE_ABORT)
                        break;
                }
                else
                {
                    // Check if file exists
                    if (file_exists(pathbuffer))
                    {
                        if (confirm || !confirmed)
                        {
                            if (!menu_confirm_file(MSG_FILE_OVERWRITE_Q, presentdirelement.name))
                                break;
                            confirmed = 1;
                        }
                        // Delete file if user selects overwrite
                        if (loci_unlink(pathbuffer) < 0)
                        {
                            menu_fileerrormessage();
                            break;
                        }
                    }

                    // Copy file with progress bar on row 14, columns 2..33
                    copyresult = file_copy_progress(pathbuffer, pathbuffer2, 2, 14, 32);
                    if (copyresult == -2)
                    {
                        cwin_fill_rect(&popup, 0, 6, 36, 1, CH_SPACE);
                        cwin_putat_string(&popup, 0, 6, MSG_FILE_COPY_CANCELLED);
                        break;
                    }
                    else if (copyresult != 0)
                    {
                        menu_fileerrormessage();
                        break;
                    }

                    // Delete source file if move is selected
                    if (move)
                    {
                        if (loci_unlink(pathbuffer2) < 0)
                        {
                            menu_fileerrormessage();
                            break;
                        }
                    }
                }

                // Check for ESC to cancel
                if (keyb_check() == KEY_ESC)
                    break;
            }
            element = presentdirelement.meta.next;
        } while (element);

        // Show message if no files selected
        if (!count)
            cwin_putat_string(&popup, 0, 5, MSG_FILE_NOTHING_COPY);

        cwin_fill_rect(&popup, 0, 7, 36, 1, CH_SPACE);
        cwin_putat_string(&popup, 0, 7, MSG_MENU_PRESSAKEY);
        cwin_getch();

        menu_popup_close();

        // Draw new dirs
        dir_draw(target, 1);
        if (move)
            dir_draw(activepane, 1);

        // Reset selection
        if (count || selection[activepane])
        {
            dir_select_all(0);
            selection[activepane] = 0;
        }
    }
    else
    {
        // Active pane empty (or browsing a virtual tape dir): nothing to
        // select/copy from here. Without this, the keypress was a silent
        // no-op -- indistinguishable from "doesn't work" when the user
        // expects to copy INTO the active pane's (empty) directory.
        menu_messagepopup(MSG_FILE_NOTHING_COPY);
    }
}

void file_delete(void)
// Delete selected files from active pane
{
    OricCharWin popup;
    uint8_t  confirmed = 0;
    uint8_t  count = 0;
    uint16_t element;

    if (presentdir[activepane].firstelement && !insidetape[activepane])
    {
        menu_popup_open(0, 8, 15);
        cwin_init(&popup, 2, 8, 38, 15, A_FWBLACK, A_BGWHITE);

        // If nothing is selected and cursor pos is not a dir: select single file on cursor pos
        if (!selection[activepane] && presentdirelement.meta.type > 1)
        {
            presentdirelement.meta.select = 1;
            dir_save_element(present);
            selection[activepane]++;
        }

        cwin_putat_printf(&popup, 0, 1, MSG_FILE_DELETE_FMT, selection[activepane]);

        element = presentdir[activepane].firstelement;

        // Loop through all elements to delete all selected files
        do
        {
            dir_get_element(element);

            if (presentdirelement.meta.select)
            {
                count++;
                cwin_fill_rect(&popup, 0, 3, 36, 1, CH_SPACE);
                cwin_putat_string(&popup, 0, 3, presentdirelement.name);

                // Confirm delete
                if (confirm || !confirmed)
                {
                    if (!menu_confirm_file(MSG_FILE_DELETE_Q, presentdirelement.name))
                        break;
                    confirmed = 1;
                }

                // Derive full target path
                dir_build_path(pathbuffer, sizeof(pathbuffer), presentdir[activepane].path, presentdirelement.name);

                if (loci_unlink(pathbuffer) < 0)
                {
                    menu_fileerrormessage();
                    break;
                }
            }
            element = presentdirelement.meta.next;
        } while (element);

        // Show message if no files selected
        if (!count)
            cwin_putat_string(&popup, 0, 3, MSG_FILE_DELETE_NONE);

        cwin_putat_string(&popup, 0, 7, MSG_MENU_PRESSAKEY);
        cwin_getch();

        menu_popup_close();

        // Draw new dir
        dir_draw(activepane, 1);

        // Reset selection
        if (count || selection[activepane])
        {
            dir_select_all(0);
            selection[activepane] = 0;
        }
    }
}

void file_rename(void)
// Rename selected file or directory
{
    OricCharWin popup;
    char input[64];
    char origname[64];

    if (presentdir[activepane].firstelement && !insidetape[activepane])
    {
        menu_popup_open(0, 8, 15);
        cwin_init(&popup, 2, 8, 38, 15, A_FWBLACK, A_BGWHITE);

        cwin_putat_string(&popup, 0, 1, MSG_FILE_RENAME_TITLE);

        strncpy(input, presentdirelement.name, sizeof(input) - 1);
        input[sizeof(input) - 1] = 0;

        // Is it a directory? Then remove trailing /
        if (presentdirelement.meta.type == 1)
            input[strlen(input) - 1] = 0;

        // Snapshot of the pre-edit name (trailing / already stripped for
        // dirs, same as input) to compare against after editing.
        strncpy(origname, input, sizeof(origname));

        // Set old name full path
        dir_build_path(pathbuffer, sizeof(pathbuffer), presentdir[activepane].path, presentdirelement.name);

        cwin_putat_string(&popup, 0, 3, MSG_FILE_RENAME_PROMPT);

        // Input new name and check if not cancelled or empty string
        if (cwin_textinput(&popup, 0, 4, 35, input, sizeof(input) - 1, VINPUT_ALL) > 0)
        {
            // Check if name is actually altered
            if (strcmp(input, origname))
            {
                // Set new name full path
                dir_build_path(pathbuffer2, sizeof(pathbuffer2), presentdir[activepane].path, input);

                if (loci_rename(pathbuffer, pathbuffer2) < 0)
                {
                    menu_fileerrormessage();
                    menu_popup_close();
                    return;
                }
                else
                {
                    // Redraw dir: name length can change, shifting all addresses
                    menu_popup_close();
                    dir_draw(activepane, 1);
                    return;
                }
            }
        }
        menu_popup_close();
    }
}

void file_browse_tape(void)
// Browse a .TAP tape archive
{
    if (presentdir[activepane].firstelement && !insidetape[activepane] && presentdirelement.meta.type == 3)
    {
        drive_mount();
        insidetape[activepane] = 1;
        dir_draw(activepane, 1);
    }
}
