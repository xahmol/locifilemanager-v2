// strings_demo_en.h - libdemo English user-visible strings
// All MSG_DEMO_* macros for src/libdemo.c (English).
// Do not use these from main app sources — use strings_en.h instead.

#ifndef STRINGS_DEMO_EN_H
#define STRINGS_DEMO_EN_H

// ── Intro / title bar ─────────────────────────────────────────────────────────
#define MSG_DEMO_TITLE          "LOCI FILE MANAGER V2 - PHASE 2 TEST"
#define MSG_DEMO_SUBTITLE       "Oscar64 bare-metal Oric Atmos build"
#define MSG_DEMO_VERSION        "Version: %u.%u.%u"

// ── Build-status window ───────────────────────────────────────────────────────
#define MSG_DEMO_STATUS_CRT     "[ OK ] oric_crt.c startup + regions"
#define MSG_DEMO_STATUS_CHARWIN "[ OK ] charwin library initialized"
#define MSG_DEMO_STATUS_KEYB    "[ OK ] keyboard scanner (VIA/AY)"
#define MSG_DEMO_STATUS_ORAM    "[ -- ] overlay RAM (requires LOCI)"
#define MSG_DEMO_STATUS_ORAM_OK "[ OK ] overlay RAM (push/pop OK)"
#define MSG_DEMO_STATUS_ORAM_ERR "[ERR] overlay RAM test FAILED"

// ── Text-input section ────────────────────────────────────────────────────────
#define MSG_DEMO_INPUT_PROMPT   "Text input (ENTER=accept, ESC=cancel):"
#define MSG_DEMO_INPUT_ENTERED  "Entered:"
#define MSG_DEMO_INPUT_CANCEL   "ESC: input cancelled"
#define MSG_DEMO_KEY_ECHO_NEXT  "Press any key for key-echo loop..."
#define MSG_DEMO_KEY_ECHO_HDR   "Key echo (hex). ENTER/ESC to halt:"

// ── Color quick-test labels (pre-section-A) ───────────────────────────────────
#define MSG_DEMO_COLOR_TEST     "Color test:"
#define MSG_DEMO_COLOR_RED      "RED   INK"
#define MSG_DEMO_COLOR_CYAN     "CYAN  INK"
#define MSG_DEMO_COLOR_YELLOW   "YELLW INK"

// ── Section headers ───────────────────────────────────────────────────────────
#define MSG_DEMO_SECTION_A      "A: Colour palette (8 ink x 8 paper)"
#define MSG_DEMO_SECTION_A_HDR  "    BLK RED GRN YEL BLU MAG CYN WHT"
#define MSG_DEMO_SECTION_B      "B: Inline attribute macros (ASTR)"
#define MSG_DEMO_SECTION_C      "C: fill rect (concentric borders)"
#define MSG_DEMO_SECTION_D      "D: Animated cursor walk (perimeter)"
#define MSG_DEMO_SECTION_E      "E: getat char read-back test"
#define MSG_DEMO_SECTION_F      "F: Text editing (insert/delete/printline)"
#define MSG_DEMO_SECTION_G      "G: Scroll speed benchmark (500 scrolls)"
#define MSG_DEMO_SECTION_H      "H: Scroll down demo (cwin_scroll_down)"
#define MSG_DEMO_SECTION_I      "I: Bounce ball (ESC/ENTER exits)"
#define MSG_DEMO_SECTION_J      "J: Viewport map scroll (arrows, ESC=exit)"
#define MSG_DEMO_SECTION_K      "K: Menu system demo"
#define MSG_DEMO_SECTION_L      "L: LOCI library demo"

// ── Generic prompts ───────────────────────────────────────────────────────────
#define MSG_DEMO_PRESS_KEY      "Press any key..."

// ── Section E: getat read-back ────────────────────────────────────────────────
#define MSG_DEMO_GETAT_PASS     "GETAT: PASS"
#define MSG_DEMO_GETAT_FAIL     "GETAT: FAIL at col %u"

// ── Section F: text editing ───────────────────────────────────────────────────
#define MSG_DEMO_EDIT_INIT      "Cursor at col 5. Inserting 5 spaces..."
#define MSG_DEMO_EDIT_DELETE    "Deleting 5 chars at col 5..."
#define MSG_DEMO_EDIT_PRINTLINE "cwin_printline: 8 lines with auto-scroll:"
#define MSG_DEMO_EDIT_LINE_FMT  "Printline row %u of 8"

// ── Section G: scroll benchmark ───────────────────────────────────────────────
#define MSG_DEMO_SCROLL_DONE    "500 scrolls: done. (fast!)"

// ── Section H: scroll down ────────────────────────────────────────────────────
#define MSG_DEMO_SCROLLDN_DONE  "60 down-scrolls: done."

// ── Section I: bouncing ball ──────────────────────────────────────────────────
#define MSG_DEMO_FRAME          "Frame: %u"
#define MSG_DEMO_FRAMES_DONE    "Done: %u frames.    "

// ── Section J: viewport ───────────────────────────────────────────────────────
#define MSG_DEMO_VP_ARROWS      "Arrow keys scroll. ESC/ENTER exits."
#define MSG_DEMO_VP_POS         "View x:%u y:%u   "

// ── Section K: menu demo ──────────────────────────────────────────────────────
#define MSG_DEMO_MENU_INTRO     "Navigate menu. ESC = exit."
#define MSG_DEMO_MENU_CHOICE    "Menu choice: %u   "
#define MSG_DEMO_MENU_POPUPS    "Demoing popup helpers..."
#define MSG_DEMO_MENU_POPUP_MSG "This is a message popup."
#define MSG_DEMO_MENU_POPUP_ASK "Delete test file?"
#define MSG_DEMO_MENU_POPUP_RES "Answer: %u (1=Yes 2=No)"
#define MSG_DEMO_MENU_SEL_RES   "Selection: %u (0=cancel)"
#define MSG_DEMO_MENU_ERR_SIM   "Simulated error #42:"

// ── Section L: LOCI demo ──────────────────────────────────────────────────────
#define MSG_DEMO_LOCI_DETECTED  "LOCI: Detected"
#define MSG_DEMO_LOCI_NOT_FOUND "LOCI: Not found - real hardware needed"
#define MSG_DEMO_LOCI_FW_VER    "Firmware: "
#define MSG_DEMO_LOCI_CPU_KHZ   "CPU: "
#define MSG_DEMO_LOCI_KHZ_UNIT  " kHz"
#define MSG_DEMO_LOCI_DEV_COUNT "Devices: "
#define MSG_DEMO_LOCI_DIR_HDR   "Root directory:"
#define MSG_DEMO_LOCI_TYPE_DIR  "[DIR]"
#define MSG_DEMO_LOCI_TYPE_FILE "[FIL]"
#define MSG_DEMO_LOCI_IJK_FOUND "IJK joystick: found"
#define MSG_DEMO_LOCI_IJK_NONE  "IJK joystick: not found"
#define MSG_DEMO_ERROR          "Error: "

// ── Completion ────────────────────────────────────────────────────────────────
#define MSG_DEMO_DONE           "PHASE 2 TEST COMPLETE. HALTED."

#endif
