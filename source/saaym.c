/** @file saaym.c
 *
 * @brief TexElec SAAYM interfacing
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <conio.h>
#include <i86.h>
#include <dos.h>
#include "ym2151.h"
#include "saa1099.h"
#include "saaym.h"
#include "timer.h"
#include "irq.h"
#include "i8259a.h"

#define SAAYM_BASE_IO_START  (0x200U)
#define SAAYM_BASE_IO_END    (0x270U)
#define SAAYM_BASE_IO_OFFSET (0x10U)

typedef enum
{
    YM_TIMER_STATE_START,
    YM_TIMER_STATE_WAIT_OVERFLOW,
    YM_TIMER_STATE_OVERFLOWED
} ym_timer_state_t;

typedef struct
{
    ym_timer_state_t ym_timer_state;
    uint32_t         counter;
    uint32_t         ticks;
    uint32_t         count_at_overflow;
    e_ym2151_clock_t ym2151_clock;
    uint8_t          interrupt_number;
} saaym_detect_ym2151_clock_t;

static saaym_detect_ym2151_clock_t volatile g_saaym_detect_ym2151_clock;

static inline int saaym_clamp_value(int const min_value, int const value, int const max_value)
{
    return (value < min_value ? min_value : (value > max_value ? max_value : value));
}

static void
saaym_stopwatch_handler(void)
{
    switch (g_saaym_detect_ym2151_clock.ym_timer_state)
    {
        case YM_TIMER_STATE_START:
            {
                g_saaym_detect_ym2151_clock.counter        = 0;
                g_saaym_detect_ym2151_clock.ym_timer_state = YM_TIMER_STATE_WAIT_OVERFLOW;

                ym2151_enable_timer_b(true);
            }
            break;

        case YM_TIMER_STATE_WAIT_OVERFLOW:
            {
                ++g_saaym_detect_ym2151_clock.counter; // increment
            }
            break;

        default:
            // Does nothing
            break;
    }
    ++g_saaym_detect_ym2151_clock.ticks;
}

static void IRQ_FAR_INTERRUPT
saaym_ym2151_timer_handler(void)
{
    switch (g_saaym_detect_ym2151_clock.ym_timer_state)
    {
        case YM_TIMER_STATE_WAIT_OVERFLOW:
            {
                g_saaym_detect_ym2151_clock.ym_timer_state    = YM_TIMER_STATE_OVERFLOWED;
                g_saaym_detect_ym2151_clock.count_at_overflow = g_saaym_detect_ym2151_clock.counter;

                ym2151_enable_timer_b(false);
            }
            break;

        default:
            // Does nothing
            break;
    }

    outp(PIC1_COMMAND, PIC_EOI);

    if (g_saaym_detect_ym2151_clock.interrupt_number == INT10H_IRQ2)
    {
        outp(PIC2_COMMAND, PIC_EOI);
    }
}

/*!
 * @brief Generic detection routine for TexElec SAAYM or Creative Music System/Gameblaster base address at the specified base address.
 *
 * @param[in] base_io_port      The base address to attempt to detect the SAAYM at.
 * @param[in] write_port_offset The write port offset.
 * @param[in] read_port_offset  The read port offset.
 *
 * @retval true TexElec SAAYM or Creative Music System/Gameblaster exists at the specified base address.
 * @retval false TexElec SAAYM or Creative Music System/Gameblaster does not exist at the specified base address.
 */
static bool 
saaym_detect_game_blaster_generic(uint16_t const base_io_port, uint8_t const write_port_offset, uint8_t const read_port_offset)
{
    uint8_t const gb_detect_byte  = 0xAC;
    uint8_t const gb_default_byte = 0x7F;

    if (inp(base_io_port + 0x4) == gb_default_byte)
    {
        outp(base_io_port + write_port_offset, gb_detect_byte);

        if (inp(base_io_port + read_port_offset) == gb_detect_byte)
        {
            return true;
        }
    }

    return false;
} /* saaym_detect_game_blaster_generic() */

