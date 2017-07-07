#include "main.h"

extern "C" {

int main (void){
    ayplayer_clock_init();
    ayplayer_port_init();
    ayplayer_spi_init();
    ayplayer_housekeeping_init();
    ayplayer_gui_init();
    ayplayer_microsd_card_init();

    tim_ay_clk.reinit();
    tim_ay_clk.on();

    vTaskStartScheduler();
    while ( true ) {};
}

}
