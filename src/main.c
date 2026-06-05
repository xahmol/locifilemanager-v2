// main.c - Phase 2 test demo for Oric Atmos
//
// Tests the complete Phase 2 stack in Oricutron (no LOCI required):
//   - Oscar64 bare-metal buildchain (oric_crt.c, regions, startup)
//   - charwin library (screen I/O, colors, scroll, cursor)
//   - keyboard scanner (direct VIA/AY, no ROM calls)
//   - cwin_textinput (viewport scroll, cursor, ESC/ENTER)
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
    // Done
    // -----------------------------------------------------------------------

    cwin_clear(&scr);
    cwin_putat_string(&scr, 4, 13, "PHASE 2 TEST COMPLETE. HALTED.");

    while (1) {}
    return 0;
}
