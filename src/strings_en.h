// strings_en.h - Main application English user-visible strings
// All MSG_* macros for the main locifilemanager application (English).
// Do not use MSG_DEMO_* macros from here; use strings_demo_en.h instead.

#ifndef STRINGS_EN_H
#define STRINGS_EN_H

// ── LOCI library error / status messages used in get_locicfg() ───────────────
#define MSG_LOCI_NOT_FOUND  "No LOCI device detected or firmware too old."
#define MSG_PRESS_KEY_EXIT  "Press any key to exit."

// ── Menu system strings ───────────────────────────────────────────────────────

// Application header / window title
#define MSG_MENU_HEADER         "LOCI File Manager"

// Menu bar titles
#define MSG_MENU_BAR_APP        "App"
#define MSG_MENU_BAR_FILE       "File"
#define MSG_MENU_BAR_DIR        "Dir"
#define MSG_MENU_BAR_MOUNTS     "Mounts"
#define MSG_MENU_BAR_INFO       "Info"
#define MSG_MENU_BAR_TOOLS      "Tools"

// Pulldown 0 — App (items 0-3 overwritten at runtime via snprintf + _FMT macros)
#define MSG_MENU_APP0           "Confirm:  Once"
#define MSG_MENU_APP1           "Return:   Select"
#define MSG_MENU_APP2           "[F] Type: None"
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

// Pulldown 10 — Tools
#define MSG_MENU_TOOLS0         "[K] Properties"
#define MSG_MENU_TOOLS1         "[L] Text: Off"
#define MSG_MENU_TOOLS2         "[J] View text"
#define MSG_MENU_TOOLS3         "[Y] Favourites"

// Format macros for dynamic App pulldown entries (snprintf label + value → 16 chars max)
#define MSG_MENU_APP_CONFIRM_FMT    "Confirm: %s"
#define MSG_MENU_APP_RETURN_FMT     "Return: %s"
#define MSG_MENU_APP_FILTER_FMT     "[F] Type: %s"
#define MSG_MENU_APP_SORT_FMT       "S[O]rt: %s"
#define MSG_MENU_MNT_TARGET_FMT     "Tar[G]et: %s"
#define MSG_MENU_TOOLS_TEXT_FMT     "[L] Text: %s"

// Value strings for dynamic entries (padded so total fits in 16 chars)
#define MSG_MENU_VAL_ONCE       "Once"
#define MSG_MENU_VAL_ALL        "All"
#define MSG_MENU_VAL_SELECT     "Select"
#define MSG_MENU_VAL_ENTER      "Enter"
#define MSG_MENU_VAL_LAUNCH     "Launch"
#define MSG_MENU_VAL_OFF        "Off"
#define MSG_MENU_VAL_ON         "On"
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
#define MSG_MENU_CONFIRM_NAME   "Name:"

// ── Dir module strings ────────────────────────────────────────────────────────
#define MSG_DIR_TAPE_PREFIX     "Tape: "
#define MSG_DIR_EMPTY           "Empty directory."
#define MSG_DIR_CREATE_TITLE    "Create directory."
#define MSG_DIR_ENTER_NAME      "Enter dir name:"
#define MSG_DIR_DELETE_Q        "Delete dir?"
#define MSG_DIR_DELETE_RECURSE_Q "Dir not empty. Delete ALL?"
#define MSG_DIR_DELETING        "Deleting:"
#define MSG_DIR_RECURSE_TRUNCATED "Tree too deep, delete incomplete."
#define MSG_DIR_NAMEFILTER_TITLE  "Filter by name."
#define MSG_DIR_NAMEFILTER_CURRENT_FMT "Current: %s"
#define MSG_DIR_NAMEFILTER_PROMPT "Pattern (* ?), empty=off:"

// Properties popup (Tools -> Properties, hotkey K)
#define MSG_PROP_TITLE          "Properties"
#define MSG_PROP_NAME_FMT       "Name: %s"
#define MSG_PROP_TYPE_FMT       "Type: %s"
#define MSG_PROP_PATH_FMT       "Path: %s"
#define MSG_PROP_ATTR_FMT       "Attr: %s"
#define MSG_PROP_SIZE_FMT       "Size: %s %s"
#define MSG_PROP_BYTES          "bytes"
#define MSG_PROP_BYTES_APPROX   "bytes+"
#define MSG_PROP_CALCULATING_FMT "Calculating... %u"
#define MSG_PROP_CANCELLED      "Cancelled."
#define MSG_PROP_TYPE_DIR       "Directory"
#define MSG_PROP_TYPE_DSK       ".DSK - Disk image"
#define MSG_PROP_TYPE_TAP       ".TAP - Tape image"
#define MSG_PROP_TYPE_ROM       ".ROM - ROM image"
#define MSG_PROP_TYPE_LCE       ".LCE - LOCI Executable"
#define MSG_PROP_TYPE_CFG       ".CFG - Config file"
#define MSG_PROP_TYPE_SYS       ".SYS - System file"

// Favourites popup (Tools -> Favourites, hotkey Y)
#define MSG_FAV_TITLE           "Favourites"
#define MSG_FAV_SLOT_FMT        "%u: %s"
#define MSG_FAV_EMPTY           "(empty)"
#define MSG_FAV_HELP            "1-8:Go A:Add D:Del ESC:Close"
#define MSG_FAV_ADD_PROMPT      "Add to slot (1-8):"
#define MSG_FAV_DELETE_PROMPT   "Delete slot (1-8):"

