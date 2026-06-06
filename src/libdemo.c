// libdemo.c - Phase 2+3 library demo for Oric Atmos
//
// Tests the complete Phase 2 stack in Oricutron (no LOCI required):
//   A–G: Oscar64 bare-metal buildchain, charwin library, keyboard scanner
// Tests the Phase 3 LOCI library (section H, requires real LOCI hardware):
//   H:   LOCI detection, firmware version, directory listing, IJK joystick
//
// Overlay RAM (cwin_push/cwin_pop) is NOT tested here — requires LOCI.

#include "oric.h"
#include "charwin.h"
#include "keyboard.h"
#include "loci.h"
#include "strings_demo.h"

// Hex digit table for key-echo display
static const char hexdig[16] = "0123456789ABCDEF";

static void hex2(char *buf, uint8_t v)
{
    buf[0] = hexdig[v >> 4];
    buf[1] = hexdig[v & 0x0F];
}

int main(void)
{
    charwin_init();

    // ─────────────────────────────────────────────────────────────────────────
    // Full-screen background: white ink, black paper
    // ─────────────────────────────────────────────────────────────────────────

    OricCharWin scr;
    cwin_init(&scr, 2, 0, 38, 28, A_FWWHITE, A_BGBLACK);
    cwin_clear(&scr);

    cwin_putat_string(&scr, 1, 0, MSG_DEMO_TITLE);
    cwin_putat_string(&scr, 1, 1, MSG_DEMO_SUBTITLE);

    // ─────────────────────────────────────────────────────────────────────────
    // Status window: green ink — shows build component status
    // ─────────────────────────────────────────────────────────────────────────

    OricCharWin status;
    cwin_init(&status, 2, 3, 38, 5, A_FWGREEN, A_BGBLACK);
    cwin_clear(&status);
    cwin_putat_string(&status, 0, 0, MSG_DEMO_STATUS_CRT);
    cwin_putat_string(&status, 0, 1, MSG_DEMO_STATUS_CHARWIN);
    cwin_putat_string(&status, 0, 2, MSG_DEMO_STATUS_KEYB);
    cwin_putat_string(&status, 0, 3, MSG_DEMO_STATUS_ORAM);
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
    // F. Scroll Speed Benchmark (500 scroll_up calls)
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_F);

    OricCharWin bench;
    cwin_init(&bench, 2, 2, 36, 10, A_FWYELLOW, A_BGBLACK);
    cwin_clear(&bench);

    for (uint8_t i = 0; i < 10; i++)
        cwin_putat_printf(&bench, 0, i, "L%02u Seed line", (uint16_t)i);

    for (uint16_t f = 0; f < 500; f++)
    {
        cwin_scroll_up(&bench);
        cwin_putat_printf(&bench, 0, 9, "S%u scroll done", f);
    }

    cwin_putat_string(&scr, 0, 13, MSG_DEMO_SCROLL_DONE);
    cwin_putat_string(&scr, 0, 14, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // G. Bouncing Character Animation (2000 frames)
    //
    // Head: CH_INVSPACE (solid magenta block).
    // Trail: 'O' -> 'o' -> '.' representing diminishing density.
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_G);

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
    // H. LOCI Library Demo
    //
    // Requires real LOCI hardware. In Oricutron (no LOCI), shows "Not found"
    // and stops gracefully — no hang, no crash.
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 0, 0, MSG_DEMO_SECTION_H);

    OricCharWin lwin;
    cwin_init(&lwin, 2, 2, 38, 24, A_FWWHITE, A_BGBLACK);
    cwin_clear(&lwin);

    // Step 1: detection
    if (!loci_present())
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

    // Step 2: firmware version and CPU speed
    {
        int16_t cpu_khz = phi2();
        loci_uname(&locicfg.uname);
        // Parse version from release string
        const char *rel = locicfg.uname.release;
        locicfg.version.major = (uint8_t)(rel[0] - '0');
        locicfg.version.minor = (uint8_t)(rel[2] - '0');
        locicfg.version.patch = (uint8_t)(rel[4] - '0');

        cwin_putat_printf(&lwin, 0, 1, MSG_DEMO_LOCI_FW_VER "%u.%u.%u",
            (uint16_t)locicfg.version.major,
            (uint16_t)locicfg.version.minor,
            (uint16_t)locicfg.version.patch);

        cwin_putat_printf(&lwin, 0, 2, MSG_DEMO_LOCI_CPU_KHZ "%u" MSG_DEMO_LOCI_KHZ_UNIT,
            (uint16_t)cpu_khz);
    }

    // Step 3: root directory listing (first 10 entries)
    {
        cwin_putat_string(&lwin, 0, 4, MSG_DEMO_LOCI_DIR_HDR);

        LociDir    *dir  = loci_opendir("");
        LociDirent *ent;
        uint8_t     row  = 5;
        uint8_t     devnr = 0;

        while (row < 18)
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

            // Show: "[DIR] name" or "[FIL] name"
            cwin_putat_printf(&lwin, 0, row, "%s %s", typ, ent->d_name);
            row++;
        }
        loci_closedir(dir);

        cwin_putat_printf(&lwin, 0, 19, MSG_DEMO_LOCI_DEV_COUNT "%u", (uint16_t)devnr);
    }

    // Step 4: IJK joystick detection
    ijk_detect();
    cwin_putat_string(&lwin, 0, 21,
        ijk_present ? MSG_DEMO_LOCI_IJK_FOUND : MSG_DEMO_LOCI_IJK_NONE);

    // Step 5: done
    cwin_putat_string(&lwin, 0, 23, MSG_DEMO_PRESS_KEY);
    cwin_getch();

    // ─────────────────────────────────────────────────────────────────────────
    // Done
    // ─────────────────────────────────────────────────────────────────────────

    cwin_clear(&scr);
    cwin_putat_string(&scr, 4, 13, MSG_DEMO_DONE);

    while (1) {}
    return 0;
}
