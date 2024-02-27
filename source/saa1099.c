/** @file saa1099.c
 *
 * @brief VERA PSG to SAA1099 interfacing.
 *
 * @par
 * Handles VERA PSG to SAA1099 playback (for both 7.15909 MHz and 8 MHz settings).
 * VERA PSG PWM is not handled as the SAA1099 lacks this feature.
 * 
 */

#include <stdint.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "saa1099.h"
#include "vera.h"

#define FREQ_23_9_FIXED_POINT_SHIFT           (9U)
#define FREQ_SAA1099_FREQUENCY_DIV_MAX_RANGE  (511U)
#define FREQ_SAA1099_FREQUENCY_DIV_MAX_VALUE  (256U)

#define SAA1099_7MHZ_VERA_MIN_FREQUENCY ((uint16_t)74)
#define SAA1099_7MHZ_VERA_MAX_FREQUENCY ((uint16_t)18767)
#define SAA1099_8MHZ_VERA_MIN_FREQUENCY ((uint16_t)82)
#define SAA1099_8MHZ_VERA_MAX_FREQUENCY ((uint16_t)20971) // 20971.52
#define SAA1099_7MHZ_VERA_FREQ_RANGE (SAA1099_7MHZ_VERA_MAX_FREQUENCY - SAA1099_7MHZ_VERA_MIN_FREQUENCY + 1)
#define SAA1099_8MHZ_VERA_FREQ_RANGE (SAA1099_8MHZ_VERA_MAX_FREQUENCY - SAA1099_8MHZ_VERA_MIN_FREQUENCY + 1)

#define SAA1099_MAX_CHIPS         (2U)
#define SAA1099_CHANNEL_COUNT     (6U)
#define SAA1099_OCTAVE_COUNT      (8U)
#define SAA1099_REGISTER_COUNT    (32U)
#define SAA1099_MAX_OCTAVE        (SAA1099_OCTAVE_COUNT - 1)
#define SAA1099_VERA_PSG_CHANNELS (12U)

#define SAA1099_ADDRESS_AMPLITUDE                    (0x00U)
#define SAA1099_ADDRESS_FREQUENCY                    (0x08U)
#define SAA1099_ADDRESS_OCTAVE                       (0x10U)
#define SAA1099_ADDRESS_FREQUENCY_ENABLE             (0x14U)
#define SAA1099_ADDRESS_NOISE_ENABLE                 (0x15U)
#define SAA1099_ADDRESS_FREQUENCY_RESET_SOUND_ENABLE (0x1CU)

#define SAA1099_SOUND_ENABLE                         (0x01U)
#define SAA1099_FREQUENCY_RESET                      (0x02U)

#define SAA1099_FREQUENCY_ENABLE_ALL_CHANNELS        (0x3FU)
#define SAA1099_MASK_OCTAVE_BITS                     (0x77U)
#define SAA1099_MASK_OCTAVE_SELECT                   (0x07U)

#define SAA1099_MASK_PACKED_FREQUENCY_FREQUENCY      (0xFFU)
#define SAA1099_MASK_PACKED_FREQUENCY_OCTAVE         (0x0700U)
#define SAA1099_SHIFT_PACKED_FREQUENCY_OCTAVE        (8U)


#define SAA1099_GET_CACHE_OFFSET(chip_id) ((chip_id) << 5U)

#define SAA1099_8MHZ_VERA_FREQ 164 // 164 - (8,000,000 / 25,000,000) * 512
#define SAA1099_7MHZ_VERA_FREQ 147 // 147 - (7,159,090 / 25,000,000) * 512

typedef struct
{
    uint32_t clock_ratio;
    uint16_t range;
    uint16_t min_r;
    uint16_t max_r;
} saa1099_verga_psg_range_t;

