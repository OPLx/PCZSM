/** @file ym2151.c
 *
 * @brief YM2151 interfacing
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <conio.h>
#include "ym2151.h"

#define YM2151_REGISTER_COUNT  (256U)

#define YM_CLOCK_RATIO_3579545 3495 // 3579545 / 1024
#define YM_CLOCK_RATIO_4000000 3906 // 4000000 / 1024

#define YM2151_START_TIMER_B (YM2151_RESET_TIMER_B | YM2151_IRQ_EN_TIMER_B | YM2151_LOAD_TIMER_B)

static uint16_t         g_port_address = 0x22E;
static uint16_t         g_port_data    = 0x22F;
static e_ym2151_clock_t g_ym2151_clock = YM2151_CLOCK_INVALID;

/*!
 * @brief Waits until YM2151 is not busy
 */
static void
ym2151_wait_until_ready(void)
{
    while (inp(g_port_data) & YM2151_STATUS_WRITE_BUSY_FLAG)
    {
        // wait until not busy
    }
} /* ym2151_wait_until_ready() */

/*!
 * @brief Clears all of the YM2151 registers.
 */
static void
ym2151_clear_all(void)
{
    for (uint16_t address = 0; address < YM2151_REGISTER_COUNT; ++address)
    {
        ym2151_write((uint8_t)address, 0);
    }
} /* ym2151_clear_all() */

/*!
 * @brief Initializes YM2151 and other internal states.
 * 
 * @param[in] port_base_address The port base address where the YM2151 resides.
 * @param[in] clock             The clock frequency that the YM2151 is being driven at.
 */
void
ym2151_initialize(uint16_t const port_base_address, e_ym2151_clock_t const clock)
{
    g_port_address = port_base_address;
    g_port_data    = port_base_address + 1;
    g_ym2151_clock = clock;

    switch (clock)
    {
        case YM2151_CLOCK_4000000_HZ:
            break;

        default:
            // 3.579545 MHz clock
            break;
    }

    ym2151_clear_all();
} /* ym2151_initialize() */

/*!
 * @brief Silences YM2151 and clears internal states.
 */
void
ym2151_terminate(void)
{
    for (uint8_t channel = 0; channel < YM2151_CHANNEL_COUNT; ++channel)
    {
        ym2151_write(YM2151_ADDRESS_KON, channel);
    }
} /* ym2151_terminate() */

/*!
 * @brief Writes data to the YM2151 at the specified address.
 *
 * @param[in] address Address of the YM2151 to write to.
 * @param[in] data    Data to write to the specified address.
 */
void
ym2151_write(uint8_t const address, uint8_t const data)
{
    ym2151_wait_until_ready();
    outp(g_port_address, address);

    ym2151_wait_until_ready();
    outp(g_port_data, data);
} /* ym2151_write() */

/*!
 * @brief Sets the YM2151's Timer B value.
 * 
 * @param[in] value The Timer B value.
 */
void
ym2151_set_timer_b(uint8_t const value)
{
    ym2151_write(YM2151_ADDRESS_CLKB, value);  // set Timer B overflow
} /* ym2151_set_timer_b() */

/*!
 * @brief Sets the YM2151's Timer B from a Hertz value.
 *
 * @param[in] rate_in_hertz The timer rate in Hertz.
 */
void
ym2151_set_timer_b_hz(uint16_t const rate_in_hertz)
{
    if (rate_in_hertz > 0)
    {
        uint8_t const clock_ratio = (uint8_t)(((g_ym2151_clock == YM2151_CLOCK_4000000_HZ) ? YM_CLOCK_RATIO_4000000 : YM_CLOCK_RATIO_3579545) / rate_in_hertz);
        uint8_t const value       = (uint8_t)(256 - clock_ratio);

        ym2151_set_timer_b(value);
    }
} /* ym2151_set_timer_b_hz() */

/*!
 * @brief Enable (or disables) the YM2151's Timer B.
 *
 * @param[in] b_enable Enable (true), Disable (false)
 */
void
ym2151_enable_timer_b(bool const b_enable)
{
    ym2151_write(YM2151_ADDRESS_TIMER, b_enable ? YM2151_START_TIMER_B : YM2151_RESET_TIMER_B); // Start/Stop Timer B
} /* ym2151_enable_timer_b() */

/*!
 * @brief Reads the YM2151's status.
 *
 * @return status.
 */
uint8_t ym2151_read_status(void)
{
    return inp(g_port_data);
} /* ym2151_read_status() */

/*** end of file ***/
