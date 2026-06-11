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

void drive_targetdrive(void)
// Select which drive is the target for mount operations
{
    uint8_t select = menu_option_select(MSG_DRIVE_SELECT_TARGET, 7);

    if (select)
    {
        targetdrive = select - 1;
        sprintf(pulldown_titles[3][5], MSG_MENU_MNT_TARGET_FMT, pulldown_titles[7][targetdrive]);
    }
}

void drive_unmount_all(void)
// Unmount all images for disk, tape and ROM
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

void drive_showmounts(void)
// Show filenames that are currently mounted
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

void drive_mount(void)
// Mount the selected file (or seek to it, when browsing inside a tape)
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

void drive_unmount(void)
// Unmount selected drive, tape or ROM
{
    uint8_t select;
    uint8_t drive;
    uint8_t mountcount;

    select = menu_option_select(MSG_DRIVE_SELECT_UNMOUNT, 8);

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