static saa1099_verga_psg_range_t const g_saa1099_8mhz_vera_psg_range = { SAA1099_8MHZ_VERA_FREQ, SAA1099_8MHZ_VERA_FREQ_RANGE, SAA1099_8MHZ_VERA_MIN_FREQUENCY, SAA1099_8MHZ_VERA_MAX_FREQUENCY };
static saa1099_verga_psg_range_t const g_saa1099_7mhz_vera_psg_range = { SAA1099_7MHZ_VERA_FREQ, SAA1099_7MHZ_VERA_FREQ_RANGE, SAA1099_7MHZ_VERA_MIN_FREQUENCY, SAA1099_7MHZ_VERA_MAX_FREQUENCY };
static saa1099_verga_psg_range_t const * g_p_saa1099_vera_psg_range = &g_saa1099_8mhz_vera_psg_range;

static uint8_t const g_vera_psg_volume_to_saa1099_volume[] =
{ 
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U,
    0x22U, 0x22U, 0x22U, 0x22U, 0x22U, 0x22U, 0x22U, 0x22U, 0x22U, 0x33U, 0x33U, 0x33U, 0x33U, 0x33U, 0x44U, 0x44U,
    0x44U, 0x44U, 0x44U, 0x55U, 0x55U, 0x55U, 0x66U, 0x66U, 0x77U, 0x77U, 0x77U, 0x88U, 0x88U, 0x99U, 0x99U, 0xAAU
};

static uint16_t g_vera_psg_channel_frequency[SAA1099_VERA_PSG_CHANNELS];
static uint8_t  g_vera_psg_channel_waveform[SAA1099_VERA_PSG_CHANNELS];
static uint8_t g_cache[SAA1099_REGISTER_COUNT * SAA1099_MAX_CHIPS];
static uint8_t const g_saa_amplitude_mask[4] = { 0x00U, 0x0FU, 0xF0U, 0xFFU };
static uint16_t const * g_p_vera_psg_to_frequency_table = NULL;
static uint16_t g_port_base_address = 0x210U;

// Adapted from: http://www.hackersdelight.org/hdcodetxt/nlz.c.txt
static uint16_t const
nlz5_16(uint16_t x)
{
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);

    x = ~x;

    // pop
    x = x - ((x >> 1) & 0x5555);
    x = (x & 0x3333) + ((x >> 2) & 0x3333);
    x = (x + (x >> 4)) & 0x0F0F;
    x = x + (x << 8);

    return x >> 8;
}

#if defined(__WATCOMC__)
uint16_t nlz5_16_inline(uint16_t x);
#pragma aux nlz5_16_inline = \
    "mov dx, ax    " \
    "shr dx, 0x01  " \
    "or  ax, dx    " \
    "mov dx, ax    " \
    "shr dx, 0x01  " \
    "shr dx, 0x01  " \
    "or  ax, dx    " \
    "mov cl, 0x04  " \
    "mov dx, ax    " \
    "shr dx, cl    " \
    "or  ax, dx    " \
    "mov dl, ah    " \
    "xor dh, dh    " \
    "or  ax, dx    " \
    "not ax        " \
    "mov dx, ax    " \
    "shr dx, 0x01  " \ 
    "and dx, 0x5555" \
    "sub ax, dx    " \
    "mov dx, ax    " \
    "shr dx, 0x01  " \
    "shr dx, 0x01  " \
    "and ax, 0x3333" \
    "and dx, 0x3333" \
    "add ax, dx    " \
    "mov dx, ax    " \
    "shr dx, cl    " \
    "add ax, dx    " \
    "and ax, 0x0f0f" \
    "mov dh, al    " \
    "xor dl, dl    " \
    "add ax, dx    " \
    "mov al, ah    " \
    "xor ah, ah    " \
    parm[ax]     \
    value[ax]     \
    modify[ax cx dx];
#else
uint16_t nlz5_16_inline(uint16_t x) {
    return nlz5_16(x);
}
#endif

/*!
 * @brief Writes data to specified SAA1099 address.
 *
 * @param[in] p_range SAA1099 chip ID.
 * 
 * @return VERA PSG to SAA1099 frequency table.
 */
