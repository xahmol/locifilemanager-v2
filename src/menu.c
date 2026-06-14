// menu.c - Menu system for locifilemanager v2 (Oric Atmos, Oscar64)
//
// Based on v1 locifilemanager menu.c by Xander Mol (CC65/Oric).
// Rewritten for Oscar64 bare-metal: direct screen RAM access, no conio.h,
// full localisation via MSG_MENU_* macros, overlay RAM window save/restore.

#include <stdint.h>
#include <string.h>
#include "oric.h"
#include "keyboard.h"
#include "ijk.h"
#include "loci.h"
#include "strings.h"
#include "menu.h"

// -------------------------------------------------------------------------
// Screen RAM access helpers
// -------------------------------------------------------------------------

// Direct pointer to screen row y (avoids importing charwin's row_base table).
// Oscar64 -O2 handles 16-bit multiply; menus are called infrequently.
#define MENU_ROW(y) ((uint8_t *)((uint16_t)TEXTVRAM + (uint16_t)(y) * 40U))

/**
 * Write a single raw byte directly to screen RAM at (x, y).
 *
 * @param x Column (0-39).
 * @param y Row (0-27).
 * @param b Raw byte to write (character code or attribute).
 * @return (none)
 */
static void menu_screen_putb(uint8_t x, uint8_t y, uint8_t b)
{
    MENU_ROW(y)[x] = b;
}

/**
 * Write a NUL-terminated string directly to screen RAM starting at (x, y),
 * left to right with no wrapping or clipping.
 *
 * @param x Starting column (0-39).
 * @param y Row (0-27).
 * @param s NUL-terminated string to write.
 * @return (none)
 */
static void menu_screen_puts(uint8_t x, uint8_t y, const char *s)
{
    uint8_t *p = MENU_ROW(y) + x;
    while (*s) *p++ = (uint8_t)*s++;
}

/**
 * Write exactly n bytes from raw buffer buf to screen row y starting at x.
 * Replaces any byte < 0x20 (attribute codes, NUL) with a space to prevent
 * unintended attribute pollution from zero-padded pulldown_titles entries.
 *
 * @param x   Starting column (0-39).
 * @param y   Row (0-27).
 * @param buf Buffer of n raw bytes to write.
 * @param n   Number of bytes to write from buf.
 * @return (none)
 */
static void menu_screen_putn(uint8_t x, uint8_t y, const uint8_t *buf, uint8_t n)
{
    uint8_t *p = MENU_ROW(y) + x;
    for (uint8_t i = 0; i < n; i++)
    {
        uint8_t c = buf[i];
        *p++ = (c >= 0x20) ? c : 0x20;
    }
}

// -------------------------------------------------------------------------
// Window save/restore — full-row (40-byte) overlay RAM copies
// -------------------------------------------------------------------------

static MenuWinRecord menu_win_stack[MENU_WIN_DEPTH];
static uint8_t       menu_win_depth;
static uint16_t      menu_win_ptr;

/**
 * Push a record onto the window-save stack for height rows starting at
 * screen row ypos, and (if LOCI is present) copy those rows from screen RAM
 * into overlay RAM at the current menu_win_ptr, advancing menu_win_ptr by
 * height*40 bytes. If LOCI is absent, the depth/record bookkeeping still
 * happens but no actual copy is made (menus then leave screen residue on
 * close). No-op if the window-save stack (MENU_WIN_DEPTH) is already full.
 *
 * @param ypos   First screen row to save.
 * @param height Number of rows to save.
 * @return (none) -- result is written to menu_win_stack[], menu_win_depth,
 *         and menu_win_ptr.
 */
