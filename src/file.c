// file.c - File operations (copy/move/delete/rename/tape browse) for
// locifilemanager v2.
// Based on v1 locifilemanager file.c by Xander Mol, 2025 — local reference at
// locifilemanager/src/file.c. See file.h for adaptation notes.

#include <string.h>
#include "oric.h"
#include "charwin.h"
#include "loci.h"
#include "menu.h"
#include "dir.h"
#include "drive.h"
#include "strings.h"
#include "file.h"

void file_copy_move_selected(uint8_t move)
// Copy or move selected files from active pane to non-active pane
{
    OricCharWin popup;
    uint8_t  confirmed = 0;
    uint8_t  target = !activepane;
    uint8_t  count = 0;
    uint16_t element;
    uint8_t  roomleft = (uint8_t)(255 - strlen(presentdir[target].path));

    if (!strcmp(presentdir[target].path, presentdir[activepane].path))
    {
        menu_messagepopup(MSG_FILE_TARGET_SAME);
        return;
    }

    if (presentdir[activepane].firstelement && !insidetape[activepane])
    {
        menu_popup_open(0, 8, 15);
        cwin_init(&popup, 2, 8, 38, 15, A_FWBLACK, A_BGWHITE);

        // If nothing is selected and cursor pos is not a dir: select single file on cursor pos
        if (!selection[activepane] && presentdirelement.meta.type > 1)
        {
            presentdirelement.meta.select = 1;
            dir_save_element(present);
            selection[activepane]++;
        }

        cwin_putat_printf(&popup, 0, 1, MSG_FILE_COPYMOVE_FMT,
                          move ? MSG_FILE_VAL_MOVE : MSG_FILE_VAL_COPY,
                          selection[activepane]);
        cwin_putat_string(&popup, 0, 2, presentdir[target].path);

        element = presentdir[activepane].firstelement;

        cwin_putat_string(&popup, 0, 4, MSG_FILE_PROCESSING);
        cwin_putat_string(&popup, 0, 7, MSG_FILE_ESC_CANCEL);

        // Loop through all elements to copy/move all selected files
        do
        {
            dir_get_element(element);

            if (presentdirelement.meta.select)
            {
                count++;
                cwin_fill_rect(&popup, 0, 5, 36, 1, CH_SPACE);
                cwin_putat_string(&popup, 0, 5, presentdirelement.name);

                // Check if path gets too long
                if (roomleft < strlen(presentdirelement.name))
                {
                    cwin_putat_string(&popup, 0, 6, MSG_FILE_PATH_TOO_LONG);
                    break;
                }

                // Derive full target path
                strncpy(pathbuffer, presentdir[target].path, sizeof(pathbuffer));
                strncat(pathbuffer, presentdirelement.name, sizeof(pathbuffer) - strlen(pathbuffer) - 1);

                // Check if file exists
                if (file_exists(pathbuffer))
                {
                    if (confirm || !confirmed)
                    {
                        if (!menu_confirm_file(MSG_FILE_OVERWRITE_Q, presentdirelement.name))
                            break;
                        confirmed = 1;
                    }
                    // Delete file if user selects overwrite
                    if (loci_unlink(pathbuffer) < 0)
                    {
                        menu_fileerrormessage();
                        break;
                    }
                }

                // Derive full source path
                strncpy(pathbuffer2, presentdir[activepane].path, sizeof(pathbuffer2));
                strncat(pathbuffer2, presentdirelement.name, sizeof(pathbuffer2) - strlen(pathbuffer2) - 1);

                // Copy file with progress bar on row 14, columns 2..33
                if (file_copy_progress(pathbuffer, pathbuffer2, 2, 14, 32) != 0)
                {
                    menu_fileerrormessage();
                    break;
                }

                // Delete source file if move is selected
                if (move)
                {
                    if (loci_unlink(pathbuffer2) < 0)
                    {
                        menu_fileerrormessage();
                        break;
                    }
                }

                // Check for ESC to cancel
                if (keyb_check() == KEY_ESC)
                    break;
            }
            element = presentdirelement.meta.next;
        } while (element);

        // Show message if no files selected
        if (!count)
            cwin_putat_string(&popup, 0, 5, MSG_FILE_NOTHING_COPY);

        cwin_fill_rect(&popup, 0, 7, 36, 1, CH_SPACE);
        cwin_putat_string(&popup, 0, 7, MSG_MENU_PRESSAKEY);
        cwin_getch();

        menu_popup_close();

        // Draw new dirs
        dir_draw(target, 1);
        if (move)
            dir_draw(activepane, 1);

        // Reset selection
        if (count || selection[activepane])
        {
            dir_select_all(0);
            selection[activepane] = 0;
        }
    }
}