static uint16_t const * const
saa1099_generate_vera_psg_frequency_table(saa1099_verga_psg_range_t const * const p_range)
{
    // SAA1099 @ 7.15909 MHz - 27.36 Hz ~  6,991.30 Hz
    // SAA1099 @ 8       MHz - 30.58 Hz ~  7,812.50 Hz
    // VERA    @ 25      MHz -  0    Hz ~ 24,413.69 Hz
    uint16_t * const p_vera_psg_to_frequency_table = (uint16_t *)malloc(sizeof(uint16_t) * p_range->range);

    if (p_vera_psg_to_frequency_table != NULL)
    {
        // 1.  Generate clock ratio table for SAA1099 octave range; VERA seems to range from 0 to 15
        uint32_t ratios[SAA1099_OCTAVE_COUNT];
        for (uint8_t octave = 0; octave < SAA1099_OCTAVE_COUNT; ++octave)
        {
            ratios[octave] = p_range->clock_ratio << (VERA_PSG_FREQ_BASE_OCTAVE + octave);
        }

        // 2. Generate VERA frequency word to SAA1099 frequency/octave (within SAA1099 ranges)
        for (uint16_t index = 0; index < p_range->range; ++index)
        {
            uint16_t const vera_frequency_word = index + p_range->min_r;

            uint8_t octave = (uint8_t)(16 - 7 - nlz5_16(vera_frequency_word));

            if (octave > SAA1099_MAX_OCTAVE)
            {
                octave = SAA1099_MAX_OCTAVE;
            }

            uint32_t const value_fixed_point = ratios[octave] / vera_frequency_word;

            uint16_t value = (uint16_t)(value_fixed_point >> FREQ_23_9_FIXED_POINT_SHIFT);

            if (value > FREQ_SAA1099_FREQUENCY_DIV_MAX_RANGE)
            {
                value >>= 1;
                --octave;
            }
            else if (value < FREQ_SAA1099_FREQUENCY_DIV_MAX_VALUE)
            {
                value = FREQ_SAA1099_FREQUENCY_DIV_MAX_VALUE;
            }

            uint16_t const packed_frequency = (octave << SAA1099_SHIFT_PACKED_FREQUENCY_OCTAVE) | (FREQ_SAA1099_FREQUENCY_DIV_MAX_RANGE - value);

            p_vera_psg_to_frequency_table[index] = packed_frequency;
        }
    }

    return p_vera_psg_to_frequency_table;
}

/*!
 * @brief Writes data to specified SAA1099 address.
 *
 * @param[in] chip_id SAA1099 chip ID.
 * @param[in] address Internal SAA1099 address.
 * @param[in] data    Data to write.
 */
static void
saa1099_write(uint8_t const chip_id, uint8_t const address, uint8_t const data)
{
    uint8_t const cache_offset = SAA1099_GET_CACHE_OFFSET(chip_id) + address;

    g_cache[cache_offset] = data;

    uint16_t const port_base_address = g_port_base_address + (chip_id * 2);

    outp(port_base_address + 1, address);
    outp(port_base_address, data);
} /* saa1099_write() */

/*!
 * @brief Sets the SA1099 channel's frequency
 *
 * @param[in] chip_id                  SAA1099 chip ID.
 * @param[in] saa_channel              SAA1099 channel.
 * @param[in] vera_psg_frequency_word  VERA PSG frequency word.
 */
