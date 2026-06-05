// charwin.h - Character window library for Oric Atmos bare-metal
//
// Oric screen model (from Oricutron ula.c, confirmed hardware):
//   Attribute check: (byte & 0x60) == 0  → serial attribute (displays as paper-color box)
//   Character: everything else (0x20-0x7F, 0xA0-0xFF)
//   INK attrs:   0x00-0x07 (A_FW* values)
//   PAPER attrs: 0x10-0x17 (A_BG* values)
//   Inverse char: byte | 0x80  (e.g. 0xA0 = inverse space = solid ink block)
//   ULA resets to white-ink/black-paper at start of EACH rasterline.
//
// Window convention: cols 0-1 of every managed row hold INK+PAPER attributes.
// Window content starts at sx (minimum 2).

#ifndef CHARWIN_H
#define CHARWIN_H

#include <stdint.h>
#include <stdbool.h>
#include "oric.h"
#include "keyboard.h"

// -------------------------------------------------------------------------
// Text input validation flags (match v1 locifilemanager textInput)
// -------------------------------------------------------------------------

#define VINPUT_ALL      0   // All printable chars
#define VINPUT_NUMS     1   // Digits 0-9 only
#define VINPUT_ALPHA    2   // Alpha + digits (adds VINPUT_NUMS)
#define VINPUT_WILD     4   // Also allow '*' and '?'

// -------------------------------------------------------------------------
// OricCharWin — window descriptor
// -------------------------------------------------------------------------

typedef struct {
    uint8_t sx, sy;     // start col (min 2), start row
    uint8_t wx, wy;     // width, height in characters
    uint8_t cx, cy;     // cursor within window (0-based)
    uint8_t ink;        // INK color (A_FW* = 0-7)
    uint8_t paper;      // PAPER color (A_BG* = 16-23)
} OricCharWin;

// -------------------------------------------------------------------------
// Initialisation
// -------------------------------------------------------------------------

// Build row address lookup table. Call once before any cwin_* function.
void charwin_init(void);

// Populate window struct. Enforces sx >= 2.
void cwin_init(OricCharWin *w,
               uint8_t sx, uint8_t sy,
               uint8_t wx, uint8_t wy,
               uint8_t ink, uint8_t paper);

// -------------------------------------------------------------------------
// Screen I/O
// -------------------------------------------------------------------------

// Clear window: write INK at col 0, PAPER at col 1 of every row; fill
// content cols with space (0x20). Reset cursor to (0,0).
void cwin_clear(OricCharWin *w);

// Write ch at window-relative (x, y). No cursor update.
void cwin_putat_char(OricCharWin *w, uint8_t x, uint8_t y, uint8_t ch);

// Read ch at window-relative (x, y).
uint8_t cwin_getat_char(OricCharWin *w, uint8_t x, uint8_t y);

// Write null-terminated string starting at (x, y). Clips at window right edge.
void cwin_putat_string(OricCharWin *w, uint8_t x, uint8_t y, const char *s);

// Write ch at cursor, advance cursor. No newline/scroll.
void cwin_put_char(OricCharWin *w, uint8_t ch);

// Write string at cursor. No wrap/scroll.
void cwin_put_string(OricCharWin *w, const char *s);

// Write ch as console output: '\n' advances row (scrolls if needed), other
// chars wrap at right edge.
void cwin_console_put_char(OricCharWin *w, uint8_t ch);

// Write string via console (handles '\n').
void cwin_console_put_string(OricCharWin *w, const char *s);

// Fill bw×bh rectangle at window-relative (x,y) with ch. Clips to window.
void cwin_fill_rect(OricCharWin *w,
                    uint8_t x, uint8_t y,
                    uint8_t bw, uint8_t bh,
                    uint8_t ch);

// Scroll window content up by 1 row. New bottom row filled with spaces +
// attrs refreshed for that row.
void cwin_scroll_up(OricCharWin *w);

// -------------------------------------------------------------------------
// Cursor
// -------------------------------------------------------------------------

// Show (on=true) or hide cursor at current position using inverse-video toggle.
// Caller must track show/hide state to avoid double-toggling.
void cwin_cursor_show(OricCharWin *w, bool on);

// Move cursor one position. Return true if moved, false if already at edge.
bool cwin_cursor_left(OricCharWin *w);
bool cwin_cursor_right(OricCharWin *w);
bool cwin_cursor_up(OricCharWin *w);
bool cwin_cursor_down(OricCharWin *w);

// -------------------------------------------------------------------------
// Overlay RAM save/restore — REQUIRES LOCI device (not testable in Oricutron)
// -------------------------------------------------------------------------

// Push: copy full rows [sy, sy+wy) from screen RAM to overlay RAM (LIFO).
// Up to OVERLAY_STACK_DEPTH (8) levels.
void cwin_push(OricCharWin *w);

// Pop: restore last pushed rows from overlay RAM back to screen RAM.
void cwin_pop(OricCharWin *w);

// -------------------------------------------------------------------------
// Key input and text entry
// -------------------------------------------------------------------------

// Blocking key read (calls keyb_getch).
uint8_t cwin_getch(void);

// Text-input widget at window-relative (x, y).
//   vwidth:     visible viewport width (may be < maxlen; enables scrolling)
//   str:        pre-initialised buffer, at least maxlen+1 bytes
//   maxlen:     maximum string length (not counting null terminator)
//   validation: VINPUT_* flags (0 = all printable)
// Returns: string length on ENTER, -1 on ESC.
//
// Based on v1 locifilemanager textInput() by Xander Mol, adapted from
// DraBrowse 1.0e by Sascha Bader (2009).
signed int cwin_textinput(OricCharWin *w,
                          uint8_t x, uint8_t y,
                          uint8_t vwidth,
                          char *str, uint8_t maxlen,
                          uint8_t validation);

#pragma compile("charwin.c")

#endif  // CHARWIN_H
