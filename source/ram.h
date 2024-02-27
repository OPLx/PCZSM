/** @file ram.h
 *
 * @brief Simple data RAM bank handling for a file read from disk.
 *
 */

#pragma once

typedef enum
{
    RAM_SEEK_ORIGIN_SET,
    RAM_SEEK_ORIGIN_CURRENT
} e_ram_seek_origin_t;

typedef enum
{
    RAM_LOAD_SUCCESS,
    RAM_LOAD_UNABLE_TO_OPEN_FILE,
    RAM_LOAD_INSUFFICIENT_MEMORY
} e_ram_result_t;

//--------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------------------------------

typedef struct ram * ram_handle_t;

typedef struct
{
    uint8_t const * p_start;
    uint8_t const * p_end;
    uint8_t const * p_current;
    uint8_t         bank;
} ram_bank_t;

e_ram_result_t      const ram_load_file(ram_handle_t * const pp_ram_handle, char const * const p_file_name);
void                      ram_free(ram_handle_t * const pp_ram_handle);
uint8_t     const * const ram_get_address(ram_handle_t const p_ram_handle, uint32_t const offset);
void                      ram_seek_bank(ram_handle_t p_ram_handle, int32_t const offset, e_ram_seek_origin_t const seek_origin, ram_bank_t * const p_ram_bank);
uint8_t                   ram_read_uint8(ram_handle_t p_ram_handle);
void                      ram_get_bank(ram_handle_t p_ram_handle, ram_bank_t * const p_ram_bank);

//--------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//--------------------------------------------------------------------

/*** end of file ***/