static void
saa1099_set_frequency(uint8_t const chip_id, uint8_t const saa_channel, uint16_t const vera_psg_frequency_word)
{
    uint16_t index = vera_psg_frequency_word;

    if (index < g_p_saa1099_vera_psg_range->min_r)
    {
        index = g_p_saa1099_vera_psg_range->min_r;
    }
    else if (index > g_p_saa1099_vera_psg_range->max_r)
    {
        index = g_p_saa1099_vera_psg_range->max_r;
    }

    index -= g_p_saa1099_vera_psg_range->min_r;

    uint8_t const channel_offset     = saa_channel >> 1U;
    uint8_t const octave_shift       = (saa_channel & 1U) << 2U;
    uint8_t const octave_mask        = (SAA1099_MASK_OCTAVE_BITS ^ (SAA1099_MASK_OCTAVE_SELECT << octave_shift));
    uint8_t const saa_address_octave = SAA1099_ADDRESS_OCTAVE + channel_offset;
    uint8_t const cache_offset       = SAA1099_GET_CACHE_OFFSET(chip_id) + saa_address_octave;

    uint16_t const packed_frequency = g_p_vera_psg_to_frequency_table[index];
    uint8_t  const octave_data      = (g_cache[cache_offset] & octave_mask) | (((packed_frequency & SAA1099_MASK_PACKED_FREQUENCY_OCTAVE) >> SAA1099_SHIFT_PACKED_FREQUENCY_OCTAVE) << octave_shift);

    saa1099_write(chip_id, (SAA1099_ADDRESS_FREQUENCY + saa_channel), (packed_frequency & SAA1099_MASK_PACKED_FREQUENCY_FREQUENCY));
    saa1099_write(chip_id, saa_address_octave, octave_data);
} /* saa1099_set_frequency() */

/*!
 * @brief Clears SAA1099 internal registers (all values are zeroed).
 */
static void
saa1099_clear(void)
{
    saa1099_write(0, SAA1099_ADDRESS_FREQUENCY_RESET_SOUND_ENABLE, 0);
    saa1099_write(1, SAA1099_ADDRESS_FREQUENCY_RESET_SOUND_ENABLE, 0);

    for (uint8_t address = 0; address < SAA1099_REGISTER_COUNT; ++address)
    {
        saa1099_write(0, address, 0);
        saa1099_write(1, address, 0);
    }
} /* saa1099_clear() */

/*!
 * @brief Initializes and prepares SAA1099 for VERA PSG emulation.
 *
 * @param[in] port_base_address Base port address for SAA1099 chips.
 * @param[in] clock             Input clock of SAA1099 chips.
 */
void
saa1099_vera_psg_initialize(uint16_t const port_base_address, e_saa1099_clock_t const clock)
{
    switch (clock)
    {
        case SAA1099_CLOCK_7159090_HZ:
            g_p_saa1099_vera_psg_range = &g_saa1099_7mhz_vera_psg_range;
            break;

        case SAA1099_CLOCK_8000000_HZ:
            g_p_saa1099_vera_psg_range = &g_saa1099_8mhz_vera_psg_range;
            break;

        default:
            g_p_saa1099_vera_psg_range = NULL;
            break;
    }

    if (g_p_saa1099_vera_psg_range != NULL)
    {
        g_p_vera_psg_to_frequency_table = saa1099_generate_vera_psg_frequency_table(g_p_saa1099_vera_psg_range);

        if (g_p_vera_psg_to_frequency_table != NULL)
        {
            g_port_base_address = port_base_address;

            saa1099_clear();

            memset(g_vera_psg_channel_frequency, 0, sizeof(g_vera_psg_channel_frequency));
            memset(g_vera_psg_channel_waveform, 0, sizeof(g_vera_psg_channel_waveform));

            for (uint8_t chip_id = 0; chip_id < SAA1099_MAX_CHIPS; ++chip_id)
            {
                saa1099_write(chip_id, SAA1099_ADDRESS_FREQUENCY_RESET_SOUND_ENABLE, SAA1099_FREQUENCY_RESET);
                saa1099_write(chip_id, SAA1099_ADDRESS_FREQUENCY_RESET_SOUND_ENABLE, SAA1099_SOUND_ENABLE);
                saa1099_write(chip_id, SAA1099_ADDRESS_FREQUENCY_ENABLE, SAA1099_FREQUENCY_ENABLE_ALL_CHANNELS);
            }
        }
    }
} /* saa1099_vera_psg_initialize() */

/*!
 * @brief Silences SAA1099s and clears internal states.
 */
void saa1099_vera_psg_terminate(void)
{
    saa1099_clear();

    if (g_p_vera_psg_to_frequency_table != NULL)
    {
        free((void *)g_p_vera_psg_to_frequency_table);
        g_p_vera_psg_to_frequency_table = NULL;
    }
} /* saa1099_vera_psg_terminate() */

