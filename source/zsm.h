/** @file zsm.h
 * 
 * @brief ZSM file playback routines.
 * 
 */

#pragma once

typedef enum
{
    ZSM_SUCCESS,
    ZSM_BAD_DATA_POINTER,
    ZSM_UNSUPPORTED_VERSION,
    ZSM_NOTHING_TO_PLAY
} e_zsm_result_t;

#define ZSM_REPEAT_FOREVER (0x8000U)

#pragma pack(push, 1)
typedef struct zsm_offset
{
    uint16_t address;
    uint8_t  bank;
} zsm_offset_t;

typedef struct zsm_header
{
    uint16_t      magic_header;
    uint8_t       version;
    zsm_offset_t  loop_point;
    zsm_offset_t  pcm_offset;
    uint8_t       fm_channel_mask;
    uint16_t      psg_channel_mask;
    uint16_t      tick_rate;
    uint16_t      reserved;
} zsm_header_t;
#pragma pack(pop)

typedef struct
{
    zsm_header_t   header;
    uint8_t      * p_data_stream;
} zsm_data_t;

//--------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------------------------------

typedef void(*ym2151_write_func_t)(uint8_t const address, uint8_t const data);
typedef void(*vera_psg_write_func_t)(uint8_t const address, uint8_t const data);

e_zsm_result_t     zsm_initialize(ram_handle_t p_zsm_ram_handle, ym2151_write_func_t const p_ym2151_func, vera_psg_write_func_t const p_vera_psg_write_func);
void               zsm_update(void);
void               zsm_start(uint16_t const zsm_repeat);
void               zsm_stop(void);
bool               zsm_is_playing(void);
zsm_header_t const zsm_get_header(void);

//--------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//--------------------------------------------------------------------

/*** end of file ***/
