#include "uart.h"

const uart_cfg usart1_cfg = {
	.UARTx				= USART3,
	.baudrate			= 115200,
	.mode				= UART_MODE_TX_RX,
	.dma_tx				= nullptr,
	.dma_tx_ch			= 0
};

uart usart3_obj( &usart1_cfg );

void ayplayer_usart_init ( void ) {
	usart3_obj.reinit();
	NVIC_EnableIRQ( USART3_IRQn );
}

extern "C" void USART3_IRQHandler ( void ) {
	usart3_obj.irq_handler();
}