/*!
 * @brief Detects TexElec SAAYM's YM2151 at the pre-assigned address.
 * 
 * @note Detection method is similar to Adlib's YM3812 detection method.
 *
 * @retval true TexElec SAAYM's YM2151 exists at the pre-assigned address.
 * @retval false TexElec SAAYM's YM2151 does not exist at the pre-assigned address.
 */
static bool
saaym_detect_ym2151(void)
{
    ym2151_write(YM2151_ADDRESS_TIMER, (YM2151_RESET_TIMER_B | YM2151_RESET_TIMER_A)); // Mask TimerA & TimerB.
    ym2151_write(YM2151_ADDRESS_TIMER, (YM2151_IRQ_EN_TIMER_B | YM2151_IRQ_EN_TIMER_A)); // Reset IRQ.

    uint8_t const status0 = ym2151_read_status(); // Read status.

    ym2151_write(YM2151_ADDRESS_CLKA_HI, 0xFFU); // Set TimerA latch (hi).
    ym2151_write(YM2151_ADDRESS_CLKA_LO, 0x03U); // Set TimerA latch (lo).
    ym2151_write(YM2151_ADDRESS_TIMER, (YM2151_RESET_TIMER_A | YM2151_IRQ_EN_TIMER_A | YM2151_LOAD_TIMER_A)); // Unmask start start TimerA.

    // wait at least 100us for Timer1 overflow
    for (int i = 0; i != 200; i++)
    {
        ym2151_read_status();
    }

    uint8_t const status1 = ym2151_read_status(); // Read status again.

    ym2151_write(YM2151_ADDRESS_TIMER, (YM2151_RESET_TIMER_B | YM2151_RESET_TIMER_A));
    ym2151_write(YM2151_ADDRESS_TIMER, (YM2151_IRQ_EN_TIMER_B | YM2151_IRQ_EN_TIMER_A));

    return ((status0 & 0x03U) == 0x00U) && ((status1 & 0x03U) == 0x01U);
} /* saaym_detect_ym2151 */

/*!
 * @brief Detect SAAYM base address at the specified base address.
 *
 * @param[in] base_io_port The base address to attempt to detect the SAAYM at.
 * 
 * @retval true SAAYM exists at the specified base address.
 * @retval false SAAYM does not exist at the specified base address.
 */
static bool
saaym_detect_base_io_port(uint16_t const base_io_port)
{
    if (saaym_detect_game_blaster_generic(base_io_port, 0x08, 0x0C))
    {
        ym2151_initialize(base_io_port + SAAYM_PORT_OFFSET_YM2151, YM2151_CLOCK_INVALID);

        return saaym_detect_ym2151();
    }

    return false;
} /* saaym_detect_base_io_port() */

/*!
 * @brief Detect SAAYM IRQ number. 
 */
static void
saaym_detect_irq(void)
{
    irq_install_detect_handlers();

    // 1.  Trigger Timer B to discover the IRQ number that the SAAYM is configured to.
    ym2151_set_timer_b(0xFF); // set Timer B overflow to be at ~16.667 ms (at 3.579545 MHz) or 14.848 ms (at 4 MHz).

    ym2151_enable_timer_b(true);

    delay(100); // wait 100 ms to ensure Timer B overflow.

    ym2151_enable_timer_b(false);

    // 2. Find the IRQ number (if any)
    g_saaym_detect_ym2151_clock.interrupt_number = irq_sense();

    // 3. Attempt to ascertain the speed of the YM2151's master clock.
    if (g_saaym_detect_ym2151_clock.interrupt_number != 0)
    {
        _disable();
        _dos_setvect(g_saaym_detect_ym2151_clock.interrupt_number, &saaym_ym2151_timer_handler);
        _enable();

        ym2151_set_timer_b(0x00U); // set Timer B overflow to be at ~73.23 ms (at 3.579545 MHz) or 65.536 ms (at 4 MHz).

        timer_start(0, 1000U, &saaym_stopwatch_handler); // stopwatch_handler will start Timer B

        //delay(1000); // wait 1000 ms for Timer B to overflow

        while (g_saaym_detect_ym2151_clock.ticks < 1000U && g_saaym_detect_ym2151_clock.ym_timer_state != YM_TIMER_STATE_OVERFLOWED)
        {
            // Wait for 1000 ms to pass and YM2151 timer not yet overflowed
        }

        timer_stop();

#ifdef PCZSM_DEBUG
        printf("Timer State[%02d] | Counter[%lu] | Overflow[%lu]\n", g_saaym_detect_ym2151_clock.ym_timer_state, g_saaym_detect_ym2151_clock.counter, g_saaym_detect_ym2151_clock.count_at_overflow);
#endif
    }

    irq_uninstall_detect_handlers();

    g_saaym_detect_ym2151_clock.ym2151_clock = g_saaym_detect_ym2151_clock.count_at_overflow < 68 ? YM2151_CLOCK_4000000_HZ : YM2151_CLOCK_3579545_HZ;
} /* saaym_detect_irq() */

