/** @file saaym.h
 *
 * @brief TexElec SAAYM interfacing
 *
 */

#pragma once

//--------------------------------------------------------------------

#define SAAYM_PORT_OFFSET_YM2151 (0x0EU)

typedef struct
{
    e_ym2151_clock_t  ym2151_clock;
    e_saa1099_clock_t saa1099_clock;
    uint16_t          base_io_port;
    uint8_t           irq_number;
} saaym_config_t;

//--------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------------------------------

/*!
 * @brief Detect SAAYM base IO address across its valid range (0x210 through 0x270)
 * 
 * @param[in] b_detect_irq detect IRQ
 *
 * @return The saaym_config_t structure.  If the SAAYM exists, base_io_port will be a non-zero value.
 */
saaym_config_t saaym_detect(bool const b_detect_irq);

//--------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//--------------------------------------------------------------------

/*** end of file ***/
