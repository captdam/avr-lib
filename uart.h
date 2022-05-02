/** AVR UART lib 
 * This library provides two high level interface for AVR USART hardware: 
 * - Manual mode: one character at a time with pooling (busy-wait); and 
 * - Auto mode: give the address, this library will use interrupt to send/receive a string of characters, while the application can use the CPU to perform other tasks. 
 * This library has not implement a few options provided by the hardware, hence the limitation: 
 * - Always UART, no USART, no master SPI; and
 * - Always use 8-bit data; and
 * - No parity checking and frame error detection (not like I2C where the receiver can ack/nak the sender immediately), the user may use checksum/CRC and request the sender to resend in case of error; and
 * - Hardware multi-processor communication mode is not supported for now.
 */

#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <avr/interrupt.h>

enum uart_mode_enable { uart_mode_disable = 0, uart_mode_manual = 1, uart_mode_auto = 2 };
enum uart_mode_parity { uart_mode_noParity = 0, uart_mode_evenParity = 2, uart_mode_OddParity = 3 };
enum uart_mode_stop { uart_mode_stop1 = 0, uart_mode_stop2 = 1 };
enum uart_mode_speed { uart_mode_speedNormal = 0, uart_mode_speedDouble = 1 };

/** UART class data. 
 * Do NOT directly modify/read!
 */
typedef struct UART {
	volatile uint8_t * baseAddr; //UCSRnA
	volatile enum uart_mode_enable tx_enable : 2;
	volatile enum uart_mode_enable rx_enable : 2;
	volatile uint8_t * tx_ptr, * tx_end;
	volatile uint8_t * rx_ptr, * rx_end, * rx_addr;
} UART;

struct uart_init_mode {
	enum uart_mode_enable txEnable : 2;
	enum uart_mode_enable rxEnable : 2;
	enum uart_mode_stop stopBits : 1;
	enum uart_mode_speed speed : 1;
};

#define SFR_OFFSET_DATA 6
#define SFR_OFFSET_BAUD 4 //12-bit right-align
#define SFR_OFFSET_CFGC 2
#define SFR_OFFSET_CFGB 1
#define SFR_OFFSET_CFGA 0

UART uart_init (volatile void * sfr_base, uint32_t f_cpu, uint16_t baud, struct uart_init_mode config) {
	UART uart = {.baseAddr = sfr_base};
	
	uint16_t clk = config.speed ? ( f_cpu / 8 / baud - 1 ) : ( f_cpu / 16 / baud - 1 );
	uart.baseAddr[SFR_OFFSET_BAUD+0] = clk >> 0;
	uart.baseAddr[SFR_OFFSET_BAUD+1] = clk >> 8;

	uart.baseAddr[SFR_OFFSET_CFGA] |= (config.speed << U2X0);
	
	uart.tx_enable = config.txEnable;
	if (config.txEnable == uart_mode_manual)
		uart.baseAddr[SFR_OFFSET_CFGB] |= (1 << TXEN0);
	else if (config.txEnable == uart_mode_auto)
		uart.baseAddr[SFR_OFFSET_CFGB] |= (1 << TXEN0) | (1 << TXCIE0);
	
	uart.rx_enable = config.rxEnable;
	if (config.rxEnable == uart_mode_manual)
		uart.baseAddr[SFR_OFFSET_CFGB] |= (1 << RXEN0);
	else if (config.rxEnable == uart_mode_auto)
		uart.baseAddr[SFR_OFFSET_CFGB] |= (1 << RXEN0) | (1 << RXCIE0);
	
	uart.baseAddr[SFR_OFFSET_CFGC] |= (config.speed << USBS0);

	return uart;
}

/** Check the UART tx data register. 
 * @param uart UART object returned by uart_init()
 * @return Non-zero if tx data register is empty, ready to send data; 0 if tx data register is not empty, should wait or do something else 
 */
uint8_t uart_sendReady (UART * uart) {
	return uart->baseAddr[SFR_OFFSET_CFGA] & (1 << UDRE0);
}

/** Send one character. 
 * Good for testing purpose or payload is small. 
 * Check the buffer before send with uart_readyToSend(), only send if the buffer is empty. 
 * @param uart UART object returned by uart_init()
 */
void uart_sendChar (UART * uart, uint8_t c) {
	uart->baseAddr[SFR_OFFSET_DATA] = c;
}

/** Use ISR to send a string of character. 
 * Higher performance than uart_sendChar() when sending multiple characters. 
 * The string must be saved in memory because it needs to be accessible in the ISR, hence be volatile. 
 * It is recommanded to use dedicated space to save the string, e.g., in global space. 
 * If the string is saved in stack, make sure it will not be overridden after current function returned prior the sring is fully sent. 
 * Check the buffer before send with uart_readyToSend(), only send if the buffer is empty. 
 * @param uart UART object returned by uart_init()
 * @param c Address of the string, DO NOT remove the volatile qualifier
 * @param size Size of the string in bytes
 */
void uart_sendString (UART * uart, volatile uint8_t * c, uint16_t size) {
	uart->baseAddr[SFR_OFFSET_DATA] = *c;
	uart->tx_ptr = c;
	uart->tx_end = c + size;
}

/** Put this in the ISR if you may need to use the auto send uart_sendString() function. 
 * @param uart UART object returned by uart_init()
 * @return Number of character left to send
 */
uint16_t uart_sendString_ISR (UART * uart) {
	uart->tx_ptr++;
	if (uart->tx_ptr != uart->tx_end)
		uart->baseAddr[SFR_OFFSET_DATA] = *uart->tx_ptr;
	return uart->tx_end - uart->tx_ptr;
}

/** Get number of character left to send, this result is valid only if use auto send uart_sendString() function. 
 * @return Number of character left to send, in bytes
 */
uint16_t uart_sendStringProgress (UART * uart) {
	return uart->tx_end - uart->tx_ptr;
}

/** Check for new data in receiver. 
 * @param uart UART object returned by uart_init()
 * @return Non-zero if data avirable; 0 if buffer empty
 */
uint8_t uart_receiveReady (UART * uart) {
	return uart->baseAddr[SFR_OFFSET_CFGA] & (1 << RXC0);
}

/** Read a character from receiver. 
 * @param uart UART object returned by uart_init()
 */
uint8_t uart_receiveFetch (UART * uart) {
	return uart->baseAddr[SFR_OFFSET_DATA];
}

/** Assign a buffer space for receiver. 
 * @param uart UART object returned by uart_init()
 */
void uart_receiveSpace (UART * uart, volatile uint8_t * address, uint16_t size) {
	uart->rx_addr = address;
	uart->rx_end = address + size;
}

/** Reset the receiver pointer. 
 * @param ptr New pointer, must be greater or equal to the buffer space address and less than address+size
 */
void uart_receiveReset (UART * uart, volatile uint8_t * ptr) {
	uart->rx_ptr = ptr;
}

/** Get the current receiver pointer. 
 * @return Current receiver pointer
 */
volatile uint8_t * uart_receivGetptr (UART * uart) {
	return uart->rx_ptr;
}

#undef SFR_OFFSET_DATA
#undef SFR_OFFSET_BAUD
#undef SFR_OFFSET_CFGC
#undef SFR_OFFSET_CFGB
#undef SFR_OFFSET_CFGA

#endif /*#ifndef UART_H*/