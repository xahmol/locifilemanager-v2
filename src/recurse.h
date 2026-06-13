// recurse.h - Iterative recursive directory walker for locifilemanager v2
//
// recurse_walk() is a single iterative depth-first directory walker shared
// by recursive copy/move, recursive delete, and recursive directory-size
// (ROADMAP.md phases 1, 2, 7). Implemented with an explicit frame stack
// (RECURSE_MAX_DEPTH levels) instead of C-level recursion: the whole call
// chain runs under a 512-byte stack (#pragma stacksize(0x0200),
// include/oric_crt.c).

#ifndef RECURSE_H
#define RECURSE_H

#include <stdint.h>
#include "loci.h"

#define RECURSE_MAX_DEPTH 8

typedef enum { RECURSE_ENTER_DIR, RECURSE_FILE, RECURSE_LEAVE_DIR } RecurseEvent;

#define RECURSE_CONTINUE 0
#define RECURSE_ABORT    1

// entry is NULL for RECURSE_LEAVE_DIR (only fullpath is meaningful there).
// Return RECURSE_CONTINUE to keep walking or RECURSE_ABORT to stop early.
typedef int8_t (*RecurseCallback)(RecurseEvent ev, const LociDirent *entry,
                                   const char *fullpath, void *userdata);

// Set by recurse_walk() if RECURSE_MAX_DEPTH was reached and a subtree was
// skipped (or a subdirectory could not be opened). Caller should check this
// after recurse_walk() returns.
extern uint8_t recurse_truncated;

// Walks the CONTENTS of rootpath (rootpath itself is not reported).
// Returns RECURSE_CONTINUE (0) on completion, RECURSE_ABORT (1) if cb
// requested an early stop, or -1 if rootpath could not be opened.
int8_t recurse_walk(const char *rootpath, RecurseCallback cb, void *userdata);

#pragma compile("recurse.c")

#endif  // RECURSE_H
