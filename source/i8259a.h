/** @file i8259a.h
 *
 * @brief
 *
 * @par
 */

#pragma once

#define PIC1_COMMAND 0x20
#define PIC1_DATA    (PIC1_COMMAND + 1)
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    (PIC2_COMMAND + 1)

#define PIC_EOI 0x20