static void menu_winsave(uint8_t ypos, uint8_t height)
{
    if (menu_win_depth >= MENU_WIN_DEPTH) return;

    menu_win_stack[menu_win_depth].overlay_addr = menu_win_ptr;
    menu_win_stack[menu_win_depth].ypos         = ypos;
    menu_win_stack[menu_win_depth].height       = height;
    menu_win_depth++;

    if (!loci_present()) return;   // increment depth but skip actual copy in emulator

    uint8_t  *src = MENU_ROW(ypos);
    uint8_t  *dst = (uint8_t *)menu_win_ptr;
    uint16_t  len = (uint16_t)height * 40U;

    // PHP/PLP, not SEI/CLI: while overlay RAM is enabled, $C000-$FFFF (incl.
    // the IRQ vector at $FFFE/$FFFF and the ROM IRQ handler) is uninitialized
    // RAM — an IRQ here would jump to garbage. SEI alone prevents that, but
    // an unconditional CLI afterward would permanently re-enable IRQs (the
    // startup code leaves them disabled with no handler installed), letting
    // the stock ROM IRQ handler corrupt zero page / screen RAM thereafter.
    // PLP restores the interrupt-disable flag to whatever it was before.
    __asm { php }
    __asm { sei }
    enable_overlay_ram();
    for (uint16_t i = 0; i < len; i++)
        dst[i] = src[i];
    disable_overlay_ram();
    __asm { plp }

    menu_win_ptr += len;
}

/**
 * Pop the most recent record from the window-save stack and (if LOCI is
 * present) copy its saved rows back from overlay RAM to screen RAM,
 * restoring menu_win_ptr to the popped record's overlay address. No-op if
 * the window-save stack is empty.
 *
 * @return (none) -- result is written to screen RAM, menu_win_depth, and
 *         menu_win_ptr.
 */
static void menu_winrestore(void)
{
    if (menu_win_depth == 0) return;
    menu_win_depth--;

    uint16_t addr   = menu_win_stack[menu_win_depth].overlay_addr;
    uint8_t  ypos   = menu_win_stack[menu_win_depth].ypos;
    uint8_t  height = menu_win_stack[menu_win_depth].height;

    menu_win_ptr = addr;

    if (!loci_present()) return;

    uint8_t  *src = (uint8_t *)addr;
    uint8_t  *dst = MENU_ROW(ypos);
    uint16_t  len = (uint16_t)height * 40U;

    // PHP/PLP required — see menu_winsave() above.
    __asm { php }
    __asm { sei }
    enable_overlay_ram();
    for (uint16_t i = 0; i < len; i++)
        dst[i] = src[i];
    disable_overlay_ram();
    __asm { plp }
}

/**
 * Draw a white-paper popup background for rows ypos..ypos+height-1. Paper
 * (A_BGWHITE) at col 5, ink (A_FWBLACK=0x00) at col 6, spaces at cols 7-39.
 * Cols 0-4 retain existing content (part of the saved background).
 *
 * @param ypos   First screen row to paint.
 * @param height Number of rows to paint.
 * @return (none)
 */
static void menu_wininit(uint8_t ypos, uint8_t height)
{
    for (uint8_t y = 0; y < height; y++)
    {
        uint8_t *row = MENU_ROW(ypos + y);
        row[5] = A_BGWHITE;
        row[6] = A_FWBLACK;   // 0x00 — must write directly (not in string literal)
        for (uint8_t x = 7; x < 40; x++)
            row[x] = 0x20;
    }
}

/**
 * Open a full-width popup: save rows ypos..ypos+height-1, then paint a
 * white-paper background from column xpos to 39. Generalizes menu_wininit()
 * (hardcoded xpos=5) for dir/file popups that need to start at column 0.
 *
 * @param xpos   First column of the white-paper background (popup left edge).
 * @param ypos   First screen row to save and paint.
 * @param height Number of rows to save and paint.
 * @return (none)
 */
// Based on v1 windownew() (locifilemanager src/menu.c) by Xander Mol, 2025.
void menu_popup_open(uint8_t xpos, uint8_t ypos, uint8_t height)
{
    menu_winsave(ypos, height);
    for (uint8_t y = 0; y < height; y++)
    {
        uint8_t *row = MENU_ROW(ypos + y);
        row[xpos]     = A_BGWHITE;
        row[xpos + 1] = A_FWBLACK;   // 0x00 — must write directly (not in string literal)
        for (uint8_t x = (uint8_t)(xpos + 2); x < 40; x++)
            row[x] = 0x20;
    }
}

