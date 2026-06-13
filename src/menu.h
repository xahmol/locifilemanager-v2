// menu.h - Menu system for locifilemanager v2 (Oric Atmos, Oscar64)
//
// Three-layer architecture:
//   menu bar (row 1) → pulldown menus → popup dialogs → LIFO window stack
//
// All user-visible strings come from MSG_MENU_* macros in strings_en/fr.h.
// Dynamic entries (confirm mode, filter, sort, target drive) are updated by
// main.c via snprintf into pulldown_titles[] before calling menu_main().
//
// Window save/restore uses full-row overlay RAM copies (40 bytes × height),
// independent of charwin's cwin_push/pop.  LOCI device required at runtime
// for save/restore; menus still display without LOCI but leave screen residue.
//
// Return encoding from menu_main():
//   menubarchoice * 10 + menuoptionchoice
//   99 = ESC/STOP from the menu bar

#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include "oric.h"     // OVERLAY_BASE, TEXTVRAM, SCREEN_ROWS, SCREEN_COLS, A_*

// -------------------------------------------------------------------------
// Capacity constants
// -------------------------------------------------------------------------

// Overlay RAM partition: charwin cwin_push/pop owns OVERLAY_BASE..OVERLAY_BASE+0x22FF
// (worst case 8 depths × 28 rows × 40 bytes = 8960 = 0x2300 bytes).
// Menu window saves start after that region.
#define MENU_OVERLAY_BASE   ((uint16_t)(OVERLAY_BASE + 0x2300U))  // 0xE300

#define MENUBAR_MAXOPTIONS   6
#define MENUBAR_MAXLENGTH   12   // char[] size for each bar title
#define PULLDOWN_NUMBER     11
#define PULLDOWN_MAXOPTIONS  9
#define PULLDOWN_MAXLENGTH  17   // 16 visible chars + NUL

#define MENU_WIN_DEPTH       9   // maximum nested window saves (matches v1 Window[9])

// Special return codes from menu_pulldown()
#define MENU_CANCEL          0   // ESC pressed (when escapable=1)
#define MENU_LEFT_ARROW     18   // left arrow → caller should open previous bar item
#define MENU_RIGHT_ARROW    19   // right arrow → caller should open next bar item

// Pulldown index for Yes/No dialog
#define MENU_YESNO          10

// -------------------------------------------------------------------------
// Structs
// -------------------------------------------------------------------------

typedef struct {
    uint16_t  overlay_addr;   // address in overlay RAM where rows are saved
    uint8_t   ypos;           // first screen row saved
    uint8_t   height;         // number of rows saved (each row = SCREEN_COLS bytes)
} MenuWinRecord;

typedef struct {
    char     titles[MENUBAR_MAXOPTIONS][MENUBAR_MAXLENGTH];
    uint8_t  xstart[MENUBAR_MAXOPTIONS];   // screen col of highlight attribute byte
    uint8_t  ypos;                          // screen row where bar is drawn
} MenuBar;

// -------------------------------------------------------------------------
// Exported data (defined in menu.c; updated by main.c for dynamic entries)
// -------------------------------------------------------------------------

extern MenuBar menubar;
extern char    pulldown_options[PULLDOWN_NUMBER];
extern char    pulldown_titles[PULLDOWN_NUMBER][PULLDOWN_MAXOPTIONS][PULLDOWN_MAXLENGTH];

// -------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------

// Initialise menu module: reset window stack, set overlay RAM pointer, detect IJK.
// Call once after charwin_init() and loci_present() check.
void menu_init(void);

// Draw application header at row 0 (green bar, header text).
void menu_placeheader(const char *header);

// Draw menu bar at row y; computes xstart[] positions from title lengths.
void menu_placebar(uint8_t y);

// Clear screen to black, draw header at row 0 and menu bar at row 1.
void menu_placetop(const char *header);

// Open a pulldown menu at (xpos, ypos) from pulldown_titles[menunumber].
//   escapable: 1 = ESC allowed, 0 = must choose
// Returns: 1..N (choice), MENU_CANCEL (0), MENU_LEFT_ARROW (18), MENU_RIGHT_ARROW (19).
uint8_t menu_pulldown(uint8_t xpos, uint8_t ypos,
                      uint8_t menunumber, uint8_t escapable);

// Run the menu bar navigation loop.
// Returns menubarchoice*10 + menuoptionchoice, or 99 (ESC/STOP).
uint8_t menu_main(void);

// Popup: show message + "Are you sure?" + Yes/No pulldown.
// Returns 1=Yes, 2=No.
uint8_t menu_areyousure(const char *message);

// Popup: show file error number (from loci_errno) + wait for keypress.
void menu_fileerrormessage(void);

// Popup: show message + wait for keypress.
void menu_messagepopup(const char *message);

// Popup: show message + a sub-pulldown for selection.
// Returns 1..N (choice) or MENU_CANCEL (0, if escaped).
uint8_t menu_option_select(const char *message, uint8_t menu);

// Popup: show message + "Name: <filename>" + Yes/No pulldown.
// Generic confirm-with-filename dialog for dir/file operations
// (overwrite/delete confirmations). Returns 1=Yes, 0=No.
uint8_t menu_confirm_file(const char *message, const char *filename);

// Open a full-width popup: save rows ypos..ypos+height-1, then paint a
// white-paper background from column xpos to 39 (col xpos = A_BGWHITE,
// col xpos+1 = A_FWBLACK, cols xpos+2..39 = spaces). Generalizes
// menu_wininit() (which is hardcoded to xpos=5) for dir/file popups that
// need to start at column 0. Pair with menu_popup_close().
void menu_popup_open(uint8_t xpos, uint8_t ypos, uint8_t height);

// Close the most recently opened menu_popup_open() window (restores the
// saved rows). Equivalent to menu_winrestore().
void menu_popup_close(void);

#pragma compile("menu.c")

#endif  // MENU_H
