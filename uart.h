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

#include <stddef.h>
#include <avr/io.h>

typedef uint8_t uart_mode;
#define uart_mode_txManual	0x01 //Polling / busy-wait method on transmitter
#define uart_mode_txAuto	0x02 //Auto UART, handled by this lib and ISR on transmitter
#define uart_mode_rxManual	0x04 //Polling / busy-wait method on receiver
#define uart_mode_rxAuto	0x08 //Auto UART, handled by this lib and ISR on receiver
#define uart_mode_stop2		0x10 //Use 2 stop bits instead of 1
#define uart_mode_speedDouble	0x20 //Double speed mode, 8 clock instead of 16 clock / bit

/** UART class data. 
 * Do NOT directly modify/read! 
 * Must be stored in global sapce (define it outside of any function, define at compile time, no dynamic allocation of this variable) 
 */
typedef volatile struct UART {
	volatile uint8_t * volatile srfAddr;
	volatile uint8_t mode;
	volatile const uint8_t * volatile tx_ptr, * volatile tx_end;
	volatile uint8_t * volatile rx_ptr, * volatile rx_end, * volatile rx_addr;
} UART;

/* == Init ================================================================================== */

/** Init or reset a software UART interfacec, config the hardware, set the BAUD rate. 
 * Some MCU comes with multiple independent UART, they can have different setting. 
 * The transmitter and receiver can be set in manual or auto mode repectly and independently. 
 * If *manual and *auto mode are flaged at the same time, will use auto mode. 
 * If use auto transmitter mode, place uart_sendAuto_ISR in the USART_TX_vect or USARTn_TX_vect ISR. 
 * @param uart A UART object, pass-by-reference, must be defined in global space at compile time. 
 * @param sft_base The address of UCSRnA register of the desired UART
 * @param f_cpu The CPU speed
 * @param baud The UART BAUD rate
 * @param config ORed uart_mode_* used to set the behaviour
 * @return Actual BAUD rate the hardware will provide, may slightly differ from the baud argument
 */
void uart_init(UART * const uart, volatile void * const sfr_base, const uint32_t f_cpu, const uint16_t baud, const uart_mode mode);

/* == Sender ================================================================================ */

/** Check whether the UART transmitter is free to use. 
 * @param uart UART object returned by uart_init()
 * @return Non-zero if tx data register is empty, ready to send data; 0 if tx data register is not empty, should wait or do something else
 */
static inline uint8_t uart_sendFree(const UART * const uart);

/** Send one character manually. 
 * Good for testing purpose or payload is small. 
 * Before send, check the transmitter with uart_sendFree(), only send if it is free. 
 * @param uart UART object returned by uart_init()
 * @param data Data to send
 */
void uart_sendManual(const UART * const uart, const uint8_t data);

/** Use ISR to send a string of character automatically. 
 * This lib will take care about loading bytes into transmitter. 
 * Higher performance than uart_sendManual() when sending multiple characters. 
 * The string must be saved in memory because it needs to be accessible in the ISR (volatile). 
 * It is recommanded to use dedicated space to save the string, e.g. in global space. 
 * If the string is saved in stack, make sure it will not be overridden after current function returned prior the sring is fully sent. 
 * Before send, check the transmitter with uart_sendFree(), only send if it is free. 
 * @param uart UART object returned by uart_init()
 * @param data Address of the string, DO NOT remove the volatile qualifier
 * @param size Size of the string in bytes
 */
void uart_sendAuto(UART * const uart, volatile const uint8_t * const data, const uint16_t size);

/** Get number of character left to send, this result is valid only if use auto send with uart_sendAuto() function. 
 * @return Number of bytes left to send
 */
uint16_t uart_sendAutoProgress(const UART * const uart);

/** Put this function in the USART_TX_vect or USARTn_TX_vect ISR if you may need to use the auto send uart_sendAuto() function. 
 * @param uart UART object returned by uart_init()
 */
static inline void uart_sendAuto_ISR(UART * const uart);

/* == Receiver ============================================================================== */

/** Check for new data in receiver. 
 * @param uart UART object returned by uart_init()
 * @return Non-zero if data avirable; 0 if buffer empty
 */
uint8_t uart_receiveReady (UART * uart);

/** Read a character from receiver. 
 * @param uart UART object returned by uart_init()
 */
uint8_t uart_receiveFetch (UART * uart);

/** Assign a buffer space for receiver. 
 * @param uart UART object returned by uart_init()
 */
void uart_receiveSpace (UART * uart, volatile uint8_t * address, uint16_t size);

/** Reset the receiver pointer. 
 * @param ptr New pointer, must be greater or equal to the buffer space address and less than address+size
 */
void uart_receiveReset (UART * uart, volatile uint8_t * ptr);

/** Get the current receiver pointer. 
 * @return Current receiver pointer
 */
volatile uint8_t * uart_receivGetptr (UART * uart);

/* == Definition ============================================================================ */

#define ASM_SENDAUTO_ISR

#define SFR_DATA 6
#define SFR_BAUD 4 //12-bit right-align
#define SFR_CFGC 2
#define SFR_CFGB 1
#define SFR_CFGA 0