/*!
 * @brief Gets the SAAYM configuration from the SAAYM environment variable.
 * 
 * @return saaym configuration (if environment variable exists)
 */
static saaym_config_t
saaym_get_config_from_environment(void)
{
    saaym_config_t config = { YM2151_CLOCK_INVALID, SAA1099_CLOCK_INVALID, 0, 0 };

    char const * const p_saaym_var = getenv("SAAYM");

    if (p_saaym_var)
    {
        char const * p_token = p_saaym_var;
        while (*p_token)
        {
            char const parameter = *p_token++;

            switch (parameter | 0x20) // make token char lower case
            {
                case 'a':
                    config.base_io_port = (uint16_t)strtoul(p_token, NULL, 16);
                    break;

                case 'i':
                    config.irq_number = (uint8_t)atoi(p_token);
                    break;

                case 'y':
                    config.ym2151_clock = (e_ym2151_clock_t)saaym_clamp_value(YM2151_CLOCK_FIRST_VALID, atoi(p_token), YM2151_CLOCK_LAST_VALID);
                    break;

                case 's':
                    config.saa1099_clock = (e_saa1099_clock_t)saaym_clamp_value(SAA1099_CLOCK_FIRST_VALID, atoi(p_token), SAA1099_CLOCK_LAST_VALID);
                    break;

                default:
                    break;
            }
        }
    }

    return config;
}

/*!
 * @brief Detected whether the SAAYM exists.
 * 
 * @param[in] b_detect_irq If true, attempts to detect the IRQ the SAAYM is installed at.  Otherwise, false.
 *
 * @return saaym configuration.
 */
saaym_config_t
saaym_detect(bool const b_detect_irq)
{
    saaym_config_t config = saaym_get_config_from_environment(); // try getting settings from environment first

    if (config.base_io_port == 0)
    {
#if defined(PCZSM_DEBUG)
        printf("SAAYM environment variable not found; auto detecting hardware ...\n\n");
#endif
        // Valid environment variable was not found; try auto detection instead.
        for (uint16_t base_io_port = SAAYM_BASE_IO_START; base_io_port <= SAAYM_BASE_IO_END; base_io_port += SAAYM_BASE_IO_OFFSET)
        {
            if (saaym_detect_base_io_port(base_io_port))
            {
                config.base_io_port  = base_io_port;

                // assume default YM2151 SAA1099 clocks; user should define environment variable to override.
                config.ym2151_clock  = YM2151_CLOCK_3579545_HZ;
                config.saa1099_clock = SAA1099_CLOCK_7159090_HZ;

                if (b_detect_irq)
                {
                    saaym_detect_irq();

                    if (g_saaym_detect_ym2151_clock.interrupt_number)
                    {
                        config.irq_number   = g_saaym_detect_ym2151_clock.interrupt_number ? g_saaym_detect_ym2151_clock.interrupt_number - INT08H_IRQ0 : 0;
                        config.ym2151_clock = g_saaym_detect_ym2151_clock.ym2151_clock;
                    }
                }

                return config;
            }
        }
    }

    return config; // No TexElect SAAYM found!
} /* saaym_detect() */

/*** end of file ***/
