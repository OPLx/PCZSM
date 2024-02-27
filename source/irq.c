/** @file irq.c
 *
 * @brief IRQ detection handler
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>
#include <dos.h>
#include <i86.h>
#include "irq.h"
#include "timer.h"
#include "i8259a.h"

#define IRQ_START_MASK (0x53U) // Mask for IRQs 7, 5, 3, and 2 (used by SAAYM)

typedef enum
{
    IRQ_2,
    IRQ_3,
    IRQ_5,
    IRQ_7,

    IRQ_COUNT
} e_irq_t;

typedef struct irq_detect_handler_state
{
    interrupt_handler_t const p_new_handler;
    uint8_t             const irq_number;
    interrupt_handler_t       p_old_handler;
    bool                      triggered;
    bool                      spurious;
} irq_detect_handler_state_t;

static void IRQ_FAR_INTERRUPT irq_2_detect_handler(void);
static void IRQ_FAR_INTERRUPT irq_3_detect_handler(void);
static void IRQ_FAR_INTERRUPT irq_5_detect_handler(void);
static void IRQ_FAR_INTERRUPT irq_7_detect_handler(void);

static irq_detect_handler_state_t g_detect_handler_states[IRQ_COUNT] =
{
    { &irq_2_detect_handler, 0x0AU, NULL, false, false }, // IRQ2
    { &irq_3_detect_handler, 0x0BU, NULL, false, false }, // IRQ3
    { &irq_5_detect_handler, 0x0DU, NULL, false, false }, // IRQ5
    { &irq_7_detect_handler, 0x0FU, NULL, false, false }  // IRQ7
};

static uint8_t g_pic_old_mask = 0;

static void IRQ_FAR_INTERRUPT
irq_2_detect_handler(void)
{
    irq_detect_handler_state_t * const p_handler_states = &g_detect_handler_states[IRQ_2];
    p_handler_states->triggered = true;

    outp(PIC1_COMMAND, PIC_EOI);
    outp(PIC2_COMMAND, PIC_EOI);
} /* irq_2_detect_handler() */

static void IRQ_FAR_INTERRUPT
irq_3_detect_handler(void)
{
    irq_detect_handler_state_t * const p_handler_states = &g_detect_handler_states[IRQ_3];
    p_handler_states->triggered = true;

    outp(PIC1_COMMAND, PIC_EOI);
} /* irq_3_detect_handler() */

static void IRQ_FAR_INTERRUPT
irq_5_detect_handler(void)
{
    irq_detect_handler_state_t * const p_handler_states = &g_detect_handler_states[IRQ_5];
    p_handler_states->triggered = true;

    outp(PIC1_COMMAND, PIC_EOI);
} /* irq_5_detect_handler() */

static void IRQ_FAR_INTERRUPT
irq_7_detect_handler(void)
{
    irq_detect_handler_state_t * const p_handler_states = &g_detect_handler_states[IRQ_7];
    p_handler_states->triggered = true;

    outp(PIC1_COMMAND, PIC_EOI);
} /* irq_7_detect_handler() */

void
irq_install_detect_handlers(void)
{
    g_pic_old_mask = inp(PIC1_DATA);

    _disable(); // Disable IRQs

    for (uint8_t index = 0; index < IRQ_COUNT; ++index)
    {
        irq_detect_handler_state_t * const p_detect_handler_state = &g_detect_handler_states[index];

        p_detect_handler_state->p_old_handler = _dos_getvect(p_detect_handler_state->irq_number);
        _dos_setvect(p_detect_handler_state->irq_number, p_detect_handler_state->p_new_handler);
    }

    _enable(); // Enable IRQs

    outp(PIC1_DATA, (g_pic_old_mask & IRQ_START_MASK));

    delay(100);

    for (uint8_t index = 0; index < IRQ_COUNT; ++index)
    {
        irq_detect_handler_state_t * const p_detect_handler_state = &g_detect_handler_states[index];

        p_detect_handler_state->spurious   = p_detect_handler_state->triggered;
        p_detect_handler_state->triggered = false;
    }
} /* irq_install_detect_handlers() */

void
irq_uninstall_detect_handlers(void)
{
    _disable();

    for(int index = 0; index < IRQ_COUNT; ++index)
    {
        irq_detect_handler_state_t const * const p_detect_handler_state = &g_detect_handler_states[index];

        if (p_detect_handler_state->p_old_handler != NULL)
        {
            _dos_setvect(p_detect_handler_state->irq_number, p_detect_handler_state->p_old_handler);
        }
    }   

    _enable();

    outp(PIC1_DATA, g_pic_old_mask);
} /* irq_uninstall_detect_handlers() */

uint8_t
irq_sense(void)
{
    uint8_t triggered_irq = 0x0;

    for(int index = 0; index < IRQ_COUNT; ++index)
    {
        irq_detect_handler_state_t * const p_detect_handler_state = &g_detect_handler_states[index];

        if (p_detect_handler_state->triggered && !p_detect_handler_state->spurious)
        {
            triggered_irq = p_detect_handler_state->irq_number; // found triggered IRQ 
            break;
        }
    }

    return triggered_irq;
} /* irq_sense() */

void
irq_set_mask(uint8_t const irq_number)
{
    uint8_t const mask = inp(PIC1_DATA) | (1 << irq_number);
    outp(PIC1_DATA, mask);
} /* irq_set_mask() */

void
irq_clear_mask(uint8_t const irq_number)
{
    uint8_t const mask = inp(PIC1_DATA) & ~(1 << irq_number);
    outp(PIC1_DATA, mask);
} /* irq_clear_mask() */

/*** end of file ***/