/**
 * Close the most recently opened menu_popup_open() window, restoring the
 * saved screen rows. Equivalent to menu_winrestore().
 *
 * @return (none)
 */
void menu_popup_close(void)
{
    menu_winrestore();
}

// -------------------------------------------------------------------------
// Input: keyboard + IJK joystick
// -------------------------------------------------------------------------

/**
 * Block until a key is pressed, polling both the keyboard (keyb_poll()) and,
 * if present, the IJK joystick (ijk_read()), mapping left-stick directions
 * to KEY_UP/DOWN/LEFT/RIGHT and fire to KEY_ENTER. After a joystick-derived
 * key, waits for both sticks to return to neutral before returning, to avoid
 * a held direction being read repeatedly by tight redraw/getkey loops (see
 * menu_pulldown()).
 *
 * @return Decoded ASCII/KEY_* code of the key or joystick action detected
 *         (never KEY_NONE).
 */
static uint8_t menu_getkey(void)
{
    uint8_t k;
    do {
        k = keyb_poll();
        if (ijk_present && k == KEY_NONE)
        {
            ijk_read();
            if      (ijk_ljoy & IJK_JOY_UP)    k = KEY_UP;
            else if (ijk_ljoy & IJK_JOY_DOWN)  k = KEY_DOWN;
            else if (ijk_ljoy & IJK_JOY_LEFT)  k = KEY_LEFT;
            else if (ijk_ljoy & IJK_JOY_RIGHT) k = KEY_RIGHT;
            else if (ijk_ljoy & IJK_JOY_FIRE)  k = KEY_ENTER;

            // Debounce: wait for stick to return to neutral before
            // returning the key, mirroring v1 getkey() in generic.c.
            // Without this, menu_pulldown's tight redraw/getkey loop
            // reads a held direction on every iteration and races
            // through every item per nudge (looks like the highlight
            // "disappears" — it's actually moving far too fast to see).
            if (k != KEY_NONE)
            {
                do {
                    ijk_read();
                } while (ijk_ljoy || ijk_rjoy);
                for (uint16_t d = 0; d < 1000; d++) keyb_scan();
            }
        }
    } while (k == KEY_NONE);
    return k;
}

// -------------------------------------------------------------------------
// Exported data — menus and pulldown tables
// -------------------------------------------------------------------------

MenuBar menubar = {
    { MSG_MENU_BAR_APP, MSG_MENU_BAR_FILE, MSG_MENU_BAR_DIR,
      MSG_MENU_BAR_MOUNTS, MSG_MENU_BAR_TOOLS, MSG_MENU_BAR_INFO },
    { 0, 0, 0, 0, 0, 0 },
    0
};

// Indices 0-5 are the top menu-bar pulldowns (App/File/Dir/Mounts/Tools/Info,
// matching MENUBAR_MAXOPTIONS=6 — menu_pulldown() uses
// menunumber < MENUBAR_MAXOPTIONS to pick the "top menu" end colour).
// Indices 6-10 are popup sub-menus opened via menu_option_select()/
// menu_areyousure()/menu_confirm_file() with an explicit pulldown index.
char pulldown_options[PULLDOWN_NUMBER] = { 5, 9, 7, 7, 4, 2, 3, 5, 4, 7, 2 };

