// libdemo.c - Phase 2 library demo for Oric Atmos
//
// Tests the complete Phase 2 stack in Oricutron (no LOCI required):
//   - Oscar64 bare-metal buildchain (oric_crt.c, regions, startup)
//   - charwin library (screen I/O, colors, scroll, cursor)
//   - keyboard scanner (direct VIA/AY, no ROM calls)
//   - cwin_textinput (viewport scroll, cursor, ESC/ENTER)
//   - cwin_printf / cwin_putat_printf formatted output
//
// Overlay RAM (cwin_push/cwin_pop) is NOT tested here — requires LOCI.

#include "oric.h"
#include "charwin.h"
#include "keyboard.h"

// Hex digit table for key-echo display
static const char hexdig[16] = "0123456789ABCDEF";

// Write two-hex-digit representation of v into buf (no null terminator)
static void hex2(char *buf, uint8_t v)
{
    buf[0] = hexdig[v >> 4];
    buf[1] = hexdig[v & 0x0F];
}

int main(void)
{
    charwin_init();

    // -----------------------------------------------------------------------
    // Full-screen background: white ink, black paper
    // -----------------------------------------------------------------------

    OricCharWin scr;
    cwin_init(&scr, 2, 0, 38, 28, A_FWWHITE, A_BGBLACK);
    cwin_clear(&scr);

    cwin_putat_string(&scr, 1, 0,  "LOCI FILE MANAGER V2 - PHASE 2 TEST");
    cwin_putat_string(&scr, 1, 1,  "Oscar64 bare-metal Oric Atmos build");

    // -----------------------------------------------------------------------
    // Status window: green ink — shows build component status
    // -----------------------------------------------------------------------

    OricCharWin status;
    cwin_init(&status, 2, 3, 38, 5, A_FWGREEN, A_BGBLACK);
    cwin_clear(&status);
    cwin_putat_string(&status, 0, 0, "[ OK ] oric_crt.c startup + regions");
    cwin_putat_string(&status, 0, 1, "[ OK ] charwin library initialized");
    cwin_putat_string(&status, 0, 2, "[ OK ] keyboard scanner (VIA/AY)");
    cwin_putat_string(&status, 0, 3, "[ -- ] overlay RAM (requires LOCI)");
    cwin_putat_string(&status, 0, 4, "Press any key to continue...");

    cwin_getch();

    // -----------------------------------------------------------------------
    // Color test: three windows with different ink colors
    // -----------------------------------------------------------------------

    OricCharWin ctest;
    cwin_init(&ctest, 2, 9, 38, 1, A_FWWHITE, A_BGBLACK);
    cwin_clear(&ctest);
    cwin_putat_string(&ctest, 0, 0, "Color test:");

    OricCharWin c1;
    cwin_init(&c1, 2, 10, 12, 1, A_FWRED, A_BGBLACK);
    cwin_clear(&c1);
    cwin_putat_string(&c1, 0, 0, "RED   INK");

    OricCharWin c2;
    cwin_init(&c2, 2, 11, 12, 1, A_FWCYAN, A_BGBLACK);
    cwin_clear(&c2);
    cwin_putat_string(&c2, 0, 0, "CYAN  INK");

    OricCharWin c3;
    cwin_init(&c3, 2, 12, 12, 1, A_FWYELLOW, A_BGBLACK);
    cwin_clear(&c3);
    cwin_putat_string(&c3, 0, 0, "YELLW INK");

    // -----------------------------------------------------------------------
    // Text input test
    // -----------------------------------------------------------------------

    OricCharWin inp;
    cwin_init(&inp, 2, 14, 38, 4, A_FWWHITE, A_BGBLACK);
    cwin_clear(&inp);
    cwin_putat_string(&inp, 0, 0, "Text input (ENTER=accept, ESC=cancel):");
    cwin_putat_string(&inp, 0, 1, "> ");

    // Static: in BSS (zeroed by startup), no stack-aliasing with other locals.
    static char ibuf[29];
    signed int ires = cwin_textinput(&inp, 2, 1, 26, ibuf, 28, VINPUT_ALL);

    cwin_clear(&inp);
    if (ires >= 0)
    {
        cwin_putat_string(&inp, 0, 0, "Entered:");
        cwin_putat_string(&inp, 9, 0, ibuf);
    }
    else
    {
        cwin_putat_string(&inp, 0, 0, "ESC: input cancelled");
    }
    cwin_putat_string(&inp, 0, 2, "Press any key for key-echo loop...");
    cwin_getch();

    // -----------------------------------------------------------------------
    // Key echo loop: display hex code of each keypress.
    // ENTER or ESC exits (Oricutron may intercept PC ESC; use ENTER there).
    // -----------------------------------------------------------------------

    OricCharWin echo;
    cwin_init(&echo, 2, 19, 38, 8, A_FWCYAN, A_BGBLACK);
    cwin_clear(&echo);
    cwin_putat_string(&echo, 0, 0, "Key echo (hex). ENTER/ESC to halt:");

    // Cursor starts on row 1 col 0
    echo.cx = 0;
    echo.cy = 1;

    // Static: in BSS, avoids Oscar64 stack-aliasing with echo struct.
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

    // -----------------------------------------------------------------------
    // A. Full Colour Palette Grid — 8 ink x 8 paper
    // One window per paper row; inline ASTR_INK_* attrs show all 8 ink colours.
    // -----------------------------------------------------------------------

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, "A: Colour palette (8 ink x 8 paper)");
    cwin_putat_string(&scr, 0, 1, "    BLK RED GRN YEL BLU MAG CYN WHT");

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
        // Paper label in scr (white ink, no paper override)
        cwin_putat_string(&scr, 0, (uint8_t)(2 + p), pal_plabels[p]);

        // Ink colour samples: one window per paper row
        cwin_init(&palrow, 6, (uint8_t)(2 + p), 34, 1, A_FWWHITE, pal_papers[p]);
        cwin_clear(&palrow);
        palrow.cx = 0;
        palrow.cy = 0;

        // A_FWBLACK = 0x00 = NUL — cannot embed in string; use put_attr
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

    cwin_putat_string(&scr, 0, 11, "Press any key...");
    cwin_getch();

    // -----------------------------------------------------------------------
    // B. Inline Attribute Demo (ASTR macros)
    //
    // All labels are 7 chars wide so content aligns at col 8 (7 label + 1 attr).
    // Attr bytes display as a paper-colour box (Oric ULA: byte & 0x60 == 0).
    // Rows:
    //   0  Ink    — all 7 ink colours via ASTR_INK_*
    //   1  Paper  — all 8 paper backgrounds via ASTR_PAPER_*
    //   2  Alt    — alternate mosaic charset via ASTR_CHARSET_ALT
    //   3  Inv    — inverse video via bit-7 set chars (no ink attr needed)
    //   4  2Hi    — double-height charset top half (ASTR_CHARSET_STD2H = \x0A)
    //   5  (bot)  — double-height bottom half: same content as row 4 col 7+
    //   6  Blink  — blinking charset via ASTR_CHARSET_BLKSTD
    // -----------------------------------------------------------------------

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, "B: Inline attribute macros (ASTR)");

    OricCharWin demo;
    cwin_init(&demo, 2, 2, 38, 7, A_FWWHITE, A_BGBLACK);
    cwin_clear(&demo);

    // Row 0: ink colours — label 7 chars, first attr at col 7, text from col 8
    cwin_putat_string(&demo, 0, 0, "Ink:   "
        ASTR_INK_RED     "Red "
        ASTR_INK_GREEN   "Grn "
        ASTR_INK_YELLOW  "Yel "
        ASTR_INK_CYAN    "Cyn "
        ASTR_INK_MAGENTA "Mag "
        ASTR_INK_WHITE   "Whi");

    // Row 1: paper backgrounds — white ink already active from window attr at col 0
    cwin_putat_string(&demo, 0, 1, "Paper: "
        ASTR_PAPER_RED     "Red "
        ASTR_PAPER_GREEN   "Grn "
        ASTR_PAPER_BLUE    "Blu "
        ASTR_PAPER_MAGENTA "Mag "
        ASTR_PAPER_BLACK   "Blk");

    // Row 2: alternate mosaic charset then back to standard
    cwin_putat_string(&demo, 0, 2, "Alt:   "
        ASTR_CHARSET_ALT "\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C"
        ASTR_CHARSET_STD " Std");

    // Row 3: inverse video — char | 0x80 inverts pixel rendering.
    // ASTR_INK_WHITE at col 7 aligns content with other rows (redundant but structural).
    // Show "ORIC" in normal then inverse: 0xCF=inv-O 0xD2=inv-R 0xC9=inv-I 0xC3=inv-C
    cwin_putat_string(&demo, 0, 3, "Inv:   "
        ASTR_INK_WHITE
        "ORIC" "\xCF\xD2\xC9\xC3" " norm/inv");

    // Row 4+5: double-height via cwin_putat_dblhi_string — writes A_STD2H + text
    // + A_STD on BOTH rows y and y+1 from one call (row 5 bottom half automatic).
    cwin_putat_string(&demo, 0, 4, "2Hi:   ");
    cwin_putat_dblhi_string(&demo, 7, 4, "Dbl-Hi");

    // Row 6: blinking standard charset mode
    cwin_putat_string(&demo, 0, 6, "Blink: "
        ASTR_CHARSET_BLKSTD "blink"
        ASTR_CHARSET_STD    " norm");

    cwin_putat_string(&scr, 0, 9, "Press any key...");
    cwin_getch();

    // -----------------------------------------------------------------------
    // C. Fill Rect Pattern (concentric borders)
    // -----------------------------------------------------------------------

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, "C: fill rect (concentric borders)");

    OricCharWin frect;
    cwin_init(&frect, 2, 2, 38, 18, A_FWWHITE, A_BGBLACK);
    cwin_clear(&frect);

    cwin_fill_rect(&frect,  0,  0, 38, 18, '*');
    cwin_fill_rect(&frect,  1,  1, 36, 16, '#');
    cwin_fill_rect(&frect,  2,  2, 34, 14, '+');
    cwin_fill_rect(&frect,  3,  3, 32, 12, '.');
    cwin_fill_rect(&frect,  4,  4, 30, 10, CH_SPACE);

    cwin_putat_string(&scr, 0, 21, "Press any key...");
    cwin_getch();

    // -----------------------------------------------------------------------
    // D. Animated cursor walk — shows cursor_show + all four cursor-move funcs
    // -----------------------------------------------------------------------

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, "D: Animated cursor walk (perimeter)");

    OricCharWin walk;
    cwin_init(&walk, 2, 2, 38, 10, A_FWCYAN, A_BGBLACK);
    cwin_clear(&walk);

    // Delay between each step: 30 keyb_scan calls ≈ 11 ms per step.
    // Full perimeter (2*(38+10)-4 = 92 steps) ≈ 1 second.
