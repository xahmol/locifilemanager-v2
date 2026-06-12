// main.c - locifilemanager v2 application entry point
//
// Based on v1 locifilemanager main.c by Xander Mol, 2025 — local reference at
// locifilemanager/src/main.c.
//
// Adapted: stdint types; fm_getkey() (input.h) replaces v1's getkey(ijk_present)
// — IJK joystick decoding happens inside fm_getkey(). No `done` flag: boot()
// either reboots via mia_call_boot(MIA_OP_BOOT) (never returns) or returns
// on failure, in which case a popup is shown and the for(;;) loop continues.
// mia_call_boot() clears VIA.ifr and disables VIA.ier right before the boot
// jump (see its comment in loci.c) — v2 runs permanently under SEI with no
// IRQ handler, so a stale unacknowledged Timer 1 IFR flag could otherwise
// fire immediately when the booted ROM's cold-start executes CLI.
// help()/versioninfo() use menu_popup_open/menu_popup_close + OricCharWin
// instead of v1's windowsave/windowrestore + cputsxy/cprintf; the help table
// uses fixed-column cwin_putat_string calls because _cwin_vformat does not
// support "%-10s" left-justified width padding.

#include <stdio.h>
#include <string.h>
#include "oric.h"
#include "charwin.h"
#include "keyboard.h"
#include "loci.h"
#include "ijk.h"
#include "strings.h"
#include "menu.h"
#include "input.h"
#include "dir.h"
#include "drive.h"
#include "file.h"

// Help text — key, description (see strings_en/fr.h MSG_HELP_TABLE)
static const char * const helpinfo[22][2] = { MSG_HELP_TABLE };

// -------------------------------------------------------------------------
// App pulldown dynamic entries
// -------------------------------------------------------------------------

static void confirm_toggle(void)
// Toggle confirm once or all
{
    confirm = !confirm;
    sprintf(pulldown_titles[0][0], MSG_MENU_APP_CONFIRM_FMT,
            confirm ? MSG_MENU_VAL_ALL : MSG_MENU_VAL_ONCE);
}

static void select_enter_choice(void)
// Select what to do on enter
{
    static const char * const vals[3] = { MSG_MENU_VAL_SELECT, MSG_MENU_VAL_ENTER, MSG_MENU_VAL_LAUNCH };
    uint8_t select = menu_option_select(MSG_MAIN_ENTER_PROMPT, 5);

    if (select)
    {
        enterchoice = select - 1;
        sprintf(pulldown_titles[0][1], MSG_MENU_APP_RETURN_FMT, vals[enterchoice]);
    }
}

static void select_filter(void)
// Select which filter to apply
{
    static const char * const vals[5] = { MSG_MENU_VAL_NONE, MSG_MENU_VAL_DSK, MSG_MENU_VAL_TAP, MSG_MENU_VAL_ROM, MSG_MENU_VAL_LCE };
    uint8_t select = menu_option_select(MSG_MAIN_FILTER_PROMPT, 6);

    if (select)
    {
        filter = select - 1;
        sprintf(pulldown_titles[0][2], MSG_MENU_APP_FILTER_FMT, vals[filter]);
        dir_draw(0, 1);
        dir_draw(1, 1);
    }
}

// -------------------------------------------------------------------------
// Info popups
// -------------------------------------------------------------------------

static void versioninfo(void)
// Show version information and credits
{
    OricCharWin win;
    char buf[40];

    menu_popup_open(0, 5, 15);
    cwin_init(&win, 2, 5, 38, 15, A_FWBLACK, A_BGWHITE);

    cwin_putat_string(&win, 0, 1, MSG_VERSION_TITLE);
    cwin_putat_string(&win, 0, 3, MSG_MENU_HEADER);
    cwin_putat_string(&win, 0, 4, MSG_VERSION_AUTHOR);

    sprintf(buf, MSG_VERSION_FMT, (uint16_t)VERSION_MAJOR, (uint16_t)VERSION_MINOR, (uint16_t)VERSION_PATCH, __DATE__, __TIME__);
    cwin_putat_string(&win, 0, 6, buf);

    cwin_putat_string(&win, 0, 8, MSG_VERSION_SOURCE);
    cwin_putat_string(&win, 0, 9, MSG_VERSION_URL);
    cwin_putat_string(&win, 0, 11, MSG_VERSION_COPYRIGHT);
    cwin_putat_string(&win, 0, 13, MSG_MAIN_PRESS_CONTINUE);

    cwin_getch();
    menu_popup_close();
}

