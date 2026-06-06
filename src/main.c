// main.c - locifilemanager v2 application entry point (Phase 3 stub)
//
// Phase 2+3 demo and library tests: src/libdemo.c -> build/libdemo.tap
// Run `make libdemo-run` to launch the demo in Oricutron.
// Phase 4 will replace this stub with the full file manager application.

#include "oric.h"
#include "charwin.h"
#include "loci.h"
#include "strings.h"

int main(void)
{
    charwin_init();

    OricCharWin scr;
    cwin_init(&scr, 2, 0, 38, 28, A_FWWHITE, A_BGBLACK);
    cwin_clear(&scr);

    cwin_putat_string(&scr, 1, 0, "LOCI FILE MANAGER V2");
    cwin_putat_string(&scr, 1, 2, "Phase 3 complete - LOCI library ready");
    cwin_putat_string(&scr, 1, 4, "Run libdemo.tap for Phase 2+3 tests.");

    while (1) {}
    return 0;
}