char pulldown_titles[PULLDOWN_NUMBER][PULLDOWN_MAXOPTIONS][PULLDOWN_MAXLENGTH] = {
    // 0 — App
    { MSG_MENU_APP0, MSG_MENU_APP1, MSG_MENU_APP2, MSG_MENU_APP3, MSG_MENU_APP4 },
    // 1 — File
    { MSG_MENU_FILE0, MSG_MENU_FILE1, MSG_MENU_FILE2, MSG_MENU_FILE3,
      MSG_MENU_FILE4, MSG_MENU_FILE5, MSG_MENU_FILE6, MSG_MENU_FILE7, MSG_MENU_FILE8 },
    // 2 — Dir
    { MSG_MENU_DIR0, MSG_MENU_DIR1, MSG_MENU_DIR2, MSG_MENU_DIR3,
      MSG_MENU_DIR4, MSG_MENU_DIR5, MSG_MENU_DIR6 },
    // 3 — Mounts
    { MSG_MENU_MNT0, MSG_MENU_MNT1, MSG_MENU_MNT2, MSG_MENU_MNT3,
      MSG_MENU_MNT4, MSG_MENU_MNT5, MSG_MENU_MNT6 },
    // 4 — Tools
    { MSG_MENU_TOOLS0, MSG_MENU_TOOLS1, MSG_MENU_TOOLS2, MSG_MENU_TOOLS3 },
    // 5 — Info
    { MSG_MENU_INFO0, MSG_MENU_INFO1 },
    // 6 — Enter-action sub-menu
    { MSG_MENU_ENT0, MSG_MENU_ENT1, MSG_MENU_ENT2 },
    // 7 — Filter
    { MSG_MENU_FLT0, MSG_MENU_FLT1, MSG_MENU_FLT2, MSG_MENU_FLT3, MSG_MENU_FLT4 },
    // 8 — Target drive
    { MSG_MENU_DRV0, MSG_MENU_DRV1, MSG_MENU_DRV2, MSG_MENU_DRV3 },
    // 9 — Mount source
    { MSG_MENU_SRC0, MSG_MENU_SRC1, MSG_MENU_SRC2, MSG_MENU_SRC3,
      MSG_MENU_SRC4, MSG_MENU_SRC5, MSG_MENU_SRC6 },
    // 10 — Yes/No
    { MSG_MENU_YN0, MSG_MENU_YN1 }
};

// -------------------------------------------------------------------------
// Init
// -------------------------------------------------------------------------

/**
 * Initialise the menu module: reset the window-save stack (depth and overlay
 * pointer) and detect the IJK joystick interface (ijk_detect()). Call once
 * after charwin_init() and the loci_present() check.
 *
 * @return (none) -- result is written to menu_win_depth, menu_win_ptr, and
 *         the globals ijk_present/ijk_ljoy/ijk_rjoy (via ijk_detect()).
 */
void menu_init(void)
{
    menu_win_depth = 0;
    menu_win_ptr   = MENU_OVERLAY_BASE;
    ijk_detect();
}

// -------------------------------------------------------------------------
// Header and bar
// -------------------------------------------------------------------------

/**
 * Draw the application header at row 0: black ink, green paper across the
 * row, with header printed starting at column 2.
 *
 * @param header NUL-terminated header text to display.
 * @return (none)
 */
void menu_placeheader(const char *header)
{
    uint8_t *row = MENU_ROW(0);
    row[0] = A_FWBLACK;   // ink black (0x00 — direct write)
    row[1] = A_BGGREEN;   // paper green
    for (uint8_t x = 2; x < 40; x++) row[x] = 0x20;
    menu_screen_puts(2, 0, header);
}

/**
 * Draw the menu bar at row y: black ink, green paper across the row, with
 * each top-level menubar.titles[] entry printed left to right with a
 * one-column gap on each side. Computes and stores each item's highlight
 * column in menubar.xstart[], clamping the last item's start column so the
 * trailing un-highlight writes in menu_main() stay within the 40-column row.
 *
 * @param y Screen row to draw the bar on.
 * @return (none) -- result is written to menubar.ypos and menubar.xstart[].
 */
void menu_placebar(uint8_t y)
{
    uint8_t *row = MENU_ROW(y);
    row[0] = A_FWBLACK;
    row[1] = A_BGGREEN;
    for (uint8_t x = 2; x < 40; x++) row[x] = 0x20;

    menubar.ypos = y;

    uint8_t xcoord = 1;
    for (uint8_t i = 0; i < MENUBAR_MAXOPTIONS; i++)
    {
        uint8_t len = (uint8_t)strlen(menubar.titles[i]);
        // Keep room for the trailing un-highlight writes in menu_main()
        // (row[xi+1+len] and row[xi+2+len]): require xi <= 37-len so
        // row[xi+2+len] <= row[39], the last byte of the 40-byte row.
        if (xcoord + len + 2 > 39)
            xcoord = 37 - len;
        menubar.xstart[i] = xcoord;
        menu_screen_puts(xcoord + 1, y, menubar.titles[i]);
        xcoord += (uint8_t)(len + 2);
    }
}

