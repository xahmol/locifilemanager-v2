// input.c - Shared keyboard/joystick input helper for locifilemanager v2
//
// See input.h for attribution and rationale.

#include "input.h"
#include "keyboard.h"
#include "ijk.h"

/**
 * Block until a key or joystick direction/fire is pressed and return it as
 * a KEY_* code. Polls the keyboard via keyb_poll() and, if an IJK joystick
 * interface is present and no key was pressed, also polls the joystick and
 * maps stick directions/fire to the corresponding KEY_UP/DOWN/LEFT/RIGHT/
 * ENTER codes. After a joystick-derived key, waits for the stick to return
 * to neutral (debounce) before returning.
 *
 * @return KEY_* code of the key or joystick action detected (never KEY_NONE).
 */
uint8_t fm_getkey(void)
{
    uint8_t k;
    do {
        k = keyb_poll();
        if (ijk_present && k == KEY_NONE)
        {
            ijk_read();
            if      (ijk_ljoy & IJK_JOY_UP)    k = KEY_UP;
            else if (ijk_ljoy & IJK_JOY_DOWN)  k = KEY_DOWN;
            else if (ijk_ljoy & IJK_JOY_LEFT)  k = KEY_LEFT;
            else if (ijk_ljoy & IJK_JOY_RIGHT) k = KEY_RIGHT;
            else if (ijk_ljoy & IJK_JOY_FIRE)  k = KEY_ENTER;

            // Debounce: wait for stick to return to neutral before
            // returning the key, mirroring v1 getkey() in generic.c.
            if (k != KEY_NONE)
            {
                do {
                    ijk_read();
                } while (ijk_ljoy || ijk_rjoy);
                for (uint16_t d = 0; d < 1000; d++) keyb_scan();
            }
        }
    } while (k == KEY_NONE);
    return k;
}
