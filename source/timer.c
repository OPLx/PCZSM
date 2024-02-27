/** @file timer.c
 *
 * @brief Timer interface
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <dos.h>
#include <conio.h>
#include <i86.h>
#include "timer.h"
#include "irq.h"
#include "i8253.h"
#include "i8259a.h"
#include "ym2151.h"

#define INTERRUPT_FIXED_POINT (8U)
#define FIXED_POINT_ONE       (1U << INTERRUPT_FIXED_POINT)
#define FIXED_POINT_PRECISION (1000U)
#define FIXED_POINT_INV_18_2  (14065U) // fixed point representation of 1000.0f/18.2f; 1000 is multiplied to get more precision

static interrupt_handler_t g_p_old_interrupt_handler = NULL;
static timer_callback_t    g_p_timer_callback        = NULL;
static uint8_t             g_interrupt_number        = 0;

static volatile uint32_t g_interrupt_counter = 0;
static volatile uint32_t g_interrupt_total   = 0;
static volatile bool     g_timer_b_stop_requested    = false;
static volatile bool     g_timer_b_stop_acknowledged = false;

static void IRQ_FAR_INTERRUPT
timer_irq_0_handler(void)
{
    if (g_p_timer_callback)
    {
        g_p_timer_callback();
    }

    /* Update old interrupt handler.*/
    if (g_p_old_interrupt_handler && (g_interrupt_counter >= g_interrupt_total))
    {
        g_p_old_interrupt_handler();
        g_interrupt_counter -= g_interrupt_total;
    }
    else
    {
        outp(PIC1_COMMAND, PIC_EOI);
    }

    g_interrupt_counter += FIXED_POINT_ONE;
} /* timer_irq_0_handler() */

static void IRQ_FAR_INTERRUPT
timer_irq_2_handler(void)
{
    bool const ym2151_timer_b = (ym2151_read_status() & YM2151_STATUS_TIMER_B_FLAG) != 0;

    if (ym2151_timer_b && g_p_timer_callback)
    {
        g_p_timer_callback();

        outp(PIC1_COMMAND, PIC_EOI);
        outp(PIC2_COMMAND, PIC_EOI);

        ym2151_enable_timer_b(!g_timer_b_stop_requested); // restart (if not request to stop)
        g_timer_b_stop_acknowledged = g_timer_b_stop_requested;
    }
} /* timer_irq_2_handler() */

static void IRQ_FAR_INTERRUPT
timer_irq_357_handler(void)
{
    bool const ym2151_timer_b = (ym2151_read_status() & YM2151_STATUS_TIMER_B_FLAG) != 0;

    if (ym2151_timer_b && g_p_timer_callback)
    {
        g_p_timer_callback();

        outp(PIC1_COMMAND, PIC_EOI);

        ym2151_enable_timer_b(!g_timer_b_stop_requested); // restart (if no request to stop)
        g_timer_b_stop_acknowledged = g_timer_b_stop_requested;
    }
} /* timer_irq_357_handler() */

static void
timer_setup_pit_channel_0(uint16_t const rate_in_hertz)
{
    uint16_t const pit_counter  = (uint16_t)(I8253_CLOCK_SPEED_IN_HZ / rate_in_hertz);
    uint8_t  const counter_low  = pit_counter & 0xFFU;
    uint8_t  const counter_high = (pit_counter >> 8U);

    g_interrupt_total = ((uint32_t)rate_in_hertz * FIXED_POINT_INV_18_2) / FIXED_POINT_PRECISION;
    g_interrupt_counter = g_interrupt_total;

    _disable();

    outp(I8253_CONTROL_WORD_PORT, I8253_COUNTER_0 | I8253_ACCESS_MODE_LATCH_LOHI_BYTE | I8253_MODE_2 | I8253_BCD_0);
    outp(I8253_COUNTER_0_DATA_PORT, counter_low);
    outp(I8253_COUNTER_0_DATA_PORT, counter_high);

    _enable();
} /* timer_setup_pit_channel_0() */

static void
timer_restore_pit_channel_0(void)
{
    _disable();

    outp(I8253_CONTROL_WORD_PORT, I8253_COUNTER_0 | I8253_ACCESS_MODE_LATCH_LOHI_BYTE | I8253_MODE_2 | I8253_BCD_0);
    outp(I8253_COUNTER_0_DATA_PORT, 0);
    outp(I8253_COUNTER_0_DATA_PORT, 0);

    _enable();
} /* timer_restore_pit_channel_0() */

int
timer_start(uint8_t const irq_number, uint16_t const rate_in_hertz, timer_callback_t const p_timer_callback)
{
    interrupt_handler_t irq_handler = NULL;

    switch (irq_number)
    {
        case 0:
            irq_handler = &timer_irq_0_handler;
            break;

        case 2:
            irq_handler = &timer_irq_2_handler;
            break;

        case 3:
        case 5:
        case 7:
            irq_handler = &timer_irq_357_handler;
            break;

        default:
            break;
    }

    if ((irq_handler != NULL) && (p_timer_callback != NULL))
    {
        g_interrupt_number        = INT08H_IRQ0 + irq_number;
        g_p_timer_callback        = p_timer_callback;
        g_p_old_interrupt_handler = _dos_getvect(g_interrupt_number);

        _disable();
        _dos_setvect(g_interrupt_number, irq_handler);
        _enable();

        if (g_interrupt_number == INT08H_IRQ0)
        {
            if (rate_in_hertz > I8253_MIN_SPEED_IN_MHZ)
            {
                timer_setup_pit_channel_0(rate_in_hertz);
            }
            else
            {
                timer_restore_pit_channel_0();
            }
        }
        else
        {
            _disable();
            irq_clear_mask(irq_number);
            _enable();

            ym2151_set_timer_b_hz(rate_in_hertz);
            ym2151_enable_timer_b(true);
        }

        return 0;
    }

    return 1;
} /* timer_start() */

void
timer_stop(void)
{
    if (g_p_timer_callback != NULL)
    {
        if (g_interrupt_number == INT08H_IRQ0)
        {
            timer_restore_pit_channel_0();
        }
        else
        {
            uint16_t wait_delay = 0;
            g_timer_b_stop_requested = true;
            while ( (g_timer_b_stop_acknowledged == false) || (wait_delay++ < 255 ) )
            {
                // wait
            }

            _disable();
            irq_set_mask(g_interrupt_number - INT08H_IRQ0);
            _enable();
        }

        _dos_setvect(g_interrupt_number, g_p_old_interrupt_handler);  /* restore */
        g_p_timer_callback = NULL;
    }
} /* timer_stop() */

/*** end of file ***/