#define WALK_STEPS 30

    // Top edge: left to right (cursor_right implied by put_char)
    for (uint8_t i = 0; i < walk.wx; i++)
    {
        walk.cx = i; walk.cy = 0;
        cwin_cursor_show(&walk, true);
        for (uint8_t d = 0; d < WALK_STEPS; d++) keyb_scan();
        cwin_cursor_show(&walk, false);
        cwin_putat_char(&walk, i, 0, '*');
    }

    // Right edge: down (cursor_down after each write)
    for (uint8_t i = 1; i < walk.wy; i++)
    {
        walk.cx = walk.wx - 1; walk.cy = i;
        cwin_cursor_show(&walk, true);
        for (uint8_t d = 0; d < WALK_STEPS; d++) keyb_scan();
        cwin_cursor_show(&walk, false);
        cwin_putat_char(&walk, walk.wx - 1, i, '*');
    }

    // Bottom edge: right to left (cursor_left)
    walk.cy = walk.wy - 1;
    for (int8_t i = (int8_t)(walk.wx - 2); i >= 0; i--)
    {
        walk.cx = (uint8_t)i;
        cwin_cursor_show(&walk, true);
        for (uint8_t d = 0; d < WALK_STEPS; d++) keyb_scan();
        cwin_cursor_show(&walk, false);
        cwin_putat_char(&walk, (uint8_t)i, walk.cy, '*');
    }

    // Left edge: bottom to top (cursor_up)
    walk.cx = 0;
    for (int8_t i = (int8_t)(walk.wy - 2); i >= 1; i--)
    {
        walk.cy = (uint8_t)i;
        cwin_cursor_show(&walk, true);
        for (uint8_t d = 0; d < WALK_STEPS; d++) keyb_scan();
        cwin_cursor_show(&walk, false);
        cwin_putat_char(&walk, 0, (uint8_t)i, '*');
    }

    cwin_putat_string(&scr, 0, 13, "Press any key...");
    cwin_getch();

    // -----------------------------------------------------------------------
    // E. Read-Back Verification (cwin_getat_char)
    // -----------------------------------------------------------------------

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, "E: getat char read-back test");

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
        cwin_putat_string(&vpass, 0, 0, "GETAT: PASS");
    }
    else
    {
        OricCharWin vfail;
        cwin_init(&vfail, 2, 4, 26, 1, A_FWRED, A_BGBLACK);
        cwin_clear(&vfail);
        cwin_putat_printf(&vfail, 0, 0, "GETAT: FAIL at col %u", (uint16_t)fail_col);
    }

    cwin_putat_string(&scr, 0, 6, "Press any key...");
    cwin_getch();

    // -----------------------------------------------------------------------
    // F. Scroll Speed Benchmark (500 scroll_up calls)
    // -----------------------------------------------------------------------

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, "F: Scroll speed benchmark (500 scrolls)");

    OricCharWin bench;
    cwin_init(&bench, 2, 2, 36, 10, A_FWYELLOW, A_BGBLACK);
    cwin_clear(&bench);

    // Seed with 10 numbered lines
    for (uint8_t i = 0; i < 10; i++)
        cwin_putat_printf(&bench, 0, i, "L%02u Seed line", (uint16_t)i);

    for (uint16_t f = 0; f < 500; f++)
    {
        cwin_scroll_up(&bench);
        cwin_putat_printf(&bench, 0, 9, "S%u scroll done", f);
    }

    cwin_putat_string(&scr, 0, 13, "500 scrolls: done. (fast!)");
    cwin_putat_string(&scr, 0, 14, "Press any key...");
    cwin_getch();

    // -----------------------------------------------------------------------
    // G. Bouncing Character Animation (2000 frames)
    //
    // Head: CH_INVSPACE (solid magenta block).
    // Trail: 'O' → 'o' → '.' (diminishing density) in magenta.
    // On Oric, per-char colour changes require an attr byte per column (2 cols
    // per coloured char), which breaks single-cell animation — char density
    // is used instead to show the trail effect.
    // Frame counter updates live every frame.
    // -----------------------------------------------------------------------

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, "G: Bounce ball (ESC/ENTER exits)");

    OricCharWin ball;
    cwin_init(&ball, 2, 2, 38, 18, A_FWMAGENTA, A_BGBLACK);
    cwin_clear(&ball);

    int8_t   dx = 1,   dy = 1;
    uint8_t  bx = 1,   by = 1;
    uint16_t frames = 0;

    // Trail: 3 positions behind the head (oldest erased, newest is old head)
    uint8_t px[3], py[3];
    px[0] = px[1] = px[2] = bx;
    py[0] = py[1] = py[2] = by;

    for (uint16_t f = 0; f < 2000; f++)
    {
        // Live frame counter
        cwin_putat_printf(&scr, 0, 21, "Frame: %u", (uint16_t)(f + 1));

        // Erase oldest trail position
        cwin_putat_char(&ball, px[2], py[2], CH_SPACE);

        // Shift trail (old head → trail[0], oldest shifts to [2])
        px[2] = px[1]; py[2] = py[1];
        px[1] = px[0]; py[1] = py[0];
        px[0] = bx;    py[0] = by;

        // Move head, bounce off walls
        bx = (uint8_t)(bx + dx);
        by = (uint8_t)(by + dy);
        if (bx == 0 || bx == ball.wx - 1) { dx = (int8_t)-dx; bx = (uint8_t)(bx + dx + dx); }
        if (by == 0 || by == ball.wy - 1) { dy = (int8_t)-dy; by = (uint8_t)(by + dy + dy); }

        // Draw trail (oldest → newest) then head — ensures head is topmost
        cwin_putat_char(&ball, px[2], py[2], '.');      // oldest trail
        cwin_putat_char(&ball, px[1], py[1], 'o');      // middle trail
        cwin_putat_char(&ball, px[0], py[0], 'O');      // newest trail (was head)
        cwin_putat_char(&ball, bx, by, CH_INVSPACE);    // head: solid magenta block

        frames = (uint16_t)(f + 1);
        uint8_t k = keyb_check();
        if (k == KEY_ENTER || k == KEY_ESC) break;
    }

    // Erase head and all trail positions
    cwin_putat_char(&ball, bx,    by,    CH_SPACE);
    cwin_putat_char(&ball, px[0], py[0], CH_SPACE);
    cwin_putat_char(&ball, px[1], py[1], CH_SPACE);
    cwin_putat_char(&ball, px[2], py[2], CH_SPACE);

    cwin_putat_printf(&scr, 0, 21, "Done: %u frames.    ", frames);
    cwin_putat_string(&scr, 0, 22, "Press any key...");
    cwin_getch();

    // -----------------------------------------------------------------------
    // Done
    // -----------------------------------------------------------------------

    cwin_clear(&scr);
    cwin_putat_string(&scr, 4, 13, "PHASE 2 TEST COMPLETE. HALTED.");

    while (1) {}
    return 0;
}
