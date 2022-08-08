#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

#include "lcd1602.h"

volatile char buffer[2][16];
volatile uint8_t rp;

void cmd(uint8_t cmd);
void dat(uint8_t dat);

void lcd1602_init() {
	cmd(0x01); //Clear display
	_delay_ms(3);
	cmd(0x02); //Return home
	_delay_ms(3);
	cmd(0x04 | (1<<1) | (0<<0)); //Mode: Increment cursor; shift display
	_delay_us(50);
	cmd(0x08 | (1<<2) | (0<<1) | (0<<0)); //Display: display; cursor; blibnk
	_delay_us(50);
	cmd(0x20 | (0<<4) | (1<<3) | (0<<2)); //Function: 8/4-bit D-bus; 2/1-line; big/small font
	_delay_us(50);

	for (uint8_t i = 0; i < 16; i++) {
		buffer[0][i] = ' ';
		buffer[1][i] = ' ';
	}
	rp = 0;
}

void lcd1602_writec(uint8_t row, uint8_t column, char data) {
	buffer[row][column] = data;
}

void lcd1602_writes(uint8_t row, uint8_t column, uint8_t cnt, char* data) {
	for (uint8_t i = 0; i < cnt; i++)
		buffer[row][column+i] = data[i];
}

void lcd1602_evt() {
	if (rp == 0x10) { //End of row 0
		cmd(0x80 | 0x40); //LCD1602 row 1 DDRAM addr
		rp = 0x20;
	} else if (rp == 0x30) { //End of row 1
		cmd(0x80 | 0x00); //LCD1602 row 0 DDRAM addr
		rp = 0x00;
	} else {
		dat(buffer[ (rp & 0x20) >> 5 ][ rp & 0x0F ]);
		rp++;
	}
}

#define lcd1602_delay() _delay_us(0.2);

void cmd(uint8_t cmd) {
	LCD1602_ENPORT &= ~(1<<LCD1602_ENPIN); //When disable, setup RS and pull RW low for writing
	LCD1602_RSPORT &= ~(1<<LCD1602_RSPIN);
	#ifdef LCD1602_RWPORT
		LCD1602_RWPORT &= ~(1<<LCD1602_RWPIN);
	#endif
	lcd1602_delay(); //delay to setup mode
	LCD1602_ENPORT |= (1<<LCD1602_ENPIN); //Enable LCD
	#if LCD1602_DPIN == 4
		uint8_t dh = cmd & 0xF0; //0bdddd0000
		LCD1602_DPORT = (LCD1602_DPORT & 0x0F) | dh; //Keep low bibble
	#else
		uint8_t dh = (cmd & 0xF0) >> 4; //0b0000dddd
		LCD1602_DPORT = (LCD1602_DPORT & 0xF0) | dh; //Keep high bibble
	#endif
	lcd1602_delay(); //delay to setup cmd
	LCD1602_ENPORT &= ~(1<<LCD1602_ENPIN); //Disable
	lcd1602_delay(); //delay to save cmd
	lcd1602_delay(); //delay to setup mode
	LCD1602_ENPORT |= (1<<LCD1602_ENPIN); //Enable LCD
	#if LCD1602_DPIN == 4
		uint8_t dl = (cmd & 0x0F) << 4;
		LCD1602_DPORT = (LCD1602_DPORT & 0x0F) | dl;
	#else
		uint8_t dl = cmd & 0x0F;
		LCD1602_DPORT = (LCD1602_DPORT & 0xF0) | dl;
	#endif
	lcd1602_delay(); //delay to setup cmd
	LCD1602_ENPORT &= ~(1<<LCD1602_ENPIN); //Disable
	lcd1602_delay(); //delay to save cmd
}

void dat(uint8_t dat) {
	LCD1602_ENPORT &= ~(1<<LCD1602_ENPIN);
	LCD1602_RSPORT |= (1<<LCD1602_RSPIN);
	#ifdef LCD1602_RWPORT
		LCD1602_RWPORT &= ~(1<<LCD1602_RWPIN);
	#endif
	lcd1602_delay(); //delay to setup mode
	LCD1602_ENPORT |= (1<<LCD1602_ENPIN); //Enable LCD
	#if LCD1602_DPIN == 4
		uint8_t dh = dat & 0xF0; //0bdddd0000
		LCD1602_DPORT = (LCD1602_DPORT & 0x0F) | dh; //Keep low bibble
	#else
		uint8_t dh = (dat & 0xF0) >> 4; //0b0000dddd
		LCD1602_DPORT = (LCD1602_DPORT & 0xF0) | dh; //Keep high bibble
	#endif
	lcd1602_delay(); //delay to setup data
	LCD1602_ENPORT &= ~(1<<LCD1602_ENPIN); //Disable
	lcd1602_delay(); //delay to save data
	lcd1602_delay(); //delay to setup mode
	LCD1602_ENPORT |= (1<<LCD1602_ENPIN); //Enable LCD
	#if LCD1602_DPIN == 4
		uint8_t dl = (dat & 0x0F) << 4;
		LCD1602_DPORT = (LCD1602_DPORT & 0x0F) | dl;
	#else
		uint8_t dl = dat & 0x0F;
		LCD1602_DPORT = (LCD1602_DPORT & 0xF0) | dl;
	#endif
	lcd1602_delay(); //delay to setup data
	LCD1602_ENPORT &= ~(1<<LCD1602_ENPIN); //Disable
	lcd1602_delay(); //delay to save data
}
