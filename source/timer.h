/** @file timer.h
 *
 * @brief Timer interface
 *
 */

#pragma once

//--------------------------------------------------------------------
typedef void (*timer_callback_t)(void);
//--------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------------------------------

int  timer_start(uint8_t const irq_number, uint16_t const rate_in_hertz, timer_callback_t const p_timer_callback);
void timer_stop(void);

//--------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//--------------------------------------------------------------------
