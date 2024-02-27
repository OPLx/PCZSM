/** @file zsm.c
 *
 * @brief ZSM file playback routines.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ram.h"
#include "zsm.h"

enum
{
    ZSM_CMD_PSG_WRITE_00 = 0x00,
    ZSM_CMD_PSG_WRITE_3F = 0x3F,
    ZSM_CMD_EXT          = 0x40,
    ZSM_CMD_FM_WRITE_41  = 0x41,
    ZSM_CMD_FM_WRITE_7F  = 0x7F,
    ZSM_CMD_EOF          = 0x80,
    ZSM_CMD_DELAY_81     = 0x81,
    ZSM_CMD_DELAY_FF     = 0xFF
};

#define ZSM_VERSION_01                ((uint8_t)0x01U)
#define ZSM_REPEAT_FOREVER_COUNT      ((uint8_t)0xFFU)
#define ZSM_REPEAT_COUNT_MASK         ((uint8_t)0xFFU)
#define ZSM_MASK_CMD_DATA_EXT         ((uint8_t)0x3FU)
#define ZSM_MASK_CMD_DATA_PSG_ADDRESS ((uint8_t)0x3FU)
#define ZSM_MASK_CMD_DATA_FM_PAIRS    ((uint8_t)0x3FU)
#define ZSM_MASK_CMD_DATA_DELAY       ((uint8_t)0x7FU)

#define RAM_UPDATE_POS() \
    if (g_ram_bank.p_current < (g_ram_bank.p_end - 1)) \
    { \
        ++g_ram_bank.p_current; \
    } \
    else \
    { \
        ++g_ram_bank.bank; \
        ram_get_bank(g_p_zsm_ram_handle, &g_ram_bank); \
    }

static void
zsm_ym2151_write_func_null(uint8_t const address, uint8_t const data)
{
    (void)address;
    (void)data;
} /* zsm_ym2151_write_func_null() */

static void
zsm_vera_psg_func_null(uint8_t const address, uint8_t const data)
{
    (void)address;
    (void)data;
} /* zsm_vera_psg_func_null() */

static zsm_header_t          g_empty_header          = { 0 };
static ym2151_write_func_t   g_p_ym2151_write_func   = &zsm_ym2151_write_func_null;
static vera_psg_write_func_t g_p_vera_psg_write_func = &zsm_vera_psg_func_null;
static ram_handle_t          g_p_zsm_ram_handle      = NULL;
static bool                  g_play_stream           = false;
static uint16_t              g_zsm_repeat            = 0;
static uint8_t               g_delay_ticks           = 0;
static uint8_t               g_repeat_count          = 0;
static ram_bank_t            g_ram_bank;

/*!
 * @brief Intializes the ZSM playback system.
 * 
 * @param[in] p_zsm_ram_handle      The ram handle to the ZSM data.
 * @param[in] p_ym2151_write_func   Pointer to the YM2151 write function.
 * @param[in] p_vera_psg_write_func Pointer to the VERA PSG write function.
 *
 * @return The result of the initialization
 */
e_zsm_result_t
zsm_initialize(ram_handle_t p_zsm_ram_handle, ym2151_write_func_t const p_ym2151_write_func, vera_psg_write_func_t const p_vera_psg_write_func)
{
    if (p_zsm_ram_handle)
    {
        g_p_zsm_ram_handle = p_zsm_ram_handle;

        zsm_header_t const * const p_header = (zsm_header_t const * const)ram_get_address(g_p_zsm_ram_handle, 0);

        if (p_header->version == ZSM_VERSION_01)
        {
            g_play_stream = false;

            if (p_header->fm_channel_mask && p_ym2151_write_func)
            {
                g_p_ym2151_write_func = p_ym2151_write_func;
            }

            if (p_header->psg_channel_mask && p_vera_psg_write_func)
            {
                g_p_vera_psg_write_func = p_vera_psg_write_func;
            }

            if ((g_p_ym2151_write_func != &zsm_ym2151_write_func_null) || (g_p_vera_psg_write_func != &zsm_vera_psg_func_null))
            {
                ram_seek_bank(g_p_zsm_ram_handle, sizeof(zsm_header_t), RAM_SEEK_ORIGIN_SET, &g_ram_bank);

                return ZSM_SUCCESS;
            }
            else
            {
                return ZSM_NOTHING_TO_PLAY;
            }
        }

        return ZSM_UNSUPPORTED_VERSION;
    }

    return ZSM_BAD_DATA_POINTER;
} /* zsm_initialize() */

