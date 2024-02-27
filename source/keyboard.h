/** @file keyboard.h
 *
 * @brief Simple keyboard handling.
 * 
 */

#pragma once

#define KEYBOARD_SCANCODE_ESC 0x01

//--------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------------------------------

bool const keyboard_get_state_pcxt_bios(uint8_t const scan_code);

//--------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//--------------------------------------------------------------------

/*** end of file ***/
