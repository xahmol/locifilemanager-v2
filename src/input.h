// input.h - Shared keyboard/joystick input helper for locifilemanager v2
//
// Based on v1 locifilemanager generic.c getkey() by Xander Mol, 2025 — local
// reference at locifilemanager/src/generic.c — and on menu.c's static
// menu_getkey() (same logic, already verified on real LOCI hardware).
//
// Adapted: standalone, exported function for dir/file/drive modules.
// menu.c's menu_getkey() is intentionally NOT reused/refactored — it is
// static and any change to its call graph risks the Oscar64 -O2 whole-program
// register-allocator under-count bug (see project memory
// project_locifm_pulldown_menu_bug.md). fm_getkey() duplicates the same
// keyboard+IJK polling/debounce logic as a separate function instead.

#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

// Blocking: wait for and return the next key. If no keyboard key is
// pressed and an IJK joystick is present, joystick directions/fire are
// decoded to KEY_UP/DOWN/LEFT/RIGHT/KEY_ENTER. Debounces until the
// joystick returns to neutral before returning.
uint8_t fm_getkey(void);

#pragma compile("input.c")

#endif  // INPUT_H
