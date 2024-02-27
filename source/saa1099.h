/** @file saa1099.h
 *
 * @brief VERA PSG to SAA1099 interfacing.
 *
 */

#pragma once

typedef enum
{
    SAA1099_CLOCK_7159090_HZ,
    SAA1099_CLOCK_8000000_HZ,

    SAA1099_CLOCK_COUNT,

    SAA1099_CLOCK_FIRST_VALID = 0,
    SAA1099_CLOCK_LAST_VALID  = SAA1099_CLOCK_COUNT - 1,
    SAA1099_CLOCK_INVALID     = -1
} e_saa1099_clock_t;

//--------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------------------------------

void saa1099_vera_psg_initialize(uint16_t const port_base_address, e_saa1099_clock_t const clock);
void saa1099_vera_psg_terminate(void);
void saa1099_vera_psg_write(uint8_t const address, uint8_t const data);

//--------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//--------------------------------------------------------------------

/*** end of file ***/