static void help(void)
// Show keyboard shortcuts help information
{
    OricCharWin win;
    uint8_t item, y;

    menu_popup_open(0, 1, 27);
    cwin_init(&win, 2, 1, 38, 27, A_FWBLACK, A_BGWHITE);

    y = 1;
    cwin_putat_string(&win, 0, y++, MSG_HELP_TITLE1);
    for (item = 0; item < 20; item++)
    {
        cwin_putat_string(&win, 0, y, helpinfo[item][0]);
        cwin_putat_char(&win, 10, y, ':');
        cwin_putat_string(&win, 12, y, helpinfo[item][1]);
        y++;
    }

    cwin_putat_string(&win, 0, y++, MSG_HELP_TITLE2);
    for (item = 20; item < 22; item++)
    {
        cwin_putat_string(&win, 0, y, helpinfo[item][0]);
        cwin_putat_char(&win, 10, y, ':');
        cwin_putat_string(&win, 12, y, helpinfo[item][1]);
        y++;
    }

    y++;
    cwin_putat_string(&win, 0, y, MSG_MAIN_PRESS_CONTINUE);

    cwin_getch();
    menu_popup_close();
}

// -------------------------------------------------------------------------
// Boot
// -------------------------------------------------------------------------

static void boot(void)
// Boot from active mounts (disk > tape > ROM). On success this never
// returns (LOCI reboots the machine); on failure shows an error popup.
{
    // Enable tape auto load if no disk mounted on A takes preference
    if (!mount_filename[0][0] && mount_filename[4][0])
        ald_on = 1;

    if (mia_call_boot((uint8_t)(0x80 | (ald_on << 4) | (bit_on << 3) | (b11_on << 2) | (tap_on << 1) | fdc_on)) < 0)
        menu_messagepopup(MSG_MAIN_BOOT_FAILED);
}

// -------------------------------------------------------------------------
// Main menu loop
// -------------------------------------------------------------------------

static void mainmenuloop(void)
// Go to main menu and act on selection made
{
    uint8_t choice;

    do
    {
        dir_refresh_present();

        choice = menu_main();

        switch (choice)
        {
        case 11:
            confirm_toggle();
            break;

        case 12:
            select_enter_choice();
            break;

        case 13:
            select_filter();
            break;

        case 14:
            dir_togglesort();
            break;

        case 15:
            if (menu_areyousure(MSG_MAIN_EXIT_Q) == 1)
            {
                boot();
                choice = 99;
            }
            break;

        case 21:
            dir_select_toggle();
            break;

        case 22:
            dir_select_all(1);
            break;

        case 23:
            dir_select_all(0);
            break;

        case 24:
            dir_select_inverse();
            break;

        case 25:
            // If no selection made and cursor is on a dir: delete dir
            if (!selection[activepane] && presentdirelement.meta.type == 1)
                dir_deletedir();
            else
                file_delete();
            break;

        case 26:
            file_rename();
            break;

        case 27:
            file_copy_move_selected(0);
            break;

        case 28:
            file_copy_move_selected(1);
            break;

        case 29:
            file_browse_tape();
            break;

        case 31:
            dir_gotoroot();
            break;

        case 32:
            dir_parentdir();
            break;

        case 33:
            dir_pagedown();
            break;

        case 34:
            dir_pageup();
            break;

        case 35:
            dir_top();
            break;

        case 36:
            dir_bottom();
            break;

        case 37:
            dir_newdir();
            break;

        case 41:
            dir_switch_pane();
            break;

        case 42:
            dir_get_next_drive(activepane);
            break;

        case 43:
            dir_get_prev_drive(activepane);
            break;

        case 44:
            drive_mount();
            break;

        case 45:
            drive_unmount();
            break;

        case 46:
            drive_targetdrive();
            break;

        case 47:
            drive_showmounts();
            break;

        case 51:
            versioninfo();
            break;

        case 52:
            help();
            break;

        default:
            break;
        }
    } while (choice < 99);
}

