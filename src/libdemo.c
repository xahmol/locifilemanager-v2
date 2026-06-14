// libdemo.c - library demo for Oric Atmos
//
// Tests the core stack in Oricutron (no LOCI required):
//   A–K: Oscar64 bare-metal buildchain, charwin library, keyboard scanner,
//        text editing, scroll down, viewport, menu system
// Tests the LOCI library (section L, requires real LOCI hardware):
//   L:   LOCI detection, firmware version, directory listing, IJK joystick
//
// Overlay RAM: tested on first screen when LOCI is detected.

#include <stdio.h>
#include "oric.h"
#include "charwin.h"
#include "keyboard.h"
#include "loci.h"
#include "ijk.h"
#include "strings.h"
#include "menu.h"
#include "strings_demo.h"

// Hex digit table for key-echo display
static const char hexdig[16] = "0123456789ABCDEF";

/**
 * Format an 8-bit value as two uppercase hex digits (no terminator written).
 *
 * @param buf Destination buffer; at least 2 bytes are written (buf[0..1]).
 * @param v   Value to format.
 * @return    (none)
 */
static void hex2(char *buf, uint8_t v)
{
    buf[0] = hexdig[v >> 4];
    buf[1] = hexdig[v & 0x0F];
}

/**
 * Overlay RAM write/read-back smoke test: enable overlay RAM, write two
 * sentinel bytes at OVERLAY_BASE, disable, then re-enable and read them
 * back. LOCI presence must be confirmed (loci_present()) before calling, as
 * overlay RAM requires real LOCI hardware.
 *
 * @return true if both sentinel bytes (0xA5, 0x5A) read back correctly
 *         (PASS), false otherwise.
 */
static bool test_overlay_ram(void)
{
    // volatile required: Oscar64 -O2 optimises writes to constant ROM-space
    // address (0xC000) as no-ops without it; reads would return ROM bytes.
    volatile uint8_t *oram = (volatile uint8_t *)OVERLAY_BASE;
    bool ok;

    // PHP/PLP, not SEI/CLI: oric_startup leaves IRQs permanently disabled
    // (no IRQ handler is installed). An unconditional CLI here would
    // re-enable IRQs for the rest of the program, letting the stock ROM
    // IRQ handler run every frame and corrupt zero page / screen RAM.
    __asm { php }
    __asm { sei }
    MICRODISCCFG = OVERLAY_ON;
    oram[0] = 0xA5;
    oram[1] = 0x5A;
    MICRODISCCFG = OVERLAY_OFF;
    __asm { plp }

    __asm { php }
    __asm { sei }
    MICRODISCCFG = OVERLAY_ON;
    ok = (oram[0] == 0xA5 && oram[1] == 0x5A);
    MICRODISCCFG = OVERLAY_OFF;
    __asm { plp }

    return ok;
}

/**
 * Library demo entry point. Runs a fixed sequence of self-contained demo
 * screens exercising the Oscar64 bare-metal buildchain, charwin library,
 * keyboard scanner and menu system (testable in Oricutron), plus the LOCI
 * library and overlay RAM (section L, requires real LOCI hardware):
 *   - Status/colour/text-input/key-echo intro screens
 *   - A: full colour palette grid
 *   - B: inline attribute (ASTR) demo
 *   - C: fill-rect concentric border pattern
 *   - D: animated cursor walk (cursor_show + move funcs)
 *   - E: read-back verification (cwin_getat_char)
 *   - F: text editing (insert/delete char, printline)
 *   - G: scroll-up speed benchmark
 *   - H: scroll-down demo
 *   - I: bouncing-character animation
 *   - J: viewport/map scroll demo
 *   - K: menu system demo (menu_main + popup helpers)
 *   - L: LOCI library demo (detection, firmware version, directory
 *        listing, IJK joystick; skipped if LOCI is absent)
 *   - M: cursor move/forward/backward/newline + put_chars/getat_chars
 *   - N: get_rect/put_rect screen-region save and restore
 *   - O: cwin_printwrap word-wrap demo
 *   - P: cwin_scroll_left/right horizontal scroll
 *   - Q: LOCI lseek/file I/O smoke test
 * Each section waits for a keypress before advancing. Never returns.
 *
 * @return Does not return (infinite input loop on the final screen).
 */
