/** @file ram.c
 * 
 * @brief Simple data RAM bank handling for a file read from disk.
 * 
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <malloc.h>
#include "ram.h"

#define RAM_DATA_DEBUG
#undef RAM_DATA_DEBUG

#if defined(_M_I86)
    #define USE_BANKS
#endif

#if defined(USE_BANKS)
    // Allocations will be limited up to blocks*(2^blocks) bytes.
    #define RAM_BANK_MAX_BANKS   (16U)
    #define RAM_BANK_SHIFT       (15U)
    #define RAM_BANK_SIZE        (1U << RAM_BANK_SHIFT)
    #define RAM_BANK_OFFSET_MASK (RAM_BANK_SIZE-1)
    #define RAM_BANK_INDEX_MASK  (0xFU)

    typedef uint16_t banksize_t;
#else
    typedef uint32_t banksize_t;
#endif

typedef struct ram
{
    uint8_t  ** pp_banks;
    size_t   *  p_bank_sizes;
    uint32_t    absolute_position;
    banksize_t  bank_offset;
    uint8_t     bank;
    uint8_t     number_of_banks;
} ram_t;

/*!
 * @brief Releases memory for RAM data banks.
 * @param[in, out] p_ram_data RAM bank handler structure.
 */
static void
ram_data_free_banks(ram_t * const p_ram)
{
    if (p_ram->number_of_banks > 0)
    {
        for (int8_t bank = p_ram->number_of_banks - 1; bank >= 0; --bank)
        {
            free((void *)p_ram->pp_banks[bank]);
        }

        free((void *)p_ram->p_bank_sizes);
        free((void *)p_ram->pp_banks);

        p_ram->pp_banks        = NULL;
        p_ram->p_bank_sizes    = NULL;
        p_ram->number_of_banks = 0;
    }
} /* ram_data_free_banks() */

/*!
 * @brief Allocates memory for RAM data banks.
 * @param[in, out] p_ram_data RAM bank handler.
 * @param[in]      total_size Total allocation size.
 * 
 * @return true if allocation was successful, otherwise false.
 */
static bool
ram_data_alloc_banks(ram_t * const p_ram, long const total_size)
{
#if defined(USE_BANKS)
    uint8_t number_of_banks = total_size / RAM_BANK_SIZE;
    size_t const bank_size = RAM_BANK_SIZE;

    if (total_size & RAM_BANK_OFFSET_MASK)
    {
        ++number_of_banks;
    }

    if (number_of_banks > RAM_BANK_MAX_BANKS)
    {
        return false;
    }

#if defined(RAM_DATA_DEBUG)
    printf("Allocating %u RAM banks ...\n", number_of_banks);
#endif
#else
    uint8_t const number_of_banks = 1;
    size_t  const bank_size       = total_size;
#endif

    p_ram->pp_banks = (uint8_t **)malloc(sizeof(uint8_t *) * number_of_banks);

    if (p_ram->pp_banks)
    {
        p_ram->p_bank_sizes = (size_t *)malloc(sizeof(size_t) * number_of_banks);

        if (p_ram->p_bank_sizes)
        {
            for (uint8_t bank = 0; bank < number_of_banks; ++bank)
            {
                uint8_t ** const pp_bank = &p_ram->pp_banks[bank];
                *pp_bank = (uint8_t *)malloc(bank_size);

                if (*pp_bank)
                {
                    p_ram->p_bank_sizes[bank] = bank_size;
                    ++p_ram->number_of_banks;
                }
                else
                {
                    ram_data_free_banks(p_ram);
                    break;
                }
            }
        }
        else
        {
            // Fill me in
        }
    }

    return p_ram->number_of_banks > 0;
} /* ram_data_alloc_banks() */

/*!
 * @brief Loads file into RAM banks.
 * @param[in, out] pp_ram_handle The handle to the RAM bank handler.
 * @param[in]      p_file_name   The file to load.
 * 
 * @return results of file load operation.
 */
e_ram_result_t const
ram_load_file(ram_handle_t * const pp_ram_handle, char const * const p_file_name)
{
    e_ram_result_t result = RAM_LOAD_UNABLE_TO_OPEN_FILE;

    FILE * const p_file = fopen(p_file_name, "rb");

    if (p_file != NULL)
    {
        fseek(p_file, 0, SEEK_END);
        long const file_size = ftell(p_file);
        fseek(p_file, 0, SEEK_SET);

#if defined(RAM_DATA_DEBUG)
#if defined(__WATCOMC__)
        printf("\nMemory available = %u/%u --> %u\n\n", _memmax(), _memavl(), (size_t)file_size);
#endif
#endif
        *pp_ram_handle = (ram_t *)calloc(1, sizeof(ram_t));
        if (*pp_ram_handle && ram_data_alloc_banks(*pp_ram_handle, file_size))
        {
            for (uint8_t bank = 0; bank < (*pp_ram_handle)->number_of_banks; ++bank)
            {
                uint8_t ** const pp_bank = &(*pp_ram_handle)->pp_banks[bank];
                fread(*pp_bank, 1, (*pp_ram_handle)->p_bank_sizes[bank], p_file);
            }

            result = RAM_LOAD_SUCCESS;
        }
        else
        {
            result = RAM_LOAD_INSUFFICIENT_MEMORY;
        }

        fclose(p_file);
    }
    else
    {
        *pp_ram_handle = NULL;
    }

    return result;
} /* ram_load_file() */

/*!
 * @brief Releases resources allocated.
 * param[in,out] pp_ram_handle
 */
