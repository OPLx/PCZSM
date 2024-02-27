/** @file i8253.h
 *
 * @brief
 *
 * @par
 */

#pragma once

#define I8253_MIN_SPEED_IN_MHZ  18U
#define I8253_CLOCK_SPEED_IN_HZ 1193181U
#define I8253_CONTROL_WORD_PORT     0x43
#define I8253_COUNTER_0_DATA_PORT   0x40
#define I8253_COUNTER_1_DATA_PORT   0x41
#define I8253_COUNTER_2_DATA_PORT   0x42

#define I8253_COUNTER_0 0x00  // Select Counter 0
#define I8253_COUNTER_1 0x40  // Select Counter 1
#define I8253_COUNTER_2 0x80  // Select Counter 2

#define I8253_ACCESS_MODE_LATCH_COUNT 0x00
#define I8253_ACCESS_MODE_LATCH_LO_BYTE_ONLY 0x10
#define I8253_ACCESS_MODE_LATCH_HI_BYTE_ONLY 0x20
#define I8253_ACCESS_MODE_LATCH_LOHI_BYTE 0x30

#define I8253_MODE_0 0x00 // Interrupt on Terminal Count
#define I8253_MODE_1 0x02 // Programmable One-shot
#define I8253_MODE_2 0x04 // Rate Generator
#define I8253_MODE_3 0x06 // Square Wave Mode
#define I8253_MODE_4 0x08 // Software Triggered Strobe
#define I8253_MODE_5 0x0A // Hardware Triggered Strobe (Retriggerable)
#define I8253_MODE_6 0x0C // Rate Generator (don't care)
#define I8253_MODE_7 0x0E // Square Wave Mode (don't care)

#define I8253_BCD_0 0x0   // Binary Counter 16-bits
#define I8253_BCD_1 0x1   // Binary Coded Decimal (BCD) Counter (4 Decades)
