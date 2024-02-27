/** @file vera.h
 *
 * @brief
 *
 * @par
 */

enum
{
    VERA_PSG_OFFSET_FREQ_LO,
    VERA_PSG_OFFSET_FREQ_HI,
    VERA_PSG_OFFSET_RL_VOLUME,
    VERA_PSG_OFFSET_WAVEFORM_PULSE_WIDTH
};

#define VERA_PSG_ADDRESS_TO_CHANNEL(address) ((address) >> 2U)
#define VERA_PSG_ADDRESS_TO_OFFSET(address)  ((address) & 3U)

#define VERA_PSG_MAX_VOLUME_LEVELS (64U)
#define VERA_PSG_MASK_FREQ_LO      (0xFFU)
#define VERA_PSG_MASK_FREQ_HI      (0xFF00U)
#define VERA_PSG_MASK_RIGHT        (0x80U)
#define VERA_PSG_MASK_LEFT         (0x40U)
#define VERA_PSG_MASK_RIGHT_LEFT   (VERA_PSG_MASK_RIGHT | VERA_PSG_MASK_LEFT)
#define VERA_PSG_MASK_VOLUME       (0x3FU)
#define VERA_PSG_MASK_WAVEFORM     (0xC0U)
#define VERA_PSG_SHIFT_WAVEFORM    (6U)
#define VERA_PSG_SHIFT_RIGHT       (7U)
#define VERA_PSG_SHIFT_LEFT        (6U)
#define VERA_PSG_SHIFT_FREQ_HI     (8U)
#define VERA_PSG_FREQ_BASE_OCTAVE  (17U)
