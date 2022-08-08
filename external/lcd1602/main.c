/** Example code of LCD1602 lib. 
 * This example works for m328/P. 
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "lcd1602.h"

void main() {

	/* Init IO */
	DDRB = 0xFF;
	DDRD = 0xFF;
	_delay_ms(1000); //External device power-up

	/* Init timer */
	TCCR0B = (3<<CS00); //Use timer scaler 64, T0 overflow every 16k CPU tick: (488Hz at 1MHz CPU)
	TIMSK0 = (1<<TOIE0); //Enable timer0 overflow interrupt for user event
	// Other MCU may have different timer SFUs layout to config timer event

	/* Boot-up process */
	lcd1602_init();
	
	/* Init done */

	sei();

	/* Modify buffer in user thread */
	for(uint16_t i = 0; ; i++) {

		uint16_t x = i;
		uint8_t d4 = x / 10000;
		x -= d4 * 10000;
		uint8_t d3 = x / 1000;
		x -= d3 * 1000;
		uint8_t d2 = x / 100;
		x -= d2 * 100;
		uint8_t d1 = x / 10;
		x -= d1 * 10;
		uint8_t d0 = x;
		lcd1602_writec(0, 0, d4 + '0');
		lcd1602_writec(0, 1, d3 + '0');
		lcd1602_writec(0, 2, d2 + '0');
		lcd1602_writec(0, 3, d1 + '0');
		lcd1602_writec(0, 4, d0 + '0');

		char h[4];
		h[0] = ( (i & 0xF000) >> 12 ) + '0';
		h[1] = ( (i & 0x0F00) >> 8 ) + '0';
		h[2] = ( (i & 0x00F0) >> 4 ) + '0';
		h[3] = ( (i & 0x000F) >> 0 ) + '0';
		for (int j = 0; j < 4; j++) {
			if (h[j] > '9') h[j] += 7;
		}
		lcd1602_writes(1, 12, 4, h);

		_delay_ms(50);
	}
}

/* Write content in buffer to device in module thread */
ISR (TIMER0_OVF_vect) {
	lcd1602_evt(); //Update LCD module
}