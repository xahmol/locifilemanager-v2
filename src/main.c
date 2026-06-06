// main.c - locifilemanager v2 stub (Phase 3+ application entry point)
//
// Phase 2 demo and library tests are in src/libdemo.c → build/libdemo.tap
// Run `make libdemo-run` to launch the demo in Oricutron.

#include "oric.h"
#include "charwin.h"

int main(void)
{
    charwin_init();

    OricCharWin scr;
    cwin_init(&scr, 2, 0, 38, 28, A_FWWHITE, A_BGBLACK);
    cwin_clear(&scr);

    cwin_putat_string(&scr, 1,  0, "LOCI FILE MANAGER V2");
    cwin_putat_string(&scr, 1,  2, "Phase 2 complete - starting Phase 3");
    cwin_putat_string(&scr, 1,  4, "Run libdemo.tap for Phase 2 tests.");

    while (1) {}
    return 0;
}