/**
 * Clear the whole screen to black ink/black paper, then draw the application
 * header (menu_placeheader()) at row 0 and the menu bar (menu_placebar()) at
 * row 1.
 *
 * @param header NUL-terminated header text to display.
 * @return (none)
 */
void menu_placetop(const char *header)
{
    // Clear all rows to black paper / black ink
    for (uint8_t y = 0; y < SCREEN_ROWS; y++)
    {
        uint8_t *row = MENU_ROW(y);
        row[0] = A_FWBLACK;
        row[1] = A_BGBLACK;
        for (uint8_t x = 2; x < 40; x++) row[x] = 0x20;
    }
    menu_placeheader(header);
    menu_placebar(1);
}

// -------------------------------------------------------------------------
// Pulldown menu
// -------------------------------------------------------------------------

/**
 * Draw one pulldown item row at screen row y, columns xpos-1..xpos+2+width,
 * showing pulldown_titles[menunumber][item] either highlighted (yellow
 * paper, '-' prefix) or unhighlighted (cyan paper, ' ' prefix), padded with
 * spaces to width and followed by endcolor.
 *
 * @param y          Screen row to draw on.
 * @param xpos       Column of the highlight-bar ink byte (item text starts
 *                    at xpos+2).
 * @param menunumber Index into pulldown_titles[]/pulldown_options[].
 * @param item       Index of the item within this pulldown to draw.
 * @param selected   Non-zero to draw this item highlighted (yellow paper,
 *                    '-' prefix), zero for unhighlighted (cyan paper,
 *                    ' ' prefix).
 * @param endcolor   Attribute byte written immediately after the padded
 *                    label (A_BGBLACK for top-menu pulldowns, A_BGWHITE for
 *                    popup sub-menus).
 * @param width      Width (in characters) every row is padded to -- the
 *                    longest item label in this pulldown (computed by the
 *                    caller from the live strings, NOT PULLDOWN_MAXLENGTH),
 *                    so every row's highlight bar is the same width
 *                    regardless of that item's own label length.
 * @return (none)
 */
static void menu_draw_item(uint8_t y, uint8_t xpos, uint8_t menunumber,
                            uint8_t item, uint8_t selected, uint8_t endcolor,
                            uint8_t width)
{
    uint8_t *row   = MENU_ROW(y);
    // WORKAROUND for an Oscar64 -O2 whole-program register-allocator bug:
    // without this dummy sprintf, menu_pulldown()'s call-site save/restore
    // set is under-counted (saves only 2 zp bytes instead of the needed 13),
    // so a live caller variable gets clobbered and stray garbage appears on
    // screen at a row that tracks menuchoice. The sprintf call's register
    // pressure forces the correct (larger) save set. 0xA000 is scratch space
    // in the unused heap region (see oscar64manual.md, "-O2 whole-program
    // register allocator: caller-save set can be under-counted")
    // — do not remove or "clean up" this call without re-testing the App
    // pulldown in the emulator (corruption appears immediately on open).
    uint8_t *debug = (uint8_t *)0xA000;
    uint8_t  paper = selected ? A_BGYELLOW : A_BGCYAN;
    uint8_t  pfx   = selected ? '-' : ' ';
    const char *title = pulldown_titles[menunumber][item];
    uint8_t  len   = (uint8_t)strlen(title);

    row[xpos - 1] = A_FWBLACK;   // ink (0x00 — direct write)
    row[xpos]     = paper;
    row[xpos + 1] = pfx;
    menu_screen_putn(xpos + 2, y, (const uint8_t *)title, len);
    for (uint8_t x = len; x < width; x++)
        row[xpos + 2 + x] = 0x20;
    row[xpos + 2 + width] = endcolor;

    sprintf((char *)debug, "draw_item: y=%u, menunumber=%u, item=%u, sel=%u, title=\"%s\", width=%u", y, menunumber, item, selected, title, width);

}

