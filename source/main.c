/** @file main.c
 *
 * @brief PCZSM main
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "irq.h"
#include "ram.h"
#include "zsm.h"
#include "ym2151.h"
#include "saa1099.h"
#include "saaym.h"
#include "timer.h"
#include "keyboard.h"

#ifdef _DEBUG
    #define PCZSM_TARGET " development"
#else
    #define PCZSM_TARGET
#endif

#if defined(_M_I86)
    #define PCZSM_ARCHITECTURE 16
#else
    #define PCZSM_ARCHITECTURE 32
#endif

#define PCZSM_STRINGIZE(arg)         #arg
#define PCZSM_STRING(arg)            PCZSM_STRINGIZE(arg)
#define PCZSM_APP_NAME               ("PCZSM")
#define PCZSM_VERSION                "v1.0"
#define PCZSM_PLATFORM               PCZSM_STRING(PCZSM_ARCHITECTURE)"-bit"
#define PCZSM_BUILD_VERSION          (PCZSM_VERSION " (" PCZSM_PLATFORM PCZSM_TARGET ")")
#define PCZSM_AUTHORS                ("OPLx")
#define PCZSM_FILENAME               ("FILENAME.ZSM")

/*!
 * @brief Plays the ZSM file specified by the filename.
 * 
 * @param[in] p_saaym_config SAAYM configuration information.
 * @param[in] p_file_name    ZSM file name.
 * @param[in] zsm_repeat     ZSM repeat flags/information.
 */
static void
play_zsm(saaym_config_t const * const p_saaym_config, char const * const p_file_name, uint16_t const zsm_repeat)
{
    ym2151_initialize(p_saaym_config->base_io_port + SAAYM_PORT_OFFSET_YM2151, p_saaym_config->ym2151_clock);
    saa1099_vera_psg_initialize(p_saaym_config->base_io_port, p_saaym_config->saa1099_clock);

    zsm_header_t const header = zsm_get_header();

    printf("\rPlaying %s\n", p_file_name);
    printf("Version   : %u\n", header.version);
    printf("Tick Rate : %s timer @ %2uHz\n", (p_saaym_config->irq_number == 0) ? "SYSTEM" : "YM2151", header.tick_rate);
    printf("Loop      : 0x%04X(0x%02X)\n\n", header.loop_point.address, header.loop_point.bank);

    timer_start(p_saaym_config->irq_number, header.tick_rate, &zsm_update);

    zsm_start(zsm_repeat);

    while (zsm_is_playing())
    {
        if (keyboard_get_state_pcxt_bios(KEYBOARD_SCANCODE_ESC))
        {
            zsm_stop();
        }
    }

    timer_stop();

    saa1099_vera_psg_terminate();
    ym2151_terminate();
} /* play_zsm() */

/*!
 * @brief Main ZSM playback handler.
 * 
 * @param[in] p_file_name ZSM file name.
 * @param[in] zsm_repeat  ZSM repeat flags/information.
 */
static void
zsm_main(char const * const p_file_name, uint16_t const zsm_repeat)
{
    saaym_config_t const saaym_config = saaym_detect(true);

#ifdef PCZSM_DEBUG
    printf("SAAYM at: 0x%X, IRQ: %d, F: %d, P: %d\n", saaym_config.base_io_port, saaym_config.irq_number, saaym_config.ym2151_clock, saaym_config.saa1099_clock);
#endif

    if (saaym_config.base_io_port > 0)
    {
        printf("Loading %s", p_file_name);

        ram_handle_t zsm_ram_handle = NULL;
        e_ram_result_t const ram_load_result = ram_load_file(&zsm_ram_handle, p_file_name);

        if (ram_load_result != RAM_LOAD_SUCCESS)
        {
            printf("\n");
        }

        switch(ram_load_result)
        {
            case RAM_LOAD_SUCCESS:
                {
                    ym2151_write_func_t   p_ym2151_write_func   = (saaym_config.ym2151_clock  == YM2151_CLOCK_INVALID)  ? NULL : &ym2151_write;
                    vera_psg_write_func_t p_vera_psg_write_func = (saaym_config.saa1099_clock == SAA1099_CLOCK_INVALID) ? NULL : &saa1099_vera_psg_write;

                    e_zsm_result_t const zsm_result = zsm_initialize(zsm_ram_handle, p_ym2151_write_func, p_vera_psg_write_func);

                    if (zsm_result != ZSM_SUCCESS)
                    {
                        printf("\n");
                    }

                    switch (zsm_result)
                    {
                        case ZSM_SUCCESS:
                            play_zsm(&saaym_config, p_file_name, zsm_repeat);
                            break;

                        case ZSM_BAD_DATA_POINTER:
                            printf("Bad data pointer!");
                            break;

                        case ZSM_UNSUPPORTED_VERSION:
                            printf("Version %d ZSM files currently not supported.\n", zsm_get_header().version);
                            break;

                        case ZSM_NOTHING_TO_PLAY:
                            printf("Nothing to play; no YM2151, VERA PSG data, or suitiable hardware to play on.\n");
                            break;

                        default:
                            printf("Unknown error: %d\n", zsm_result);
                            break;
                    }
                }
                break;

            case RAM_LOAD_UNABLE_TO_OPEN_FILE:
                {
                    printf("Unable to open %s!\n", p_file_name);
                }
                break;

            case RAM_LOAD_INSUFFICIENT_MEMORY:
                {
                    printf("Insufficient memory to load file!\n");
                }
                break;

            default:
                printf("ram_load_file returned unknown value of %d\n", ram_load_result);
                break;
        }

        ram_free(&zsm_ram_handle);
    }
    else
    {
        printf("TexElec SAAYM not found!\n");
    }
} /* zsm_main() */

int 
main(int argc, char ** argv)
{
    if (argc < 2)
    {
        printf("Usage:  %s %s\n", PCZSM_APP_NAME, PCZSM_FILENAME);
    }
    else
    {
        printf("%s %s by %s\n\n", PCZSM_APP_NAME, PCZSM_BUILD_VERSION, PCZSM_AUTHORS);

        if (strncmp(argv[1], "-h", 2) == 0)
        {
            printf("Usage: %s%d [options] %s\n\n", PCZSM_APP_NAME, PCZSM_ARCHITECTURE, PCZSM_FILENAME);
            printf("Where options can be:\n");
            printf("-rN\tRepeat N times (if the ZSM repeats). -r only to repeat forever.\n");
        }
        else
        {
            uint16_t zsm_repeat = 0;

            for (size_t argc_index = 1; argc_index < (size_t)argc-1; ++argc_index)
            {
                char const * const p_argv = argv[argc_index];

                if (p_argv)
                {
                    if ((zsm_repeat == 0) && (strncmp(p_argv, "-r", 2) == 0))
                    {
                        char const * const p_count = &p_argv[2];
                        if (*p_count)
                        {
                            zsm_repeat = (uint8_t)atoi(p_count); // get repeat count, default to 1 if no value was specified.
                        }
                        else
                        {
                            // No repeat count; repeat forever
                            zsm_repeat = ZSM_REPEAT_FOREVER;
                        }
                    }
                }
            }

            zsm_main(argv[argc - 1], zsm_repeat);
        }
    }

    return 0;
} /* main() */

/*** end of file ***/
