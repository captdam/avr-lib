#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"

UART serial;

void main (void) {
	volatile char msg[] = "This is a test message.\r\n"; //In main(), main() will never return hence will not be orerriden

	serial = uart_init (&UCSR0A, 16000000, 9600, (struct uart_init_mode){
		.rxEnable = uart_mode_manual,
		.txEnable = uart_mode_auto,
		.speed = uart_mode_speedNormal,
		.stopBits = uart_mode_stop2
	});

	sei();
	for(;;) {
		/*
		for (int i = 0; i < sizeof(msg); i++) {
			while (!uart_sendReady(&serial));
			uart_sendChar(&serial, msg[i]);
		}
		*/
		while (!uart_sendReady(&serial));
		uart_sendString(&serial, msg, sizeof(msg));
		while (uart_sendStringProgress(&serial));
	}
}

ISR (USART0_TX_vect) {
	uart_sendString_ISR(&serial);
}