// -------------------------------------------------------------------------
// Application entry point
// -------------------------------------------------------------------------

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

    // Reset application state
    activepane    = 0;
    filter        = 0;
    enterchoice   = 0;
    confirm       = 0;
    sort          = 0;
    targetdrive   = 0;
    selection[0]  = 0;
    selection[1]  = 0;
    insidetape[0] = 0;
    insidetape[1] = 0;
    fdc_on = 0;
    tap_on = 0;
    b11_on = 0;
    bit_on = 0;
    ald_on = 0;

    // Populate dynamic App pulldown entries (initial state)
    sprintf(pulldown_titles[0][0], MSG_MENU_APP_CONFIRM_FMT, MSG_MENU_VAL_ONCE);
    sprintf(pulldown_titles[0][1], MSG_MENU_APP_RETURN_FMT,  MSG_MENU_VAL_SELECT);
    sprintf(pulldown_titles[0][2], MSG_MENU_APP_FILTER_FMT,  MSG_MENU_VAL_NONE);
    sprintf(pulldown_titles[0][3], MSG_MENU_APP_SORT_FMT,    MSG_MENU_VAL_OFF);
    sprintf(pulldown_titles[3][5], MSG_MENU_MNT_TARGET_FMT,  MSG_MENU_DRV0);

    get_locicfg();

    // Unmount all drives, tape and ROM to ensure a known state
    drive_unmount_all();

    menu_placetop(MSG_MENU_HEADER);

    // Show firmware version top-right of header row
    {
        OricCharWin header;
        uint8_t len;

        sprintf(pathbuffer, MSG_MAIN_FW_FMT, locicfg.uname.sysname, locicfg.uname.release);
        len = (uint8_t)strlen(pathbuffer);
        if (len > 20)
        {
            pathbuffer[20] = 0;
            len = 20;
        }
        cwin_init(&header, 2, 0, 38, 1, A_FWBLACK, A_BGGREEN);
        cwin_putat_string(&header, 37 - len, 0, pathbuffer);
    }

    // Set start dirs and print
    strcpy(presentdir[0].path, "0:/");
    presentdir[0].drive = 0;
    dir_draw(0, 1);
    presentdir[1].drive = 0;
    dir_get_next_drive(1);

    // Main loop
    for (;;)
    {
        dir_refresh_present();

        switch (fm_getkey())
        {
        case KEY_RIGHT:
            // Go to menu
            mainmenuloop();
            break;

        case KEY_ENTER:
            // Enter: Enter directory or perform selected action on file
            if (presentdir[activepane].firstelement && presentdirelement.meta.type == 1)
            {
                dir_build_path(pathbuffer, sizeof(pathbuffer), presentdir[activepane].path, presentdirelement.name);
                strncpy(presentdir[activepane].path, pathbuffer, sizeof(presentdir[activepane].path));
                dir_draw(activepane, 1);
            }
            else
            {
                switch (enterchoice)
                {
                case 0:
                    if (insidetape[activepane])
                        drive_mount();
                    else
                        dir_select_toggle();
                    break;

                case 1:
                    drive_mount();
                    break;

                case 2:
                    drive_unmount_all();
                    drive_mount();
                    boot();
                    break;

                default:
                    break;
                }
            }
            break;

        case KEY_ESC:
            // ESC: Application exit
            if (menu_areyousure(MSG_MAIN_EXIT_Q) == 1)
                boot();
            break;

        case '.':
            // .: Next drive for active pane
            dir_get_next_drive(activepane);
            break;

        case ',':
            // ,: Previous drive for active pane
            dir_get_prev_drive(activepane);
            break;

        case '/':
            // /: Switch pane
            dir_switch_pane();
            break;

        case '\\':
            // \: Go to root
            dir_gotoroot();
            break;

        case KEY_LEFT:
            // Curs Left: Go to parent directory
            dir_parentdir();
            break;

        case KEY_DOWN:
            // Curs Down: Scroll down
            dir_go_down();
            break;

        case KEY_UP:
            // Curs Up: Scroll up
            dir_go_up();
            break;

        case 'd':
            dir_pagedown();
            break;

        case 'p':
            dir_pageup();
            break;

        case 't':
            dir_top();
            break;

        case 'b':
            dir_bottom();
            break;

        case 's':
            dir_select_toggle();
            break;

        case 'a':
            dir_select_all(1);
            break;

        case 'n':
            dir_select_all(0);
            break;

        case 'i':
            dir_select_inverse();
            break;

        case 'o':
            dir_togglesort();
            break;

        case 'c':
            file_copy_move_selected(0);
            break;

        case 'v':
            file_copy_move_selected(1);
            break;

        case 'f':
            select_filter();
            break;

        case 'g':
            drive_targetdrive();
            break;

        case 'r':
            file_rename();
            break;

        case 'm':
            drive_mount();
            break;

        case 'u':
            drive_unmount();
            break;

        case 'w':
            file_browse_tape();
            break;

        case 'e':
            dir_newdir();
            break;

        case 'h':
            help();
            break;

        case KEY_DEL:
            // Del: Delete files or dir
            if (!selection[activepane] && presentdirelement.meta.type == 1)
                dir_deletedir();
            else
                file_delete();
            break;

        default:
            break;
        }
    }
}
