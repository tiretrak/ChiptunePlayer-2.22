#include "ayplayer_clock.h"
#include <stdlib.h>

const rcc_cfg ay_player_clock_cfg[] = {
    {
        . osc_cfg = {
            .OscillatorType = RCC_OSCILLATORTYPE_HSE,
            .HSEState       = RCC_HSE_ON,
            .LSEState       = RCC_LSE_BYPASS,
            .HSIState       = RCC_HSI_OFF,
            .HSICalibrationValue    = RCC_HSICALIBRATION_DEFAULT,
            .LSIState               = RCC_LSI_OFF,
            .PLL                    = {                                         // Не используется. Значения на угад в реальном диапазоне.
                .PLLState               = RCC_PLL_NONE,
                .PLLSource              = RCC_PLLSOURCE_HSE,
                .PLLM                   = 0,
                .PLLN                   = 192,
                .PLLP                   = RCC_PLLP_DIV2,
                .PLLQ                   = 2
            }
        },
        .clk_cfg = {
            .ClockType      = RCC_CLOCKTYPE_HCLK,
            .SYSCLKSource   = RCC_SYSCLKSOURCE_HSE,
            .AHBCLKDivider  = RCC_SYSCLK_DIV4,
            .APB1CLKDivider = RCC_HCLK_DIV1,
            .APB2CLKDivider = RCC_HCLK_DIV1,
        },
        .f_latency = FLASH_LATENCY_0
    }
};

rcc ayplayer_rcc( ay_player_clock_cfg, 1 );

void ayplayer_clock_init ( void ) {
    if ( ayplayer_rcc.set_cfg( 0 ) != RCC_RESULT::OK ) {
    	abort();
    }
}
