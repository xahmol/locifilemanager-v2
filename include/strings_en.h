// strings_en.h - Main application English user-visible strings
// All MSG_* macros for the main locifilemanager application (English).
// Phase 3: placeholder — MSG_* macros will be populated in Phase 4.
// Do not use MSG_DEMO_* macros from here; use strings_demo_en.h instead.

#ifndef STRINGS_EN_H
#define STRINGS_EN_H

// ── Phase 3: LOCI library error / status messages used in get_locicfg() ──────
#define MSG_LOCI_NOT_FOUND  "No LOCI device detected or firmware too old."
#define MSG_PRESS_KEY_EXIT  "Press any key to exit."

// ── Phase 4+: menu system strings ─────────────────────────────────────────────

// Application header / window title
#define MSG_MENU_HEADER         "LOCI File Manager"

// Menu bar titles
#define MSG_MENU_BAR_APP        "App"
#define MSG_MENU_BAR_FILE       "File"
#define MSG_MENU_BAR_DIR        "Dir"
#define MSG_MENU_BAR_MOUNTS     "Mounts"
#define MSG_MENU_BAR_INFO       "Info"

// Pulldown 0 — App (items 0-3 overwritten at runtime via snprintf + _FMT macros)
#define MSG_MENU_APP0           "Confirm:  Once"
#define MSG_MENU_APP1           "Return:   Select"
#define MSG_MENU_APP2           "[F]ilter: None"
#define MSG_MENU_APP3           "S[O]rt:   Off"
#define MSG_MENU_APP4           "[ESC] Exit"

// Pulldown 1 — File
#define MSG_MENU_FILE0          "[S]elect toggle"
#define MSG_MENU_FILE1          "Select [A]ll"
#define MSG_MENU_FILE2          "Select [N]one"
#define MSG_MENU_FILE3          "[I]nverse select"
#define MSG_MENU_FILE4          "[DEL]ete"
#define MSG_MENU_FILE5          "[R]ename"
#define MSG_MENU_FILE6          "[C]opy"
#define MSG_MENU_FILE7          "Mo[v]e"
#define MSG_MENU_FILE8          "Bro[W]se tape"

// Pulldown 2 — Dir
#define MSG_MENU_DIR0           "[\\] Go to root"
#define MSG_MENU_DIR1           "[<] Back"
#define MSG_MENU_DIR2           "Page [D]own"
#define MSG_MENU_DIR3           "Page U[P]"
#define MSG_MENU_DIR4           "[T]op"
#define MSG_MENU_DIR5           "[B]ottom"
#define MSG_MENU_DIR6           "N[e]w dir"

// Pulldown 3 — Mounts (item 5 overwritten at runtime)
#define MSG_MENU_MNT0           "[/] Switch pane"
#define MSG_MENU_MNT1           "[.] Next drive"
#define MSG_MENU_MNT2           "[,] Prev drive"
#define MSG_MENU_MNT3           "[M]ount"
#define MSG_MENU_MNT4           "[U]nmount"
#define MSG_MENU_MNT5           "Tar[G]et: A"
#define MSG_MENU_MNT6           "Show mounts"

// Pulldown 4 — Info
#define MSG_MENU_INFO0          "Version/credits"
#define MSG_MENU_INFO1          "Help"

// Pulldown 5 — Enter-action sub-menu
#define MSG_MENU_ENT0           "Select"
#define MSG_MENU_ENT1           "Mount"
#define MSG_MENU_ENT2           "Launch"

// Pulldown 6 — Filter sub-menu
#define MSG_MENU_FLT0           "None"
#define MSG_MENU_FLT1           ".DSK"
#define MSG_MENU_FLT2           ".TAP"
#define MSG_MENU_FLT3           ".ROM"
#define MSG_MENU_FLT4           ".LCE"

// Pulldown 7 — Target drive A-D
#define MSG_MENU_DRV0           "A"
#define MSG_MENU_DRV1           "B"
#define MSG_MENU_DRV2           "C"
#define MSG_MENU_DRV3           "D"

// Pulldown 8 — Mount source (A/B/C/D/Tape/ROM/None)
#define MSG_MENU_SRC0           "A"
#define MSG_MENU_SRC1           "B"
#define MSG_MENU_SRC2           "C"
#define MSG_MENU_SRC3           "D"
#define MSG_MENU_SRC4           "Tape"
#define MSG_MENU_SRC5           "ROM"
#define MSG_MENU_SRC6           "None"

// Pulldown 9 — Yes/No
#define MSG_MENU_YN0            "Yes"
#define MSG_MENU_YN1            "No"

// Format macros for dynamic App pulldown entries (snprintf label + value → 16 chars max)
#define MSG_MENU_APP_CONFIRM_FMT    "Confirm: %s"
#define MSG_MENU_APP_RETURN_FMT     "Return: %s"
#define MSG_MENU_APP_FILTER_FMT     "[F]ilter: %s"
#define MSG_MENU_APP_SORT_FMT       "S[O]rt: %s"
#define MSG_MENU_MNT_TARGET_FMT     "Tar[G]et: %s"

// Value strings for dynamic entries (padded so total fits in 16 chars)
#define MSG_MENU_VAL_ONCE       "Once"
#define MSG_MENU_VAL_ALL        "All"
#define MSG_MENU_VAL_SELECT     "Select"
#define MSG_MENU_VAL_ENTER      "Enter"
#define MSG_MENU_VAL_LAUNCH     "Launch"
#define MSG_MENU_VAL_OFF        "Off"
#define MSG_MENU_VAL_NAME       "Name"
#define MSG_MENU_VAL_NONE       "None"
#define MSG_MENU_VAL_DSK        ".DSK"
#define MSG_MENU_VAL_TAP        ".TAP"
#define MSG_MENU_VAL_ROM        ".ROM"
#define MSG_MENU_VAL_LCE        ".LCE"

// Popup dialog strings
#define MSG_MENU_AREYOUSURE     "Are you sure?"
#define MSG_MENU_PRESSAKEY      "Press a key."
#define MSG_MENU_FILE_ERROR     "File error!"
#define MSG_MENU_ERR_LABEL      "Error# : "
#define MSG_MENU_SELECT_OPT     "Select option:"

#endif