/**
 * Open a pulldown menu at (xpos, ypos) listing pulldown_titles[menunumber],
 * saving the covered rows first (menu_winsave()) and restoring them on exit
 * (menu_winrestore()). Highlights the current item and handles KEY_UP/DOWN
 * to move the selection, KEY_ENTER to choose, and (for top-menu pulldowns,
 * menunumber < MENUBAR_MAXOPTIONS) KEY_LEFT/KEY_RIGHT to request the
 * previous/next bar item. If escapable, KEY_ESC cancels.
 *
 * @param xpos       Column of the highlight-bar ink byte for every item row.
 * @param ypos       First screen row of the pulldown (item 1).
 * @param menunumber Index into pulldown_titles[]/pulldown_options[] for the
 *                    pulldown to display.
 * @param escapable  Non-zero to allow KEY_ESC to cancel (returns
 *                    MENU_CANCEL); zero to require a choice.
 * @return 1..pulldown_options[menunumber] (the chosen item), MENU_CANCEL (0)
 *         if escapable and ESC was pressed, or MENU_LEFT_ARROW/
 *         MENU_RIGHT_ARROW if a top-menu pulldown's KEY_LEFT/KEY_RIGHT was
 *         pressed.
 */
uint8_t menu_pulldown(uint8_t xpos, uint8_t ypos,
                      uint8_t menunumber, uint8_t escapable)
{
    uint8_t height     = (uint8_t)pulldown_options[menunumber];
    uint8_t topmenu    = (menunumber < MENUBAR_MAXOPTIONS) ? 1 : 0;
    uint8_t endcolor   = topmenu ? A_BGBLACK : A_BGWHITE;
    uint8_t menuchoice = 1;
    uint8_t exit_flag  = 0;

    // Longest current item label in THIS pulldown — drives a consistent
    // highlight-bar width. Computed from the live strings (some entries
    // are sprintf'd at runtime with varying lengths), not assumed from
    // PULLDOWN_MAXLENGTH, so short pulldowns (e.g. Yes/No, drive letters)
    // get a compact bar instead of one stretched to the global max.
    uint8_t width = 0;
    for (uint8_t i = 0; i < height; i++)
    {
        uint8_t l = (uint8_t)strlen(pulldown_titles[menunumber][i]);
        if (l > width) width = l;
    }

    menu_winsave(ypos, height);

    // Draw all items (unselected)
    for (uint8_t y = 0; y < height; y++)
        menu_draw_item(ypos + y, xpos, menunumber, y, 0, endcolor, width);

    do {
        // Materialize the row index in its own variable so the compiler
        // can't (mis-)share the "ypos + menuchoice" subexpression between
        // the highlight-draw call and the un-highlight call below — it did
        // in an earlier build, drawing the un-highlight on the wrong row.
        uint8_t row_y = (uint8_t)(ypos + menuchoice - 1);

        // Highlight current choice
        menu_draw_item(row_y, xpos, menunumber,
                       menuchoice - 1, 1, endcolor, width);

        uint8_t key = menu_getkey();

        // Un-highlight before acting
        menu_draw_item(row_y, xpos, menunumber,
                       menuchoice - 1, 0, endcolor, width);

        switch (key)
        {
        case KEY_ENTER:
            exit_flag = 1;
            break;

        case KEY_ESC:
            if (escapable)
            {
                exit_flag  = 1;
                menuchoice = MENU_CANCEL;
            }
            break;

        case KEY_LEFT:
            if (topmenu)
            {
                exit_flag  = 1;
                menuchoice = MENU_LEFT_ARROW;
            }
            break;

        case KEY_RIGHT:
            if (topmenu)
            {
                exit_flag  = 1;
                menuchoice = MENU_RIGHT_ARROW;
            }
            break;

        case KEY_UP:
            if (menuchoice > 1) menuchoice--;
            else menuchoice = height;
            break;

        case KEY_DOWN:
            if (menuchoice < height) menuchoice++;
            else menuchoice = 1;
            break;

        default:
            break;
        }
    } while (!exit_flag);

    menu_winrestore();
    return menuchoice;
}

// -------------------------------------------------------------------------
// Main menu bar loop
// -------------------------------------------------------------------------