/*!
 * @brief Updates the ZSM playback.
 */
void
zsm_update(void)
{
    if (g_play_stream == false)
    {
        return;
    }

    if (g_delay_ticks > 0)
    {
        --g_delay_ticks;
    }

    while (g_play_stream && (g_delay_ticks == 0))
    {
        uint8_t const command = *g_ram_bank.p_current;
        RAM_UPDATE_POS();

        switch(command)
        {
            case ZSM_CMD_EXT:
                {
                    uint8_t const extension_command = *g_ram_bank.p_current;
                    RAM_UPDATE_POS();

                    ram_seek_bank(g_p_zsm_ram_handle, (extension_command & ZSM_MASK_CMD_DATA_EXT), RAM_SEEK_ORIGIN_CURRENT, &g_ram_bank);
                }
                break;

            case ZSM_CMD_EOF:
                {
                    // EOF
                    zsm_header_t const * const p_header = (zsm_header_t const * const)ram_get_address(g_p_zsm_ram_handle, 0);
                    if ( (g_repeat_count > 0) && (p_header->loop_point.address || p_header->loop_point.bank))
                    {
                        uint32_t const loop_offset = ((uint32_t)p_header->loop_point.bank << 16) | (p_header->loop_point.address);

                        ram_seek_bank(g_p_zsm_ram_handle, loop_offset, RAM_SEEK_ORIGIN_SET, &g_ram_bank);

                        --g_repeat_count;
                        if ((g_zsm_repeat & ZSM_REPEAT_FOREVER) && (g_repeat_count == 0))
                        {
                            g_repeat_count = ZSM_REPEAT_FOREVER_COUNT; // restore repeat count
                        }
                    }
                    else
                    {
                        g_play_stream = false;
                        g_delay_ticks = 0;
                    }
                }
                break;

            default:
                {
                    if (command < ZSM_CMD_EXT)
                    {
                        // PSG write
                        uint8_t const address = command & ZSM_MASK_CMD_DATA_PSG_ADDRESS;
                        uint8_t const data    = *g_ram_bank.p_current;

                        RAM_UPDATE_POS();

                        (*g_p_vera_psg_write_func)(address, data);
                    }
                    else if (command < ZSM_CMD_EOF)
                    {
                        // FM write
                        uint8_t const register_value_pairs = (command & ZSM_MASK_CMD_DATA_FM_PAIRS);

                        for(uint8_t register_value_pair = 0; register_value_pair < register_value_pairs; ++register_value_pair)
                        {
                            uint8_t const address = *g_ram_bank.p_current;
                            RAM_UPDATE_POS();
                            uint8_t const data    = *g_ram_bank.p_current;
                            RAM_UPDATE_POS();

                            (*g_p_ym2151_write_func)(address, data);
                        }
                    }
                    else
                    {
                        // Delay ticks
                        g_delay_ticks = command & ZSM_MASK_CMD_DATA_DELAY;
                    }
                }
                break;
        }
    }
} /* zsm_update() */

/*!
 * @brief Start ZSM playback.
 * 
 * @param[in] zsm_repeat Repeat control value.
 */
void
zsm_start(uint16_t const zsm_repeat)
{
    g_delay_ticks    = 1;
    g_play_stream    = true;
    g_zsm_repeat     = zsm_repeat;
    g_repeat_count   = (zsm_repeat & ZSM_REPEAT_FOREVER) ? ZSM_REPEAT_FOREVER_COUNT : (zsm_repeat & ZSM_REPEAT_COUNT_MASK);
} /* zsm_start() */

/*!
 * @brief Stop ZSM playback.
 */
void
zsm_stop(void)
{
    g_play_stream = false;
} /* zsm_stop() */

/*!
 * @brief Checks whether ZSM playback is active.
 * 
 * @return returns true if playing, otherwise false.
 */
bool
zsm_is_playing(void)
{
    return g_play_stream;
} /* zsm_is_playing() */

/*!
 * @brief Gets the current ZSM header information.
 *
 * @return ZSM header information.
 */
zsm_header_t const
zsm_get_header(void)
{
    return g_p_zsm_ram_handle ? *(zsm_header_t const *)ram_get_address(g_p_zsm_ram_handle, 0) : g_empty_header;
} /* zsm_get_header() */

/*** end of file ***/