/*!
 * @brief Translates VERA PSG writes to SAA1099.
 * 
 * @note Currently only handles VERA PSG channels 0 through 11.
 * 
 * @param[in] address VERA PSG address.
 * @param[in] data    VERA PSG data.
 */
void
saa1099_vera_psg_write(uint8_t const address, uint8_t const data)
{
    uint8_t const vera_psg_channel = VERA_PSG_ADDRESS_TO_CHANNEL(address);

    if (vera_psg_channel < SAA1099_VERA_PSG_CHANNELS)
    {
        uint8_t  const chip_id           = vera_psg_channel / SAA1099_CHANNEL_COUNT;
        uint8_t  const saa_channel       = vera_psg_channel % SAA1099_CHANNEL_COUNT;

        switch (VERA_PSG_ADDRESS_TO_OFFSET(address))
        {
            case VERA_PSG_OFFSET_FREQ_LO:
                {
                    uint16_t const vera_psg_frequency_word = (g_vera_psg_channel_frequency[vera_psg_channel] & VERA_PSG_MASK_FREQ_HI) | data;

                    saa1099_set_frequency(chip_id, saa_channel, vera_psg_frequency_word);

                    g_vera_psg_channel_frequency[vera_psg_channel] = vera_psg_frequency_word;
                }
                break;

            case VERA_PSG_OFFSET_FREQ_HI:
                {
                    uint16_t const vera_psg_frequency_word = ((uint16_t)data << VERA_PSG_SHIFT_FREQ_HI) | (g_vera_psg_channel_frequency[vera_psg_channel] & VERA_PSG_MASK_FREQ_LO);
            
                    saa1099_set_frequency(chip_id, saa_channel, vera_psg_frequency_word);

                    g_vera_psg_channel_frequency[vera_psg_channel] = vera_psg_frequency_word;
                }
                break;

            case VERA_PSG_OFFSET_RL_VOLUME:
                {
                    uint8_t const saa_amplitude = g_vera_psg_volume_to_saa1099_volume[(data & VERA_PSG_MASK_VOLUME)] & g_saa_amplitude_mask[(data & VERA_PSG_MASK_RIGHT_LEFT) >> VERA_PSG_SHIFT_LEFT];

                    saa1099_write(chip_id, (SAA1099_ADDRESS_AMPLITUDE + saa_channel), saa_amplitude);
                }
                break;

            case VERA_PSG_OFFSET_WAVEFORM_PULSE_WIDTH:
                {
                    uint8_t const vera_psg_waveform = (data & VERA_PSG_MASK_WAVEFORM) >> VERA_PSG_SHIFT_WAVEFORM;

                    // Check to see if waveform (PWM not supported); reduces unnecessary writes to SAA1099.
                    if (vera_psg_waveform ^ g_vera_psg_channel_waveform[vera_psg_channel])
                    {
                        // This just happens to work since the FE and NE addresses are adjacent
                        uint8_t const enable_index  = SAA1099_ADDRESS_FREQUENCY_ENABLE ^ ((vera_psg_waveform >> 1) & vera_psg_waveform);
                        uint8_t const disable_index = enable_index ^ 1; // Will select SAA1099_ADDRESS_FREQUENCY_ENABLE or SAA1099_ADDRESS_NOISE_ENABLE

                        uint8_t const offset   = SAA1099_GET_CACHE_OFFSET(chip_id);
                        uint8_t const saa_bits = 1 << saa_channel;

                        saa1099_write(chip_id, enable_index,  g_cache[enable_index + offset]  |  saa_bits);
                        saa1099_write(chip_id, disable_index, g_cache[disable_index + offset] & ~saa_bits);

                        g_vera_psg_channel_waveform[vera_psg_channel] = vera_psg_waveform; // cache waveform change
                    }
                }
                break;

            default:
                // Illegal VERA PSG offset
                break;
        }
    }
} /* saa1099_vera_psg_write() */

/*** end of file ***/