void uart_init(UART * const uart, volatile void * const sfr_base, const uint32_t f_cpu, const uint16_t baud, const uart_mode mode) {
	uart->srfAddr = sfr_base;
	uart->mode = 0;
	
	if (mode | uart_mode_speedDouble) {
		uint16_t clk = f_cpu / 8 / baud - 1;
		uart->srfAddr[SFR_BAUD+0] = clk >> 0;
		uart->srfAddr[SFR_BAUD+1] = clk >> 8;
		uart->srfAddr[SFR_CFGA] |= (1 << U2X0);

	} else {
		uint16_t clk = f_cpu / 16 / baud - 1;
		uart->srfAddr[SFR_BAUD+0] = clk >> 0;
		uart->srfAddr[SFR_BAUD+1] = clk >> 8;
	}

	if (mode | uart_mode_txAuto) {
		uart->srfAddr[SFR_CFGB] |= (1 << TXEN0) | (1 << TXCIE0);
		uart->mode |= uart_mode_txAuto;
	} else if (mode | uart_mode_txManual) {
		uart->srfAddr[SFR_CFGB] |= (1 << TXEN0);
		uart->mode |= uart_mode_txManual;
	}
	if (mode | uart_mode_rxAuto) {
		uart->srfAddr[SFR_CFGB] |= (1 << RXEN0) | (1 << RXCIE0);
		uart->mode |= uart_mode_rxAuto;
	} else if (mode | uart_mode_rxManual) {
		uart->srfAddr[SFR_CFGB] |= (1 << RXEN0);
		uart->mode |= uart_mode_rxManual;
	}
	
	if (mode | uart_mode_stop2) {
		uart->srfAddr[SFR_CFGC] |= (1 << USBS0);
	}
}

static inline uint8_t uart_sendFree(const UART * const uart) {
	
	return uart->srfAddr[SFR_CFGA] & (1 << UDRE0);
	
	/*uint8_t rval;
	asm volatile (
		"	LDD	YL, Z+%[sfrOffset]+0 \n"
		"	LDD	YH, Z+%[sfrOffset]+1 \n"
		"	LDD	%[ready], Y+%[sfrCfga] \n"
		"	ANDI	%[ready], %[mask]"
		: [ready] "=r"(rval)
		: [baseAddr] "z"(uart)
		, [sfrOffset] "I"(offsetof(struct UART, srfAddr))
		, [sfrCfga] "I"(SFR_CFGA)
		, [mask] "M"(1 << UDRE0)
		: "r29", "r28"
	);
	return rval;*/
}

void uart_sendManual(const UART * const uart, const uint8_t data) {
	uart->srfAddr[SFR_DATA] = data;
}

void uart_sendAuto(UART * const uart, volatile const uint8_t * const data, const uint16_t size) {
	uart->srfAddr[SFR_DATA] = *data;
	uart->tx_ptr = data;
	uart->tx_end = data + size;
}

uint16_t uart_sendAutoProgress (const UART * const uart) {
	return uart->tx_end - uart->tx_ptr;
}

static inline void uart_sendAuto_ISR (UART * const uart) {
#ifndef ASM_SENDAUTO_ISR
	uart->tx_ptr++;
	if (uart->tx_ptr != uart->tx_end)
		uart->srfAddr[SFR_DATA] = *uart->tx_ptr;
#else /* #ifndef ASM_SENDAUTO_ISR */
	asm volatile (
		//	uart->tx_ptr++;
		"	LDD	YL, Z+%[ptrOffset]+0 \n" //Get uart->tx_ptr
		"	LDD	YH, Z+%[ptrOffset]+1 \n"
		"	ADIW	YL, 1 \n"
		"	STD	Z+%[ptrOffset]+0, YL \n" //Save uart->tx_ptr++
		"	STD	Z+%[ptrOffset]+1, YH \n"
		//	if (uart->tx_ptr != uart->tx_end)
		"	LDD	r0, Z+%[endOffset]+0 \n" //Get uart->tx_end
		"	LDD	r1, Z+%[endOffset]+1 \n"
		"	CP	YL, r0 \n"
		"	CPC	YH, r1 \n" //If low-byte of ptr and end also equal, all data sent, end process
		"	BREQ	1f \n"
		//		uart->srfAddr[sfrData] = *uart->tx_ptr;
		"	LD	r0, Y \n" //Get *uart->tx_ptr
		"	LDD	YL, Z+%[sfrOffset]+0 \n"
		"	LDD	YH, Z+%[sfrOffset]+1 \n"
		"	STD	Y+%[sfrData], r0 \n"
		"1:	;; END OF PROCESS ;; \n"
		:
		: [baseAddr] "z"(uart)
		, [ptrOffset] "I"(offsetof(struct UART, tx_ptr))
		, [endOffset] "I"(offsetof(struct UART, tx_end))
		, [sfrOffset] "I"(offsetof(struct UART, srfAddr))
		, [sfrData] "I"(SFR_DATA)
		: "r29", "r28", "r1", "r0"
	);
	#undef ASM_SENDAUTO_ISR
#endif /* #ifndef ASM_SENDAUTO_ISR */
}

uint8_t uart_receiveReady (UART * uart) {
	return uart->srfAddr[SFR_CFGA] & (1 << RXC0);
}

uint8_t uart_receiveFetch (UART * uart) {
	return uart->srfAddr[SFR_DATA];
}

void uart_receiveSpace (UART * uart, volatile uint8_t * address, uint16_t size) {
	uart->rx_addr = address;
	uart->rx_end = address + size;
}

void uart_receiveReset (UART * uart, volatile uint8_t * ptr) {
	uart->rx_ptr = ptr;
}

volatile uint8_t * uart_receivGetptr (UART * uart) {
	return uart->rx_ptr;
}

#undef SFR_DATA
#undef SFR_BAUD
#undef SFR_CFGC
#undef SFR_CFGB
#undef SFR_CFGA

#endif /*#ifndef UART_H*/