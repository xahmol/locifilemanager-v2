// main.c - locifilemanager v2 application entry point (Phase 4)
//
// Phase 4: menu system wired up; full file manager dispatch loop (stub).
// Run `make libdemo-run` to launch the Phase 2+3+4 library demo in Oricutron.

#include <stdio.h>
#include "oric.h"
#include "charwin.h"
#include "loci.h"
#include "ijk.h"
#include "strings.h"
#include "menu.h"

int main(void)
{
    charwin_init();

    // LOCI required for overlay RAM save/restore; gracefully absent in Oricutron
    if (!loci_present())
    {
        OricCharWin scr;
        cwin_init(&scr, 2, 0, 38, 28, A_FWWHITE, A_BGBLACK);
        cwin_clear(&scr);
        cwin_putat_string(&scr, 0, 0, MSG_LOCI_NOT_FOUND);
        cwin_putat_string(&scr, 0, 2, MSG_PRESS_KEY_EXIT);
        cwin_getch();
        return 0;
    }

    menu_init();   // also calls ijk_detect()

    // Populate dynamic App pulldown entries (initial state)
    sprintf(pulldown_titles[0][0], MSG_MENU_APP_CONFIRM_FMT, MSG_MENU_VAL_ONCE);
    sprintf(pulldown_titles[0][1], MSG_MENU_APP_RETURN_FMT,  MSG_MENU_VAL_SELECT);
    sprintf(pulldown_titles[0][2], MSG_MENU_APP_FILTER_FMT,  MSG_MENU_VAL_NONE);
    sprintf(pulldown_titles[0][3], MSG_MENU_APP_SORT_FMT,    MSG_MENU_VAL_OFF);
    sprintf(pulldown_titles[3][5], MSG_MENU_MNT_TARGET_FMT,  "A");

    menu_placetop(MSG_MENU_HEADER);

    // Main event loop — Phase 4 stub: echo choice until ESC
    uint8_t choice;
    do {
        choice = menu_main();
        // TODO Phase 4: dispatch choice to dir/file/drive handlers
    } while (choice != 99);

    return 0;
}
