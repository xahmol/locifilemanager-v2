// file.h - File operations (copy/move/delete/rename/tape browse) for
// locifilemanager v2.
//
// Based on v1 locifilemanager file.h/file.c by Xander Mol, 2025 — local
// reference at locifilemanager/include/file.h, locifilemanager/src/file.c.
//
// Adapted: stdint types; v1's file_confirm_message() is replaced by
// menu_confirm_file() (menu.h) — overwrite/delete confirmation popups are
// generic menu.c primitives now, not a file.c-local helper.

#ifndef FILE_H
#define FILE_H

#include <stdint.h>

// -------------------------------------------------------------------------
// Function prototypes
// -------------------------------------------------------------------------

void file_copy_move_selected(uint8_t move);
void file_delete(void);
void file_rename(void);
void file_browse_tape(void);

#pragma compile("file.c")

#endif  // FILE_H