/**
 * Run the menu bar navigation loop: highlight bar items left/right with
 * KEY_LEFT/KEY_RIGHT, and on KEY_ENTER open the corresponding pulldown
 * (menu_pulldown()). If the pulldown returns MENU_LEFT_ARROW/
 * MENU_RIGHT_ARROW, move to the adjacent bar item and reopen its pulldown;
 * otherwise return the encoded choice. KEY_ESC at the bar level exits with
 * code 99.
 *
 * @return menubarchoice*10 + menuoptionchoice (1-69), or 99 if ESC/STOP was
 *         pressed at the menu-bar level.
 */
uint8_t menu_main(void)
{
    uint8_t menubarchoice    = 1;
    uint8_t menuoptionchoice = 0;

    menu_placebar(menubar.ypos);

    do {
        uint8_t key;
        do {
            // Highlight current bar item
            uint8_t  xi  = menubar.xstart[menubarchoice - 1];
            uint8_t  y   = menubar.ypos;
            uint8_t *row = MENU_ROW(y);
            uint8_t  len = (uint8_t)strlen(menubar.titles[menubarchoice - 1]);

            row[xi] = A_BGWHITE;
            menu_screen_puts(xi + 1, y, menubar.titles[menubarchoice - 1]);
            row[xi + 1 + len] = 0x20;
            row[xi + 2 + len] = A_BGGREEN;

            key = menu_getkey();

            // Un-highlight
            row[xi] = A_BGGREEN;
            menu_screen_puts(xi + 1, y, menubar.titles[menubarchoice - 1]);

            if (key == KEY_LEFT)
            {
                if (menubarchoice > 1) menubarchoice--;
                else menubarchoice = MENUBAR_MAXOPTIONS;
            }
            else if (key == KEY_RIGHT)
            {
                menubarchoice++;
                if (menubarchoice > MENUBAR_MAXOPTIONS) menubarchoice = 1;
            }
        } while (key != KEY_ENTER && key != KEY_ESC);

        if (key != KEY_ESC)
        {
            // Clamp xpos so pulldown fits within 40-col screen
            // Item occupies cols xpos-1..xpos+19; rightmost = xpos+19 ≤ 39 → xpos ≤ 20
            uint8_t xpos = menubar.xstart[menubarchoice - 1];
            if (xpos > 20) xpos = 20;

            menuoptionchoice = menu_pulldown(xpos, menubar.ypos + 1,
                                             menubarchoice - 1, 1);

            if (menuoptionchoice == MENU_LEFT_ARROW)
            {
                menuoptionchoice = 0;
                if (menubarchoice > 1) menubarchoice--;
                else menubarchoice = MENUBAR_MAXOPTIONS;
            }
            else if (menuoptionchoice == MENU_RIGHT_ARROW)
            {
                menuoptionchoice = 0;
                menubarchoice++;
                if (menubarchoice > MENUBAR_MAXOPTIONS) menubarchoice = 1;
            }
        }
        else
        {
            menuoptionchoice = 99;
        }
    } while (menuoptionchoice == 0);

    return (uint8_t)(menubarchoice * 10 + menuoptionchoice);
}

// -------------------------------------------------------------------------
// Popup helpers
// -------------------------------------------------------------------------

/**
 * Show a popup with message followed by "Are you sure?" and a Yes/No
 * pulldown (MENU_YESNO), then restore the screen.
 *
 * @param message NUL-terminated message text to display above the prompt.
 * @return 1 if Yes was chosen, 2 if No.
 */
uint8_t menu_areyousure(const char *message)
{
    uint8_t choice;
    menu_winsave(8, 6);
    menu_wininit(8, 6);
    menu_screen_puts(7, 9,  message);
    menu_screen_puts(7, 10, MSG_MENU_AREYOUSURE);
    choice = menu_pulldown(20, 11, MENU_YESNO, 0);
    menu_winrestore();
    return choice;
}

/**
 * Show a popup reporting a file-operation error: the generic "file error"
 * message, the current loci_errno value formatted as a decimal number
 * (without sprintf), and a "press a key" prompt. Waits for a keypress before
 * restoring the screen.
 *
 * @return (none)
 */