void file_delete(void)
// Delete selected files from active pane
{
    OricCharWin popup;
    uint8_t  confirmed = 0;
    uint8_t  count = 0;
    uint16_t element;

    if (presentdir[activepane].firstelement && !insidetape[activepane])
    {
        menu_popup_open(0, 8, 15);
        cwin_init(&popup, 2, 8, 38, 15, A_FWBLACK, A_BGWHITE);

        // If nothing is selected and cursor pos is not a dir: select single file on cursor pos
        if (!selection[activepane] && presentdirelement.meta.type > 1)
        {
            presentdirelement.meta.select = 1;
            dir_save_element(present);
            selection[activepane]++;
        }

        cwin_putat_printf(&popup, 0, 1, MSG_FILE_DELETE_FMT, selection[activepane]);

        element = presentdir[activepane].firstelement;

        // Loop through all elements to delete all selected files
        do
        {
            dir_get_element(element);

            if (presentdirelement.meta.select)
            {
                count++;
                cwin_fill_rect(&popup, 0, 3, 36, 1, CH_SPACE);
                cwin_putat_string(&popup, 0, 3, presentdirelement.name);

                // Confirm delete
                if (confirm || !confirmed)
                {
                    if (!menu_confirm_file(MSG_FILE_DELETE_Q, presentdirelement.name))
                        break;
                    confirmed = 1;
                }

                // Derive full target path
                strncpy(pathbuffer, presentdir[activepane].path, sizeof(pathbuffer));
                strncat(pathbuffer, presentdirelement.name, sizeof(pathbuffer) - strlen(pathbuffer) - 1);

                if (loci_unlink(pathbuffer) < 0)
                {
                    menu_fileerrormessage();
                    break;
                }
            }
            element = presentdirelement.meta.next;
        } while (element);

        // Show message if no files selected
        if (!count)
            cwin_putat_string(&popup, 0, 3, MSG_FILE_DELETE_NONE);

        cwin_putat_string(&popup, 0, 7, MSG_MENU_PRESSAKEY);
        cwin_getch();

        menu_popup_close();

        // Draw new dir
        dir_draw(activepane, 1);

        // Reset selection
        if (count || selection[activepane])
        {
            dir_select_all(0);
            selection[activepane] = 0;
        }
    }
}

void file_rename(void)
// Rename selected file or directory
{
    OricCharWin popup;
    char input[64];

    if (presentdir[activepane].firstelement && !insidetape[activepane])
    {
        menu_popup_open(0, 8, 15);
        cwin_init(&popup, 2, 8, 38, 15, A_FWBLACK, A_BGWHITE);

        cwin_putat_string(&popup, 0, 1, MSG_FILE_RENAME_TITLE);

        strncpy(input, presentdirelement.name, sizeof(input) - 1);
        input[sizeof(input) - 1] = 0;

        // Is it a directory? Then remove trailing /
        if (presentdirelement.meta.type == 1)
            input[strlen(input) - 1] = 0;

        // Set old name full path
        strncpy(pathbuffer, presentdir[activepane].path, sizeof(pathbuffer));
        strncat(pathbuffer, presentdirelement.name, sizeof(pathbuffer) - strlen(pathbuffer) - 1);

        cwin_putat_string(&popup, 0, 3, MSG_FILE_RENAME_PROMPT);

        // Input new name and check if not cancelled or empty string
        if (cwin_textinput(&popup, 0, 4, 35, input, sizeof(input) - 1, VINPUT_ALL) > 0)
        {
            // Check if name is actually altered
            if (strcmp(input, presentdirelement.name))
            {
                // Set new name full path
                strncpy(pathbuffer2, presentdir[activepane].path, sizeof(pathbuffer2));
                strncat(pathbuffer2, input, sizeof(pathbuffer2) - strlen(pathbuffer2) - 1);

                if (loci_rename(pathbuffer, pathbuffer2) < 0)
                {
                    menu_fileerrormessage();
                    menu_popup_close();
                    return;
                }
                else
                {
                    // Redraw dir: name length can change, shifting all addresses
                    menu_popup_close();
                    dir_draw(activepane, 1);
                    return;
                }
            }
        }
        menu_popup_close();
    }
}

void file_browse_tape(void)
// Browse a .TAP tape archive
{
    if (presentdir[activepane].firstelement && !insidetape[activepane] && presentdirelement.meta.type == 3)
    {
        drive_mount();
        insidetape[activepane] = 1;
        dir_draw(activepane, 1);
    }
}
