// drive.c - LOCI drive enumeration / mount / unmount for locifilemanager v2
// Based on v1 locifilemanager drive.c by Xander Mol, 2025 — local reference at
// locifilemanager/src/drive.c. See drive.h for adaptation notes.

#include <string.h>
#include <stdio.h>
#include "oric.h"
#include "charwin.h"
#include "loci.h"
#include "menu.h"
#include "dir.h"
#include "strings.h"
#include "drive.h"

// -------------------------------------------------------------------------
// Module state (storage for drive.h externs)
// -------------------------------------------------------------------------

char mount_filename[6][21];

uint8_t fdc_on;
uint8_t tap_on;
uint8_t b11_on;
uint8_t bit_on;
uint8_t ald_on;

static char drivebuffer[40];

// -------------------------------------------------------------------------
// Drive selection / mounting
// -------------------------------------------------------------------------

/**
 * Show the drive-letter selection popup (menu_option_select) and, if the
 * user picks a drive, set the global targetdrive to that slot (0-3 for
 * A-D) and update the Mount pulldown's "target" menu-item label to show
 * the newly selected drive letter.
 *
 * @return (none) -- result is written to the global targetdrive and to
 *         pulldown_titles[3][5].
 */
void drive_targetdrive(void)
{
    uint8_t select = menu_option_select(MSG_DRIVE_SELECT_TARGET, 8);

    if (select)
    {
        targetdrive = select - 1;
        sprintf(pulldown_titles[3][5], MSG_MENU_MNT_TARGET_FMT, pulldown_titles[7][targetdrive]);
    }
}

/**
 * Unmount every drive slot (A-D disk drives, tape deck, ROM box) via
 * loci_umount(), clear all mount_filename[] entries, and reset the
 * fdc_on/tap_on/b11_on/bit_on/ald_on status flags to 0. If the active pane
 * is currently browsing inside a tape image, exits tape-browse mode and
 * redraws that pane.
 *
 * @return (none) -- effects are written to the global mount state.
 */
void drive_unmount_all(void)
{
    uint8_t drive;

    if (insidetape[activepane])
    {
        insidetape[activepane] = 0;
        dir_draw(activepane, 1);
    }

    for (drive = 0; drive < 6; drive++)
    {
        loci_umount(drive);
        mount_filename[drive][0] = 0;
    }

    fdc_on = 0;
    tap_on = 0;
    b11_on = 0;
    bit_on = 0;
    ald_on = 0;
}

/**
 * Open a popup window listing the filenames currently mounted on drives
 * A-D, the tape deck and the ROM box (from mount_filename[]), and wait for
 * any key before closing it.
 *
 * @return (none)
 */
void drive_showmounts(void)
{
    OricCharWin popup;
    uint8_t drive;

    menu_popup_open(2, 8, 12);
    cwin_init(&popup, 4, 8, 36, 12, A_FWBLACK, A_BGWHITE);

    cwin_putat_string(&popup, 0, 1, MSG_DRIVE_PRESENT_MOUNTS);

    for (drive = 0; drive < 4; drive++)
        cwin_putat_printf(&popup, 0, 3 + drive, MSG_DRIVE_LBL_DRIVE, 'A' + drive, mount_filename[drive]);

    cwin_putat_printf(&popup, 0, 7, MSG_DRIVE_LBL_TAPE, mount_filename[4]);
    cwin_putat_printf(&popup, 0, 8, MSG_DRIVE_LBL_ROM, mount_filename[5]);

    cwin_putat_string(&popup, 0, 10, MSG_MENU_PRESSAKEY);
    cwin_getch();

    menu_popup_close();
}

/**
 * Act on the currently-selected entry in the active pane: if it is a
 * mountable image (.DSK -> targetdrive, .TAP -> tape deck, .ROM -> ROM box),
 * mount it via loci_mount(), record its name in mount_filename[] and set
 * the corresponding fdc_on/tap_on/b11_on status flag, then show a result
 * popup. If the active pane is browsing inside a tape image instead, seek
 * the tape to the offset stored in the selected entry's name field (via
 * tap_seek()) and show a confirmation popup.
 *
 * @return (none) -- effects are written to global mount state and shown via
 *         popups.
 */