// ── Drive module strings ──────────────────────────────────────────────────────
#define MSG_DRIVE_SELECT_TARGET   "Select target drive."
#define MSG_DRIVE_SELECT_UNMOUNT  "Select what to unmount/eject."
#define MSG_DRIVE_MOUNT_FAIL      "Mount did not succeed."
#define MSG_DRIVE_MOUNT_DRIVE_FMT "%s on %c."
#define MSG_DRIVE_MOUNT_TAPE_FMT  "%s for tape."
#define MSG_DRIVE_MOUNT_ROM_FMT   "%s for ROM."
#define MSG_DRIVE_TAPE_SEEK       "Moved to file on tape."
#define MSG_DRIVE_PRESENT_MOUNTS  "Present mounts:"
#define MSG_DRIVE_LBL_DRIVE       "Drive %c: %s"
#define MSG_DRIVE_LBL_TAPE        "Tape   : %s"
#define MSG_DRIVE_LBL_ROM         "ROM    : %s"

// ── File module strings ───────────────────────────────────────────────────────
#define MSG_FILE_TARGET_SAME    "Target dir same as active dir."
#define MSG_FILE_COPYMOVE_FMT   "%s %u files to:"
#define MSG_FILE_VAL_COPY       "Copy"
#define MSG_FILE_VAL_MOVE       "Move"
#define MSG_FILE_PROCESSING     "Processing file:"
#define MSG_FILE_ESC_CANCEL     "Press ESC to cancel."
#define MSG_FILE_PATH_TOO_LONG  "Path gets too long."
#define MSG_FILE_COPY_CANCELLED "Copy cancelled."
#define MSG_FILE_NOTHING_COPY   "Nothing to copy."
#define MSG_FILE_OVERWRITE_Q    "File exists. Overwrite?"
#define MSG_FILE_DELETE_FMT     "Delete %u files:"
#define MSG_FILE_DELETE_NONE    "No files to delete."
#define MSG_FILE_DELETE_Q       "Delete file?"
#define MSG_FILE_RENAME_TITLE   "Rename file."
#define MSG_FILE_RENAME_PROMPT  "Enter new name:"
#define MSG_FILE_RECURSE_TRUNCATED "Tree too deep, copy incomplete."
#define MSG_FILE_MKDIR_FAILED   "Could not create directory."

// ── Main module strings ───────────────────────────────────────────────────────
#define MSG_MAIN_EXIT_Q         "Exit application."
#define MSG_MAIN_BOOT_FAILED    "Boot failed."
#define MSG_MAIN_FW_FMT         "%s FW %s"
#define MSG_MAIN_ENTER_PROMPT   "Action for enter"
#define MSG_MAIN_FILTER_PROMPT  "Apply filter"
#define MSG_MAIN_PRESS_CONTINUE "Press a key to continue"

// Text file viewer
// Footer prompts: key names default to the window's ink (cyan); embedded
// ASTR_INK_YELLOW/ASTR_INK_CYAN switch to the explanation/key colour. Requires
// oric.h to be included before strings.h (true for viewer.c).
#define MSG_VIEWER_PRESS_KEY_FMT "SPACE" ASTR_INK_YELLOW ": next  " ASTR_INK_CYAN "X" ASTR_INK_YELLOW ": %s  " ASTR_INK_CYAN "ESC" ASTR_INK_YELLOW ": exit"
#define MSG_VIEWER_EOF_FMT      "X" ASTR_INK_YELLOW ": %s view  " ASTR_INK_CYAN "other" ASTR_INK_YELLOW ": exit"
#define MSG_VIEWER_MODE_HEX     "hex"
#define MSG_VIEWER_MODE_TEXT    "text"

// Version/credits popup
#define MSG_VERSION_TITLE       "Version information and credits"
#define MSG_VERSION_AUTHOR      "Written 2025-2026 by Xander Mol"
#define MSG_VERSION_FMT         "Version: v%u.%u.%u - %s"
#define MSG_VERSION_SOURCE      "Source, docs and credits at:"
#define MSG_VERSION_URL         "github.com/xahmol/locifilemanager-v2"
#define MSG_VERSION_COPYRIGHT   "(C) 2025-2026, IDreamtIn8bits.com"
#define MSG_VERSION_QR_TITLE    "Scan QR code for source:"

// Help screen
#define MSG_HELP_TITLE1         "Keyboard shortcuts help information"
#define MSG_HELP_TITLE2         "In main menu and pulldown menus"
#define MSG_HELP_TABLE \
    {"Curs Up/Do", "Move up and down"}, \
    {"Curs Left",  "Go to parent dir"}, \
    {"Curs Right", "Go to menu"}, \
    {"RETURN",     "Select/mount/launch"}, \
    {"ESC",        "Exit application"}, \
    {". / ,",      "Next/prev drive for pane"}, \
    {"/",          "Switch pane"}, \
    {"\\",         "Go to root"}, \
    {"D/P/T/B",    "Page down,up, top/bottom"}, \
    {"S/A/N/I",    "Select toggle/all/none/inv"}, \
    {"O",          "Sort toggle"}, \
    {"F",          "Select filter to apply"}, \
    {"C/V",        "Copy/Move file(s)"}, \
    {"DEL",        "Delete file(s)/dir"}, \
    {"G",          "Select target drive mount"}, \
    {"R",          "Rename file/dir"}, \
    {"M / U",      "Mount/unmount"}, \
    {"W",          "Browse tape"}, \
    {"E",          "Create new directory"}, \
    {"H",          "Show this help screen"}, \
    {"K",          "Properties"}, \
    {"L",          "Filter files by name"}, \
    {"J",          "View text file"}, \
    {"Y",          "Favourites"}, \
    {"Cur/Esc/Rt", "Navigate/cancel/confirm"}

#endif