void menu_fileerrormessage(void)
{
    menu_winsave(8, 8);
    menu_wininit(8, 8);
    menu_screen_puts(7, 9, MSG_MENU_FILE_ERROR);

    // Display error label + loci_errno number without sprintf
    menu_screen_puts(7, 11, MSG_MENU_ERR_LABEL);
    uint8_t  en  = loci_errno;
    uint8_t  col = (uint8_t)(7 + strlen(MSG_MENU_ERR_LABEL));
    char     tmp[4];
    uint8_t  di  = 0;
    if (en == 0) { tmp[di++] = '0'; }
    else {
        if (en >= 100) tmp[di++] = (char)('0' + en / 100);
        if (en >=  10) tmp[di++] = (char)('0' + (en % 100) / 10);
        tmp[di++] = (char)('0' + en % 10);
    }
    tmp[di] = 0;
    menu_screen_puts(col, 11, tmp);

    menu_screen_puts(7, 13, MSG_MENU_PRESSAKEY);
    menu_getkey();
    menu_winrestore();
}

/**
 * Show a popup with message and a "press a key" prompt, wait for a keypress,
 * then restore the screen.
 *
 * @param message NUL-terminated message text to display.
 * @return (none)
 */
void menu_messagepopup(const char *message)
{
    menu_winsave(8, 6);
    menu_wininit(8, 6);
    menu_screen_puts(7, 9,  message);
    menu_screen_puts(7, 11, MSG_MENU_PRESSAKEY);
    menu_getkey();
    menu_winrestore();
}

/**
 * Show a popup with message, a "select option" prompt, and a sub-pulldown
 * (pulldown_titles[menu]) for selection, then restore the screen.
 *
 * @param message NUL-terminated prompt text to display above the
 *                 sub-pulldown.
 * @param menu    Index into pulldown_titles[]/pulldown_options[] for the
 *                 sub-pulldown to display.
 * @return 1..pulldown_options[menu] (the chosen item), or MENU_CANCEL (0) if
 *         ESC was pressed.
 */
uint8_t menu_option_select(const char *message, uint8_t menu)
{
    uint8_t option;
    // Save 14 rows (8..21) to safely contain the nested pulldown at ypos=12
    // with up to PULLDOWN_MAXOPTIONS=9 items (rows 12..20) plus margin.
    menu_winsave(8, 14);
    menu_wininit(8, 14);
    menu_screen_puts(7, 9,  message);
    menu_screen_puts(7, 11, MSG_MENU_SELECT_OPT);
    option = menu_pulldown(10, 12, menu, 1);
    menu_winrestore();
    return option;
}

/**
 * Show a full-width popup with message, "Name: <filename>" (filename
 * truncated to 30 chars), and a Yes/No pulldown (MENU_YESNO). Generic
 * confirm-with-filename dialog for dir/file operations (overwrite/delete
 * confirmations).
 *
 * @param message  NUL-terminated message text to display above the filename.
 * @param filename NUL-terminated filename to display (truncated to 30 chars
 *                  if longer).
 * @return 1 if Yes was chosen, 0 otherwise (No or ESC).
 */
// Based on v1 file_confirm_message() (locifilemanager src/file.c) by
// Xander Mol, 2025. Adapted: full-width popup via menu_popup_open(0,...)
// instead of windownew(0,...); raw screen writes instead of cputsxy/cprintf.
uint8_t menu_confirm_file(const char *message, const char *filename)
{
    uint8_t choice;
    char    namebuf[31];
    uint8_t i;

    menu_popup_open(0, 14, 9);
    menu_screen_puts(2, 16, message);
    menu_screen_puts(2, 18, MSG_MENU_CONFIRM_NAME);

    for (i = 0; i < 30 && filename[i]; i++) namebuf[i] = filename[i];
    namebuf[i] = '\0';
    menu_screen_puts(2, 19, namebuf);

    choice = menu_pulldown(10, 20, MENU_YESNO, 0);
    menu_popup_close();
    return (choice == 1) ? 1 : 0;
}
