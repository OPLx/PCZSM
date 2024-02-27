/** @file irq.h
 *
 * @brief IRQ detection handler
 *
 */

#pragma once

#if defined(__WATCOMC__)
#   define IRQ_FAR_INTERRUPT __interrupt __far
#else
#   define IRQ_FAR_INTERRUPT
#endif

typedef void (IRQ_FAR_INTERRUPT * interrupt_handler_t)(void);

#define INT08H_IRQ0 (0x08U)
#define INT10H_IRQ2 (0x0AU)

//--------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------------------------------

void irq_install_detect_handlers(void);
uint8_t irq_sense(void);
void irq_uninstall_detect_handlers(void);
void irq_set_mask(uint8_t const irq_number);
void irq_clear_mask(uint8_t const irq_number);

//--------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//--------------------------------------------------------------------

/*** end of file ***/