void drive_mount(void)
{
    if (presentdir[activepane].firstelement && presentdirelement.meta.type < 5)
    {
        strcpy(drivebuffer, MSG_DRIVE_MOUNT_FAIL);

        if (presentdirelement.meta.type == 2)
        {
            // .DSK file -> mount on the selected target drive
            if (loci_mount(targetdrive, presentdir[activepane].path, presentdirelement.name) >= 0)
            {
                strncpy(mount_filename[targetdrive], presentdirelement.name, 20);
                mount_filename[targetdrive][20] = 0;
                sprintf(drivebuffer, MSG_DRIVE_MOUNT_DRIVE_FMT, mount_filename[targetdrive], 'A' + targetdrive);
                fdc_on = 1;
            }
        }
        else if (presentdirelement.meta.type == 3)
        {
            // .TAP file -> mount on the tape deck (drive slot 4)
            if (loci_mount(4, presentdir[activepane].path, presentdirelement.name) >= 0)
            {
                strncpy(mount_filename[4], presentdirelement.name, 20);
                mount_filename[4][20] = 0;
                sprintf(drivebuffer, MSG_DRIVE_MOUNT_TAPE_FMT, mount_filename[4]);
                tap_on = 1;
            }
        }
        else if (presentdirelement.meta.type == 4)
        {
            // .ROM file -> mount on the ROM box (drive slot 5)
            if (loci_mount(5, presentdir[activepane].path, presentdirelement.name) >= 0)
            {
                strncpy(mount_filename[5], presentdirelement.name, 20);
                mount_filename[5][20] = 0;
                sprintf(drivebuffer, MSG_DRIVE_MOUNT_ROM_FMT, mount_filename[5]);
                b11_on = 1;
            }
        }

        menu_messagepopup(drivebuffer);
    }

    // Browsing inside a tape: seek to the start of the selected entry
    if (presentdir[activepane].firstelement && insidetape[activepane])
    {
        int32_t counter;

        memcpy(&counter, presentdirelement.name, sizeof(counter));
        tap_seek(counter);
        menu_messagepopup(MSG_DRIVE_TAPE_SEEK);
    }
}

/**
 * Show the unmount-target selection popup (menu_option_select) and, if the
 * user picks a slot (1-6 = A/B/C/D/Tape/ROM), unmount it via loci_umount()
 * and clear its mount_filename[] entry. For A-D, clears fdc_on once no disk
 * drive remains mounted. For Tape, clears tap_on/ald_on and exits
 * tape-browse mode for the active pane. For ROM, clears b11_on.
 *
 * @return (none) -- effects are written to global mount state.
 */
void drive_unmount(void)
{
    uint8_t select;
    uint8_t drive;
    uint8_t mountcount;

    select = menu_option_select(MSG_DRIVE_SELECT_UNMOUNT, 9);

    if (select > 0 && select < 7)
    {
        loci_umount(select - 1);
        mount_filename[select - 1][0] = 0;

        if (select < 5)
        {
            // A/B/C/D: clear FDC-on flag once no disk drive is mounted anymore
            mountcount = 0;
            for (drive = 0; drive < 4; drive++)
                if (mount_filename[drive][0])
                    mountcount++;

            if (!mountcount)
                fdc_on = 0;
        }

        // Pulldown 8 order is A,B,C,D,Tape,ROM,None -> select 5=Tape, 6=ROM
        // (v1 checked select==4/5 here, off by one against its own pulldown).
        if (select == 5)
        {
            tap_on = 0;
            ald_on = 0;
            insidetape[activepane] = 0;
        }
        if (select == 6)
        {
            b11_on = 0;
        }
    }
}