void
ram_free(ram_handle_t * const pp_ram_handle)
{
    if (pp_ram_handle && *pp_ram_handle)
    {
        ram_data_free_banks((ram_t *)*pp_ram_handle);

        free((void *)*pp_ram_handle);
        *pp_ram_handle = NULL;
    }
} /* ram_free() */

/*!
 * @brief Gets the RAM address and the specified absolute offset.
 * @param[in] p_ram_handle  The RAM bank handler.
 * @param[in] offset        The absolute offset to get the address.
 */
uint8_t const * const
ram_get_address(ram_handle_t const p_ram_handle, uint32_t const offset)
{
    ram_t const * const p_ram = (ram_t const * const)p_ram_handle;

#if defined(USE_BANKS)
    uint8_t    const bank        = (offset >> RAM_BANK_SHIFT) & RAM_BANK_INDEX_MASK;
    banksize_t const bank_offset = offset & RAM_BANK_OFFSET_MASK;
#else
    uint8_t    const bank        = 0;
    banksize_t const bank_offset = offset;
#endif

    return &p_ram->pp_banks[bank][bank_offset];
} /* ram_get_address() */

/*!
 * @brief Seeks to the desired position within the RAM bank.
 * @param[in, out] p_ram_handle The RAM bank handler.
 * @param[in]      offset       The seek offset.
 * @param[in]      seek_origin  The seek origin.
 * @param[in, out] p_ram_bank   RAM bank information; bank variable has the desired bank number.
 */
void
ram_seek_bank(ram_handle_t p_ram_handle, int32_t const offset, e_ram_seek_origin_t const seek_origin, ram_bank_t * const p_ram_bank)
{
    ram_t * const p_ram = (ram_t * const)p_ram_handle;

    uint8_t bank = 0;

#if defined(USE_BANKS)
    banksize_t bank_offset = 0;
#else
    banksize_t bank_offset = offset;
#endif
    switch (seek_origin)
    {
        case RAM_SEEK_ORIGIN_SET:
            {
#if defined(USE_BANKS)
                bank        = (offset >> RAM_BANK_SHIFT) & RAM_BANK_INDEX_MASK;
                bank_offset = offset & RAM_BANK_OFFSET_MASK;
#endif
            }
            break;

        case RAM_SEEK_ORIGIN_CURRENT:
            {
                uint32_t const bank_absolute_position = p_ram_bank->p_current - p_ram_bank->p_start;

#if defined(USE_BANKS)
                uint32_t const abosolute_position     = ((uint32_t)p_ram_bank->bank << RAM_BANK_SHIFT) + bank_absolute_position;
                int32_t        new_absolute_position  = abosolute_position + offset;

                if (new_absolute_position < 0)
                {
                    new_absolute_position = -new_absolute_position;

                    bank        = (((uint32_t)new_absolute_position >> RAM_BANK_SHIFT) & RAM_BANK_INDEX_MASK) + 1;
                    bank_offset = RAM_BANK_SIZE - ((uint32_t)new_absolute_position & RAM_BANK_OFFSET_MASK);
                }
                else
                {
                    bank        = (new_absolute_position >> RAM_BANK_SHIFT) & RAM_BANK_INDEX_MASK;
                    bank_offset = new_absolute_position & RAM_BANK_OFFSET_MASK;
                }
#else
                int32_t new_absolute_position = bank_absolute_position + offset;

                if (new_absolute_position < 0)
                {
                    new_absolute_position = -new_absolute_position;
                }

                bank_offset = new_absolute_position;
#endif
            }
            break;

        default:
            // Does nothing; unknown seek origin
            return;
    }

    uint8_t const * const p_start = &*p_ram->pp_banks[bank];

    p_ram_bank->p_start   = p_start;
    p_ram_bank->p_end     = p_start + p_ram->p_bank_sizes[bank];
    p_ram_bank->p_current = p_start + bank_offset;
    p_ram_bank->bank      = bank;
}

/*!
 * @brief Reads an 8-bit value from the current RAM bank offset and advances to the next position.
 * @param[in,out] p_ram_handle The RAM bank handler.
 * 
 * @return value at the current RAM bank data offset.
 */
uint8_t
ram_read_uint8(ram_handle_t p_ram_handle)
{
    ram_t * const p_ram = (ram_t *)p_ram_handle;

#if defined(USE_BANKS)
    uint8_t const value = p_ram->pp_banks[p_ram->bank][p_ram->bank_offset];
#else
    uint8_t const value = p_ram->pp_banks[0][p_ram->absolute_position];
#endif

    ++p_ram->absolute_position;

#if defined(USE_BANKS)
    p_ram->bank        = (p_ram->absolute_position >> RAM_BANK_SHIFT) & RAM_BANK_INDEX_MASK;
    p_ram->bank_offset = p_ram->absolute_position & RAM_BANK_OFFSET_MASK;
#endif

    return value;
} /* ram_read_uint8() */


/*!
 * @brief Gets the desired bank
 * 
 * @param[in,out] p_ram_handle  The RAM bank handler.
 * @param[in,out] p_ram_bank    RAM bank information; bank variable has the desired bank number.
 */
void
ram_get_bank(ram_handle_t p_ram_handle, ram_bank_t * const p_ram_bank)
{
    ram_t * const p_ram = (ram_t *)p_ram_handle;

    uint8_t const         bank    = p_ram_bank->bank; // should already have next desired bank
    uint8_t const * const p_start = p_ram->pp_banks[bank];

    p_ram_bank->p_start   = p_start;
    p_ram_bank->p_end     = p_start + p_ram->p_bank_sizes[bank];
    p_ram_bank->p_current = p_start;
} /* ram_get_bank() */

/*** end of file ***/
