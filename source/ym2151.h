/** @file ym2151.h
 *
 * @brief YM2151 interfacing
 *
 */

#pragma once

#define YM2151_ADDRESS_KON            (0x08U)
#define YM2151_ADDRESS_CLKA_HI        (0x10U)
#define YM2151_ADDRESS_CLKA_LO        (0x11U)
#define YM2151_ADDRESS_CLKB           (0x12U)
#define YM2151_ADDRESS_TIMER          (0x14U)
#define YM2151_LOAD_TIMER_A           (0x01U)
#define YM2151_LOAD_TIMER_B           (0x02U)
#define YM2151_RESET_TIMER_A          (0x10U)
#define YM2151_RESET_TIMER_B          (0x20U)
#define YM2151_IRQ_EN_TIMER_A         (0x04U)
#define YM2151_IRQ_EN_TIMER_B         (0x08U)
#define YM2151_STATUS_TIMER_A_FLAG    (0x01U)
#define YM2151_STATUS_TIMER_B_FLAG    (0x02U)
#define YM2151_STATUS_WRITE_BUSY_FLAG (0x80U)
#define YM2151_CHANNEL_COUNT          (0x08U)

typedef enum
{
    YM2151_CLOCK_3579545_HZ,
    YM2151_CLOCK_4000000_HZ,

    YM2151_CLOCK_COUNT,

    YM2151_CLOCK_FIRST_VALID = 0,
    YM2151_CLOCK_LAST_VALID  = YM2151_CLOCK_COUNT - 1,
    YM2151_CLOCK_INVALID     = -1
} e_ym2151_clock_t;

//--------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------------------------------

void ym2151_initialize(uint16_t const port_base_address, e_ym2151_clock_t const clock);
void ym2151_terminate(void);
void ym2151_write(uint8_t const address, uint8_t const data);
void ym2151_set_timer_b(uint8_t const value);
void ym2151_set_timer_b_hz(uint16_t const rate_in_hertz);
void ym2151_enable_timer_b(bool const b_enable);
uint8_t ym2151_read_status(void);

//--------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//--------------------------------------------------------------------

/*** end of file ***/