int main(void)
{
    charwin_init();

    // Detect LOCI and test overlay RAM early so results appear on the first screen
    bool loci_found = loci_present();
    bool oram_ok    = false;
    if (loci_found)
        oram_ok = test_overlay_ram();

    // ─────────────────────────────────────────────────────────────────────────
    // Full-screen background: white ink, black paper
    // ─────────────────────────────────────────────────────────────────────────

    OricCharWin scr;
    cwin_init(&scr, 2, 0, 38, 28, A_FWWHITE, A_BGBLACK);
    cwin_clear(&scr);

    cwin_putat_string(&scr, 1, 0, MSG_DEMO_TITLE);
    cwin_putat_string(&scr, 1, 1, MSG_DEMO_SUBTITLE);
    cwin_putat_printf(&scr, 1, 2, MSG_DEMO_VERSION,
        (uint16_t)VERSION_MAJOR,
        (uint16_t)VERSION_MINOR,
        (uint16_t)VERSION_PATCH);

    // ─────────────────────────────────────────────────────────────────────────
    // Status window: green ink — shows build component status
    // ─────────────────────────────────────────────────────────────────────────

    OricCharWin status;
    cwin_init(&status, 2, 4, 38, 5, A_FWGREEN, A_BGBLACK);
    cwin_clear(&status);
    cwin_putat_string(&status, 0, 0, MSG_DEMO_STATUS_CRT);
    cwin_putat_string(&status, 0, 1, MSG_DEMO_STATUS_CHARWIN);
    cwin_putat_string(&status, 0, 2, MSG_DEMO_STATUS_KEYB);
    if (!loci_found)
        cwin_putat_string(&status, 0, 3, MSG_DEMO_STATUS_ORAM);
    else if (oram_ok)
        cwin_putat_string(&status, 0, 3, MSG_DEMO_STATUS_ORAM_OK);
    else
        cwin_putat_string(&status, 0, 3, MSG_DEMO_STATUS_ORAM_ERR);
    cwin_putat_string(&status, 0, 4, MSG_DEMO_PRESS_KEY);

    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // Color quick-test: three windows with different ink colors
    // ─────────────────────────────────────────────────────────────────────────

    OricCharWin ctest;
    cwin_init(&ctest, 2, 9, 38, 1, A_FWWHITE, A_BGBLACK);
    cwin_clear(&ctest);
    cwin_putat_string(&ctest, 0, 0, MSG_DEMO_COLOR_TEST);

    OricCharWin c1;
    cwin_init(&c1, 2, 10, 12, 1, A_FWRED, A_BGBLACK);
    cwin_clear(&c1);
    cwin_putat_string(&c1, 0, 0, MSG_DEMO_COLOR_RED);

    OricCharWin c2;
    cwin_init(&c2, 2, 11, 12, 1, A_FWCYAN, A_BGBLACK);
    cwin_clear(&c2);
    cwin_putat_string(&c2, 0, 0, MSG_DEMO_COLOR_CYAN);

    OricCharWin c3;
    cwin_init(&c3, 2, 12, 12, 1, A_FWYELLOW, A_BGBLACK);
    cwin_clear(&c3);
    cwin_putat_string(&c3, 0, 0, MSG_DEMO_COLOR_YELLOW);

    // ─────────────────────────────────────────────────────────────────────────
    // Text input test
    // ─────────────────────────────────────────────────────────────────────────

    OricCharWin inp;
    cwin_init(&inp, 2, 14, 38, 4, A_FWWHITE, A_BGBLACK);
    cwin_clear(&inp);
    cwin_putat_string(&inp, 0, 0, MSG_DEMO_INPUT_PROMPT);
    cwin_putat_string(&inp, 0, 1, "> ");

    // Static: in BSS (zeroed by startup), no stack-aliasing with other locals
    static char ibuf[29];
    signed int ires = cwin_textinput(&inp, 2, 1, 26, ibuf, 28, VINPUT_ALL);

    cwin_clear(&inp);
    if (ires >= 0)
    {
        cwin_putat_string(&inp, 0, 0, MSG_DEMO_INPUT_ENTERED);
        cwin_putat_string(&inp, 9, 0, ibuf);
    }
    else
    {
        cwin_putat_string(&inp, 0, 0, MSG_DEMO_INPUT_CANCEL);
    }
    cwin_putat_string(&inp, 0, 2, MSG_DEMO_KEY_ECHO_NEXT);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // Key echo loop: display hex code of each keypress.
    // ENTER or ESC exits (Oricutron may intercept PC ESC; use ENTER there).
    // ─────────────────────────────────────────────────────────────────────────

    OricCharWin echo;
    cwin_init(&echo, 2, 19, 38, 8, A_FWCYAN, A_BGBLACK);
    cwin_clear(&echo);
    cwin_putat_string(&echo, 0, 0, MSG_DEMO_KEY_ECHO_HDR);

    echo.cx = 0;
    echo.cy = 1;

    // Static: in BSS, avoids Oscar64 stack-aliasing with echo struct
    static char buf[4];

    while (1)
    {
        uint8_t ch = cwin_getch();
        if (ch == KEY_ESC || ch == KEY_ENTER) break;

        hex2(buf, ch);
        buf[2] = ' ';
        buf[3] = 0;
        cwin_console_put_string(&echo, buf);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // A. Full Colour Palette Grid — 8 ink x 8 paper
    // One window per paper row; inline ASTR_INK_* attrs show all 8 ink colours.
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_A);
    cwin_putat_string(&scr, 0, 1, MSG_DEMO_SECTION_A_HDR);

    static const uint8_t pal_papers[8] = {
        A_BGBLACK, A_BGRED, A_BGGREEN, A_BGYELLOW,
        A_BGBLUE, A_BGMAGENTA, A_BGCYAN, A_BGWHITE
    };
    static const char * const pal_plabels[8] = {
        "BLK:", "RED:", "GRN:", "YEL:", "BLU:", "MAG:", "CYN:", "WHT:"
    };

    OricCharWin palrow;
    for (uint8_t p = 0; p < 8; p++)
    {
        cwin_putat_string(&scr, 0, (uint8_t)(2 + p), pal_plabels[p]);

        cwin_init(&palrow, 6, (uint8_t)(2 + p), 34, 1, A_FWWHITE, pal_papers[p]);
        cwin_clear(&palrow);
        palrow.cx = 0;
        palrow.cy = 0;

        // A_FWBLACK = 0x00 = NUL — cannot embed in string literal; use put_attr
        cwin_put_attr(&palrow, A_FWBLACK);
        cwin_put_string(&palrow,
            "BLK"
            ASTR_INK_RED     "RED"
            ASTR_INK_GREEN   "GRN"
            ASTR_INK_YELLOW  "YEL"
            ASTR_INK_BLUE    "BLU"
            ASTR_INK_MAGENTA "MAG"
            ASTR_INK_CYAN    "CYN"
            ASTR_INK_WHITE   "WHT");
    }

    cwin_putat_string(&scr, 0, 11, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // B. Inline Attribute Demo (ASTR macros)
    //
    // All labels are 7 chars wide so content aligns at col 8.
    // Rows: 0=ink, 1=paper, 2=alt charset, 3=inverse, 4-5=double-height, 6=blink
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_B);

    OricCharWin demo;
    cwin_init(&demo, 2, 2, 38, 7, A_FWWHITE, A_BGBLACK);
    cwin_clear(&demo);

    cwin_putat_string(&demo, 0, 0, "Ink:   "
        ASTR_INK_RED     "Red "
        ASTR_INK_GREEN   "Grn "
        ASTR_INK_YELLOW  "Yel "
        ASTR_INK_CYAN    "Cyn "
        ASTR_INK_MAGENTA "Mag "
        ASTR_INK_WHITE   "Whi");

    cwin_putat_string(&demo, 0, 1, "Paper: "
        ASTR_PAPER_RED     "Red "
        ASTR_PAPER_GREEN   "Grn "
        ASTR_PAPER_BLUE    "Blu "
        ASTR_PAPER_MAGENTA "Mag "
        ASTR_PAPER_BLACK   "Blk");

    cwin_putat_string(&demo, 0, 2, "Alt:   "
        ASTR_CHARSET_ALT "\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C"
        ASTR_CHARSET_STD " Std");

    // Inverse video: char | 0x80. Show ORIC normal then inverse.
    cwin_putat_string(&demo, 0, 3, "Inv:   "
        ASTR_INK_WHITE
        "ORIC" "\xCF\xD2\xC9\xC3" " norm/inv");

    // Double-height: writes A_STD2H + text + A_STD on rows y and y+1 from one call.
    cwin_putat_string(&demo, 0, 4, "2Hi:   ");
    cwin_putat_dblhi_string(&demo, 7, 4, "Dbl-Hi");

    cwin_putat_string(&demo, 0, 6, "Blink: "
        ASTR_CHARSET_BLKSTD "blink"
        ASTR_CHARSET_STD    " norm");

    cwin_putat_string(&scr, 0, 9, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // C. Fill Rect Pattern (concentric borders)
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_C);

    OricCharWin frect;
    cwin_init(&frect, 2, 2, 38, 18, A_FWWHITE, A_BGBLACK);
    cwin_clear(&frect);

    cwin_fill_rect(&frect,  0,  0, 38, 18, '*');
    cwin_fill_rect(&frect,  1,  1, 36, 16, '#');
    cwin_fill_rect(&frect,  2,  2, 34, 14, '+');
    cwin_fill_rect(&frect,  3,  3, 32, 12, '.');
    cwin_fill_rect(&frect,  4,  4, 30, 10, CH_SPACE);

    cwin_putat_string(&scr, 0, 21, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // D. Animated cursor walk — shows cursor_show + all four cursor-move funcs
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_D);

    OricCharWin walk;
    cwin_init(&walk, 2, 2, 38, 10, A_FWCYAN, A_BGBLACK);
    cwin_clear(&walk);

#define WALK_STEPS 30

    for (uint8_t i = 0; i < walk.wx; i++)
    {
        walk.cx = i; walk.cy = 0;
        cwin_cursor_show(&walk, true);
        for (uint8_t d = 0; d < WALK_STEPS; d++) keyb_scan();
        cwin_cursor_show(&walk, false);
        cwin_putat_char(&walk, i, 0, '*');
    }

    for (uint8_t i = 1; i < walk.wy; i++)
    {
        walk.cx = walk.wx - 1; walk.cy = i;
        cwin_cursor_show(&walk, true);
        for (uint8_t d = 0; d < WALK_STEPS; d++) keyb_scan();
        cwin_cursor_show(&walk, false);
        cwin_putat_char(&walk, walk.wx - 1, i, '*');
    }

    walk.cy = walk.wy - 1;
    for (int8_t i = (int8_t)(walk.wx - 2); i >= 0; i--)
    {
        walk.cx = (uint8_t)i;
        cwin_cursor_show(&walk, true);
        for (uint8_t d = 0; d < WALK_STEPS; d++) keyb_scan();
        cwin_cursor_show(&walk, false);
        cwin_putat_char(&walk, (uint8_t)i, walk.cy, '*');
    }

    walk.cx = 0;
    for (int8_t i = (int8_t)(walk.wy - 2); i >= 1; i--)
    {
        walk.cy = (uint8_t)i;
        cwin_cursor_show(&walk, true);
        for (uint8_t d = 0; d < WALK_STEPS; d++) keyb_scan();
        cwin_cursor_show(&walk, false);
        cwin_putat_char(&walk, 0, (uint8_t)i, '*');
    }

    cwin_putat_string(&scr, 0, 13, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // E. Read-Back Verification (cwin_getat_char)
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_E);

    OricCharWin verify;
    cwin_init(&verify, 2, 2, 38, 3, A_FWWHITE, A_BGBLACK);
    cwin_clear(&verify);

    static const char teststr[] = "0123456789";
    cwin_putat_string(&verify, 0, 0, teststr);

    bool getat_pass = true;
    uint8_t fail_col = 0;
    for (uint8_t i = 0; i < 10; i++)
    {
        if (cwin_getat_char(&verify, i, 0) != (uint8_t)teststr[i])
        {
            getat_pass = false;
            fail_col = i;
            break;
        }
    }

    if (getat_pass)
    {
        OricCharWin vpass;
        cwin_init(&vpass, 2, 4, 20, 1, A_FWGREEN, A_BGBLACK);
        cwin_clear(&vpass);
        cwin_putat_string(&vpass, 0, 0, MSG_DEMO_GETAT_PASS);
    }
    else
    {
        OricCharWin vfail;
        cwin_init(&vfail, 2, 4, 26, 1, A_FWRED, A_BGBLACK);
        cwin_clear(&vfail);
        cwin_putat_printf(&vfail, 0, 0, MSG_DEMO_GETAT_FAIL, (uint16_t)fail_col);
    }

    cwin_putat_string(&scr, 0, 6, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // F. Text Editing Demo — cwin_insert_char, cwin_delete_char, cwin_printline
    //
    // Shows insert (text shifts right) and delete (shifts left) with visible
    // delays, then cwin_printline with auto-scroll on a small window.
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_F);

#define EDIT_PAUSE 80   // ~30 ms at 1 MHz: enough to see each shift

#define SCROLL_BENCH_COUNT 150  // shortened from 500; skippable via keypress

    // Insert/delete demo window
    OricCharWin ewin;
    cwin_init(&ewin, 2, 2, 36, 1, A_FWCYAN, A_BGBLACK);
    cwin_clear(&ewin);
    cwin_putat_string(&ewin, 0, 0, "Hello, World!");
    ewin.cx = 5;
    ewin.cy = 0;

    cwin_fill_rect(&scr, 0, 4, 38, 1, CH_SPACE);
    cwin_putat_string(&scr, 0, 4, MSG_DEMO_EDIT_INIT);

    for (uint8_t i = 0; i < 5; i++)
    {
        for (uint8_t d = 0; d < EDIT_PAUSE; d++) keyb_scan();
        cwin_insert_char(&ewin);
    }

    cwin_fill_rect(&scr, 0, 4, 38, 1, CH_SPACE);
    cwin_putat_string(&scr, 0, 4, MSG_DEMO_EDIT_DELETE);

    for (uint8_t i = 0; i < 5; i++)
    {
        for (uint8_t d = 0; d < EDIT_PAUSE; d++) keyb_scan();
        cwin_delete_char(&ewin);
    }

    cwin_fill_rect(&scr, 0, 4, 38, 1, CH_SPACE);
    cwin_putat_string(&scr, 0, 4, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // Printline demo: 5-row window, print 8 lines — auto-scroll kicks in
    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_F);
    cwin_putat_string(&scr, 0, 1, MSG_DEMO_EDIT_PRINTLINE);

    OricCharWin pwin;
    cwin_init(&pwin, 2, 3, 36, 5, A_FWYELLOW, A_BGBLACK);
    cwin_clear(&pwin);

    for (uint8_t i = 1; i <= 8; i++)
    {
        cwin_printf(&pwin, MSG_DEMO_EDIT_LINE_FMT "\n", (uint16_t)i);
        for (uint8_t d = 0; d < EDIT_PAUSE; d++) keyb_scan();
    }

    cwin_putat_string(&scr, 0, 9, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // G. Scroll Speed Benchmark (500 scroll_up calls)
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_G);
    cwin_putat_string(&scr, 0, 1, MSG_DEMO_SCROLL_SKIP);

    OricCharWin bench;
    cwin_init(&bench, 2, 2, 36, 10, A_FWYELLOW, A_BGBLACK);
    cwin_clear(&bench);

    for (uint8_t i = 0; i < 10; i++)
        cwin_putat_printf(&bench, 0, i, "L%02u Seed line", (uint16_t)i);

    for (uint16_t f = 0; f < SCROLL_BENCH_COUNT; f++)
    {
        cwin_scroll_up(&bench);
        cwin_fill_rect(&bench, 0, 9, 36, 1, CH_SPACE);
        cwin_putat_printf(&bench, 0, 9, "S%u scroll done", f);
        if (keyb_check() != KEY_NONE) break;
    }

    cwin_putat_string(&scr, 0, 13, MSG_DEMO_SCROLL_DONE);
    cwin_putat_string(&scr, 0, 14, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // H. Scroll Down Demo — cwin_scroll_down (companion to section G)
    //
    // Inspired by Oscar64Test 1250 ScrollDown: fill a window with numbered
    // lines, then scroll down repeatedly while inserting new content at the
    // top row — content visibly drifts toward the bottom.
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_H);

    OricCharWin sdn;
    cwin_init(&sdn, 2, 2, 36, 10, A_FWGREEN, A_BGBLACK);
    cwin_clear(&sdn);

    for (uint8_t i = 0; i < 10; i++)
        cwin_putat_printf(&sdn, 0, i, "Line %02u (initial)", (uint16_t)i);

    for (uint8_t f = 0; f < 60; f++)
    {
        cwin_scroll_down(&sdn);
        cwin_putat_printf(&sdn, 0, 0, ">> New top line %u", (uint16_t)f);
        for (uint8_t d = 0; d < 30; d++) keyb_scan();   // ~11 ms per step
    }

    cwin_putat_string(&scr, 0, 13, MSG_DEMO_SCROLLDN_DONE);
    cwin_putat_string(&scr, 0, 14, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // I. Bouncing Character Animation (2000 frames)
    //
    // Head: CH_INVSPACE (solid magenta block).
    // Trail: 'O' -> 'o' -> '.' representing diminishing density.
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_I);

    OricCharWin ball;
    cwin_init(&ball, 2, 2, 38, 18, A_FWMAGENTA, A_BGBLACK);
    cwin_clear(&ball);

    int8_t   dx = 1,   dy = 1;
    uint8_t  bx = 1,   by = 1;
    uint16_t frames = 0;

    uint8_t px[3], py[3];
    px[0] = px[1] = px[2] = bx;
    py[0] = py[1] = py[2] = by;

    for (uint16_t f = 0; f < 2000; f++)
    {
        cwin_putat_printf(&scr, 0, 21, MSG_DEMO_FRAME, (uint16_t)(f + 1));

        cwin_putat_char(&ball, px[2], py[2], CH_SPACE);

        px[2] = px[1]; py[2] = py[1];
        px[1] = px[0]; py[1] = py[0];
        px[0] = bx;    py[0] = by;

        bx = (uint8_t)(bx + dx);
        by = (uint8_t)(by + dy);
        if (bx == 0 || bx == ball.wx - 1) { dx = (int8_t)-dx; bx = (uint8_t)(bx + dx + dx); }
        if (by == 0 || by == ball.wy - 1) { dy = (int8_t)-dy; by = (uint8_t)(by + dy + dy); }

        cwin_putat_char(&ball, px[2], py[2], '.');
        cwin_putat_char(&ball, px[1], py[1], 'o');
        cwin_putat_char(&ball, px[0], py[0], 'O');
        cwin_putat_char(&ball, bx, by, CH_INVSPACE);

        frames = (uint16_t)(f + 1);
        uint8_t k = keyb_check();
        if (k == KEY_ENTER || k == KEY_ESC) break;
    }

    cwin_putat_char(&ball, bx,    by,    CH_SPACE);
    cwin_putat_char(&ball, px[0], py[0], CH_SPACE);
    cwin_putat_char(&ball, px[1], py[1], CH_SPACE);
    cwin_putat_char(&ball, px[2], py[2], CH_SPACE);

    cwin_putat_printf(&scr, 0, 21, MSG_DEMO_FRAMES_DONE, frames);
    cwin_putat_string(&scr, 0, 22, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // J. Viewport / Map Scroll Demo — OricViewport + cwin_viewport_scroll
    //
    // Inspired by Oscar64Test 1260 Scroll4Way: a source map larger than the
    // display window; arrow keys scroll the view over the map interactively.
    //
    // Source map: 60 cols × 20 rows = 1200 bytes — must be static to avoid
    // exhausting the 256-byte 6502 stack.
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_J);
    cwin_putat_string(&scr, 0, 1, MSG_DEMO_VP_ARROWS);

    // Static source map: 60 wide × 20 tall
    static uint8_t vp_map[20 * 60];

    // Fill map: each row starts with "R%02u:" then repeating 0-9 pattern
    for (uint8_t row = 0; row < 20; row++)
    {
        uint8_t *p = vp_map + (uint16_t)row * 60U;
        // Row header: "R%02u:" (5 chars)
        p[0] = 'R';
        p[1] = (uint8_t)('0' + row / 10);
        p[2] = (uint8_t)('0' + row % 10);
        p[3] = ':';
        p[4] = ' ';
        // Fill rest with repeating digit pattern
        for (uint8_t col = 5; col < 60; col++)
            p[col] = (uint8_t)('0' + (col - 5) % 10);
    }

    // Viewport window: 34 cols × 10 rows at screen row 3
    OricCharWin vpwin;
    cwin_init(&vpwin, 2, 3, 34, 10, A_FWWHITE, A_BGBLUE);
    cwin_clear(&vpwin);

    OricViewport vp;
    cwin_viewport_init(&vp, vp_map, 60, 20, &vpwin);
    cwin_viewport_blit(&vp);

    {
        uint16_t vx = vp.viewx, vy = vp.viewy;
        cwin_putat_printf(&scr, 0, 14, MSG_DEMO_VP_POS, vx, vy);
    }

    while (1)
    {
        uint8_t vk = keyb_poll();
        if (vk == KEY_NONE) continue;

        if (vk == KEY_ESC || vk == KEY_ENTER) break;

        if (vk == KEY_UP || vk == KEY_DOWN ||
            vk == KEY_LEFT || vk == KEY_RIGHT)
        {
            cwin_viewport_scroll(&vp, vk);
            {
                uint16_t vx = vp.viewx, vy = vp.viewy;
                cwin_putat_printf(&scr, 0, 14, MSG_DEMO_VP_POS, vx, vy);
            }
        }
    }

    cwin_putat_string(&scr, 0, 14, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // K. Menu System Demo — menu_main + all four popup helpers
    //
    // menu_init() calls ijk_detect() internally; no separate call needed.
    // Window save/restore requires LOCI: in Oricutron the popup backgrounds
    // are not restored (screen residue), but the navigation still works.
    // ─────────────────────────────────────────────────────────────────────────

    menu_init();

    // Populate dynamic App pulldown entries
    sprintf(pulldown_titles[0][0], MSG_MENU_APP_CONFIRM_FMT, MSG_MENU_VAL_ONCE);
    sprintf(pulldown_titles[0][1], MSG_MENU_APP_RETURN_FMT,  MSG_MENU_VAL_SELECT);
    sprintf(pulldown_titles[0][2], MSG_MENU_APP_FILTER_FMT,  MSG_MENU_VAL_NONE);
    sprintf(pulldown_titles[0][3], MSG_MENU_APP_SORT_FMT,    MSG_MENU_VAL_OFF);
    sprintf(pulldown_titles[3][5], MSG_MENU_MNT_TARGET_FMT,  "A");

    menu_placetop(MSG_MENU_HEADER);

    // Print demo instructions in the content area (row 3+)
    OricCharWin minfo;
    cwin_init(&minfo, 2, 3, 38, 2, A_FWWHITE, A_BGBLACK);
    cwin_clear(&minfo);
    cwin_putat_string(&minfo, 0, 0, MSG_DEMO_SECTION_K);
    cwin_putat_string(&minfo, 0, 1, MSG_DEMO_MENU_INTRO);

    OricCharWin mresult;
    cwin_init(&mresult, 2, 25, 38, 1, A_FWCYAN, A_BGBLACK);
    cwin_clear(&mresult);

    // menu_main() encodes its result as menubarchoice*10 + menuoptionchoice.
    // Valid selections range 10..59; ESC-at-bar sets menuoptionchoice=99,
    // giving 109..159 (always >=100). "App > [ESC] Exit" (item 4) encodes
    // as 1*10 + 5 = 15 — exit the demo on either signal.
    uint8_t mchoice;
    do {
        mchoice = menu_main();
        cwin_putat_printf(&mresult, 0, 0, MSG_DEMO_MENU_CHOICE, (uint16_t)mchoice);
    } while (mchoice != 15 && mchoice < 100);

    // Popup demonstrations
    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_K);
    cwin_putat_string(&scr, 0, 2, MSG_DEMO_MENU_POPUPS);

    // 1. Message popup
    menu_messagepopup(MSG_DEMO_MENU_POPUP_MSG);

    // 2. Are-you-sure
    uint8_t ans = menu_areyousure(MSG_DEMO_MENU_POPUP_ASK);
    cwin_putat_printf(&scr, 0, 4, MSG_DEMO_MENU_POPUP_RES, (uint16_t)ans);

    // 3. Option select (Enter-action sub-menu = pulldown 6)
    uint8_t sel = menu_option_select(MSG_DEMO_SECTION_K, 6);
    cwin_putat_printf(&scr, 0, 6, MSG_DEMO_MENU_SEL_RES, (uint16_t)sel);

    // 4. File error popup (simulated errno=42)
    cwin_putat_string(&scr, 0, 8, MSG_DEMO_MENU_ERR_SIM);
    loci_errno = 42;
    menu_fileerrormessage();

    cwin_putat_string(&scr, 0, 10, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // L. LOCI Library Demo
    //
    // Requires real LOCI hardware. In Oricutron (no LOCI), shows "Not found"
    // and stops gracefully — no hang, no crash.
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_L);

    OricCharWin lwin;
    cwin_init(&lwin, 2, 2, 38, 24, A_FWWHITE, A_BGBLACK);
    cwin_clear(&lwin);

    // Step 1: detection (result already known from startup check)
    if (!loci_found)
    {
        cwin_putat_string(&lwin, 0, 0, MSG_DEMO_LOCI_NOT_FOUND);
        cwin_putat_string(&lwin, 0, 2, MSG_DEMO_PRESS_KEY);
        cwin_getch();
        // Skip to end — no LOCI operations possible
        cwin_clear(&scr);
        cwin_putat_string(&scr, 4, 13, MSG_DEMO_DONE);
        while (1) {}
    }

    cwin_putat_string(&lwin, 0, 0, MSG_DEMO_LOCI_DETECTED);

    // Step 2: query firmware version and CPU speed before opening dir.
    // Display is deferred to AFTER the directory listing (rows 16–17) so that
    // the dir listing cannot overwrite the firmware/CPU lines regardless of
    // any XSTACK residue from loci_uname.
    int16_t cpu_khz = phi2();
    loci_uname(&locicfg.uname);
    {
        const char *rel = locicfg.uname.release;
        locicfg.version.major = (uint8_t)(rel[0] - '0');
        locicfg.version.minor = (uint8_t)(rel[2] - '0');
        locicfg.version.patch = (uint8_t)(rel[4] - '0');
    }

    // Step 3: root directory listing (first 12 entries, rows 3–14)
    {
        cwin_putat_string(&lwin, 0, 2, MSG_DEMO_LOCI_DIR_HDR);

        LociDir    *dir  = loci_opendir("");
        LociDirent *ent;
        uint8_t     row  = 3;
        uint8_t     devnr = 0;

        while (row < 15)
        {
            ent = loci_readdir(dir);
            if (!ent || ent->d_name[0] == '\0') break;

            const char *typ = (ent->d_attrib & DIR_ATTR_DIR)
                              ? MSG_DEMO_LOCI_TYPE_DIR
                              : MSG_DEMO_LOCI_TYPE_FILE;

            // Check for MSC device entry (USB mass storage)
            uint8_t devid = (uint8_t)(ent->d_name[0] - '0');
            if (devid && ent->d_name[3] == 'M' && ent->d_name[4] == 'S' && ent->d_name[5] == 'C')
                devnr++;

            cwin_putat_printf(&lwin, 0, row, "%s %s", typ, ent->d_name);
            row++;
        }
        loci_closedir(dir);

        // Step 4: firmware and CPU displayed after dir listing
        cwin_putat_printf(&lwin, 0, 16, MSG_DEMO_LOCI_FW_VER "%u.%u.%u",
            (uint16_t)locicfg.version.major,
            (uint16_t)locicfg.version.minor,
            (uint16_t)locicfg.version.patch);

        cwin_putat_printf(&lwin, 0, 17, MSG_DEMO_LOCI_CPU_KHZ "%u" MSG_DEMO_LOCI_KHZ_UNIT,
            (uint16_t)cpu_khz);

        cwin_putat_printf(&lwin, 0, 19, MSG_DEMO_LOCI_DEV_COUNT "%u", (uint16_t)devnr);
    }

    // Step 5: IJK joystick detection
    ijk_detect();
    cwin_putat_string(&lwin, 0, 21,
        ijk_present ? MSG_DEMO_LOCI_IJK_FOUND : MSG_DEMO_LOCI_IJK_NONE);

    // Step 6: done
    cwin_putat_string(&lwin, 0, 23, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // M. Cursor move / forward / backward / newline + put_chars / getat_chars
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_M);

    // Small test window: 30 cols × 10 rows
    OricCharWin mw;
    cwin_init(&mw, 2, 2, 30, 10, A_FWCYAN, A_BGBLACK);
    cwin_clear(&mw);

    // 1. cursor_move then put_char
    cwin_putat_string(&scr, 0, 13, MSG_DEMO_M_MOVE);
    cwin_cursor_move(&mw, 10, 2);
    cwin_put_char(&mw, '*');

    // 2. cursor_forward across line boundary: move to last col of row 0, then forward
    cwin_cursor_move(&mw, mw.wx - 1, 0);
    cwin_cursor_forward(&mw);   // should wrap to (0, 1)
    cwin_put_char(&mw, 'F');    // 'F' at (0,1) proves forward wrapped

    // 3. cursor_backward across line boundary: move to col 0 row 2, then backward
    cwin_cursor_move(&mw, 0, 2);
    cwin_cursor_backward(&mw);  // should wrap to (wx-1, 1)
    cwin_put_char(&mw, 'B');    // 'B' at (wx-1, 1) proves backward wrapped

    // 4. put_chars: write "HELLO" at cursor (2,3)
    cwin_putat_string(&scr, 0, 14, MSG_DEMO_M_PUT_CHARS);
    cwin_cursor_move(&mw, 2, 3);
    const char pc_src[5] = { 'H', 'E', 'L', 'L', 'O' };
    cwin_put_chars(&mw, pc_src, 5);

    // 5. getat_chars: read back those 5 chars and verify
    cwin_putat_string(&scr, 0, 15, MSG_DEMO_M_GETAT);
    char gc_buf[5];
    cwin_getat_chars(&mw, 2, 3, gc_buf, 5);
    bool m_pass = (gc_buf[0] == 'H' && gc_buf[1] == 'E' &&
                   gc_buf[2] == 'L' && gc_buf[3] == 'L' && gc_buf[4] == 'O');
    cwin_putat_string(&scr, 0, 16, MSG_DEMO_M_VERIFY);
    cwin_putat_string(&scr, 13, 16, m_pass ? MSG_DEMO_M_PASS : MSG_DEMO_M_FAIL);

    cwin_putat_string(&scr, 0, 18, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // N. get_rect / put_rect — screen region save and restore
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_N);

    OricCharWin nw;
    cwin_init(&nw, 2, 2, 30, 8, A_FWWHITE, A_BGBLUE);
    cwin_clear(&nw);

    // Draw recognisable content pattern
    for (uint8_t r = 0; r < 8; r++)
    {
        cwin_cursor_move(&nw, 0, r);
        for (uint8_t c = 0; c < 30; c++)
            cwin_put_char(&nw, (uint8_t)('A' + (c + r) % 26));
    }
    cwin_putat_string(&scr, 0, 11, MSG_DEMO_N_ORIGINAL);
    cwin_putat_string(&scr, 0, 12, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // Save a 12×3 rectangle at window (5, 2)
    static char rect_buf[12 * 3];
    cwin_get_rect(&nw, 5, 2, 12, 3, rect_buf);

    // Overwrite that region with '#'
    cwin_fill_rect(&nw, 5, 2, 12, 3, '#');
    cwin_putat_string(&scr, 0, 11, MSG_DEMO_N_SAVED);
    cwin_putat_string(&scr, 0, 12, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // Restore from buffer
    cwin_put_rect(&nw, 5, 2, 12, 3, rect_buf);
    cwin_putat_string(&scr, 0, 11, MSG_DEMO_N_RESTORED);
    cwin_putat_string(&scr, 0, 12, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // O. cwin_printwrap — word-wrap print into a narrow window
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_O);
    cwin_putat_string(&scr, 0, 2, MSG_DEMO_O_LABEL);

    // Narrow window: 20 cols × 10 rows so wrap is clearly visible
    OricCharWin ow;
    cwin_init(&ow, 2, 4, 20, 10, A_FWYELLOW, A_BGBLACK);
    cwin_clear(&ow);
    cwin_printwrap(&ow, MSG_DEMO_O_TEXT);

    cwin_putat_string(&scr, 0, 15, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // P. cwin_scroll_left / cwin_scroll_right — horizontal scroll
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_P);

    OricCharWin pw;
    cwin_init(&pw, 2, 2, 34, 8, A_FWGREEN, A_BGBLACK);
    cwin_clear(&pw);

    // Fill each row with repeating uppercase letter pattern
    for (uint8_t r = 0; r < 8; r++)
    {
        cwin_cursor_move(&pw, 0, r);
        for (uint8_t c = 0; c < 34; c++)
            cwin_put_char(&pw, (uint8_t)('A' + (c + r) % 26));
    }

    cwin_putat_string(&scr, 0, 11, MSG_DEMO_P_FILLED);
    cwin_putat_string(&scr, 0, 12, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // Scroll left 3× by 3
    cwin_scroll_left(&pw, 3);
    cwin_scroll_left(&pw, 3);
    cwin_scroll_left(&pw, 3);

    cwin_putat_string(&scr, 0, 11, MSG_DEMO_P_LEFT_DONE);
    cwin_putat_string(&scr, 0, 12, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // Scroll right 3× by 3
    cwin_scroll_right(&pw, 3);
    cwin_scroll_right(&pw, 3);
    cwin_scroll_right(&pw, 3);

    cwin_putat_string(&scr, 0, 11, MSG_DEMO_P_RIGHT_DONE);
    cwin_putat_string(&scr, 0, 12, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // Q. LOCI lseek / file I/O smoke test
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_Q);

    // 32-byte recognisable buffer: 'A'..'`'
    char qbuf[32];
    for (uint8_t i = 0; i < 32; i++)
        qbuf[i] = (char)('A' + i);

    int16_t qfd = loci_open("LSEEKTST.BIN", O_CREAT | O_TRUNC | O_RDWR);
    bool q1_pass = (qfd >= 0) && (loci_write(qfd, qbuf, 32) == 32);
    cwin_putat_string(&scr, 0, 1, MSG_DEMO_Q1);
    cwin_putat_string(&scr, 24, 1, q1_pass ? MSG_DEMO_M_PASS : MSG_DEMO_M_FAIL);

    char qc;
    bool q2_pass = (loci_lseek(qfd, 10, SEEK_SET) == 10) &&
                   (loci_read(qfd, &qc, 1) == 1) && (qc == qbuf[10]);
    cwin_putat_string(&scr, 0, 2, MSG_DEMO_Q2);
    cwin_putat_string(&scr, 24, 2, q2_pass ? MSG_DEMO_M_PASS : MSG_DEMO_M_FAIL);

    bool q3_pass = (loci_lseek(qfd, 5, SEEK_CUR) == 16) &&
                   (loci_read(qfd, &qc, 1) == 1) && (qc == qbuf[16]);
    cwin_putat_string(&scr, 0, 3, MSG_DEMO_Q3);
    cwin_putat_string(&scr, 24, 3, q3_pass ? MSG_DEMO_M_PASS : MSG_DEMO_M_FAIL);

    bool q4_pass = (loci_lseek(qfd, -7, SEEK_CUR) == 10) &&
                   (loci_read(qfd, &qc, 1) == 1) && (qc == qbuf[10]);
    cwin_putat_string(&scr, 0, 4, MSG_DEMO_Q4);
    cwin_putat_string(&scr, 24, 4, q4_pass ? MSG_DEMO_M_PASS : MSG_DEMO_M_FAIL);

    bool q5_pass = (loci_lseek(qfd, -4, SEEK_END) == 28) &&
                   (loci_read(qfd, &qc, 1) == 1) && (qc == qbuf[28]);
    cwin_putat_string(&scr, 0, 5, MSG_DEMO_Q5);
    cwin_putat_string(&scr, 24, 5, q5_pass ? MSG_DEMO_M_PASS : MSG_DEMO_M_FAIL);

    bool q6_pass = (loci_lseek(qfd, 0, SEEK_END) == 32) &&
                   (loci_read(qfd, &qc, 1) == 0);
    cwin_putat_string(&scr, 0, 6, MSG_DEMO_Q6);
    cwin_putat_string(&scr, 24, 6, q6_pass ? MSG_DEMO_M_PASS : MSG_DEMO_M_FAIL);

    bool q7_pass = (loci_close(qfd) >= 0) && (loci_unlink("LSEEKTST.BIN") >= 0);
    cwin_putat_string(&scr, 0, 7, MSG_DEMO_Q7);
    cwin_putat_string(&scr, 24, 7, q7_pass ? MSG_DEMO_M_PASS : MSG_DEMO_M_FAIL);

    cwin_putat_string(&scr, 0, 9, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // Done
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 4, 13, MSG_DEMO_DONE);

    while (1) {}
    return 0;
}
