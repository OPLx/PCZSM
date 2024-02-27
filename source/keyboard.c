/** @file keyboard.c
 *
 * @brief Simple keyboard handling.
 * 
 */

#include <stdint.h>
#include <stdbool.h>
#include <i86.h>
#include <string.h>

#define INT16H 0x16

/*!
 * @brief Checks if the key represented by the scan code is pressed.
 * 
 * @param[in] scan_code  Scan code representing the key that would be pressed.
 * 
 * @return true if the key is pressed, otherwise false.
 */
bool const
keyboard_get_state_pcxt_bios(uint8_t const scan_code)
{
    union REGPACK regs;

    memset(&regs, 0, sizeof(union REGPACK));
    regs.h.ah = 0x01;

    intr(INT16H, &regs);

    if ((regs.x.flags & INTR_ZF) == 0)
    {
        memset(&regs, 0, sizeof(union REGPACK));
        regs.h.ah = 0x00;

        intr(INT16H, &regs);

        return scan_code == regs.h.ah;
    }

    return false;
} /* keyboard_get_state_pcxt_bios() */

/*** end of file ***/
