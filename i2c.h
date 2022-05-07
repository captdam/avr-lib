

#include <avr/io.h>
#include <avr/interrupt.h>

enum i2c_flag { i2c_flag_holdControl = 1, i2c_flag_retry = 2 };

enum i2c_state { //Next task
	i2c_state_unknown = -1, i2c_state_free = 0,
	i2c_state_masterWrite, i2c_state_masterRead,
	i2c_state_error = -2
};

enum i2c_status { //AVR TWSR code
	i2c_status_master_start = 0x08, i2c_status_master_repeatedStart = 0x10, i2c_status_master_lost = 0x38,
	i2c_status_masterWrite_addrAck = 0x18, i2c_status_masterWrite_addrNak = 0x20,
	i2c_status_masterWrite_dataAck = 0x28, i2c_status_masterWrite_dataNak = 0x30,
	i2c_status_masterRead_addrAck = 0x40, i2c_status_masterRead_addrNak = 0x48,
	i2c_status_masterRead_dataAck = 0x50, i2c_status_masterRead_dataNak = 0x58,
	i2c_status_free = 0xF8, i2c_status_error = 0x00
};

volatile struct I2C{
	volatile enum i2c_state state;
	volatile uint8_t status; //Hardware status register (TWSR), record
	volatile enum i2c_flag flag;

	volatile uint8_t deviceAddr;
	volatile uint8_t * volatile dataStart, * volatile dataPtr, * volatile dataEnd;
} i2c = {
	.state = i2c_state_unknown
};

/** Init I2C. 
 * Set the bitrate of the I2C bus. 
 * @param f_cpu CPU frequency
 * @param f_i2c I2C bitrate
 */
void i2c_init(uint32_t f_cpu, uint32_t f_i2c) {
	i2c.state = i2c_state_free;
	TWBR = ( f_cpu / f_i2c - 16 ) / 2; //SCL frequency = CPU frequency / (16 + 2 * TWBR)
}

/** Use ISR to send a string of character on I2C. 
 * When the transaction is finished, i2c_getState becomes i2c_state_free. Use i2c_progress() to check the progress. 
 * The string must be saved in memory because it needs to be accessible in the ISR, hence be volatile. 
 * It is recommanded to use dedicated space to save the string, e.g., in global space. 
 * If the string is saved in stack, make sure it will not be overridden after current function returned prior the sring is fully sent. 
 * @param addr Slave addresss (0-127)
 * @param flag ORed enum i2c_flag to set the behaviour of this transaction
 * @param data A pointer to the data to send, DO NOT remove the volatile qualifier
 * @param size Size of the string in bytes
 */
void i2c_master_write(uint8_t addr, enum i2c_flag flag, volatile uint8_t * volatile data, uint16_t size) {
	i2c.state = i2c_state_masterWrite;
	i2c.flag = flag;
	i2c.deviceAddr = (addr << 1);
	i2c.dataStart = data;
	i2c.dataPtr = data;
	i2c.dataEnd = data + size;
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);

}

/** Use ISR to receive a string of character on I2C. 
 * When the transaction is finished, i2c_getState becomes i2c_state_free. Use i2c_progress() to check the progress. 
 * The string must be saved in memory because it needs to be accessible in the ISR, hence be volatile. 
 * It is recommanded to use dedicated space to save the string, e.g., in global space. 
 * If the string is saved in stack, make sure it will not be overridden after current function returned prior the sring is fully sent. 
 * @param addr Slave address (0-127)
 * @param flag ORed enum i2c_flag to set the behaviour of this transaction
 * @param data A pointer to the space to save the data, DO NOT remove the volatile qualifier
 * @param size Size of the string in bytes
 */
void i2c_master_read(uint8_t addr, enum i2c_flag flag, volatile uint8_t * volatile data, uint16_t size) {
	i2c.state = i2c_state_masterRead;
	i2c.flag = flag;
	i2c.deviceAddr = (addr << 1) | 1;
	i2c.dataStart = data;
	i2c.dataPtr = data;
	i2c.dataEnd = data + size;
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
}

/** Get current I2C status (from I2C hardware). 
 * @return Current status
 */
enum i2c_status i2c_getStatus() {
	return i2c.status;
}

/** Get current I2C state (from software register). 
 * @return Current state
 */
enum i2c_state i2c_getState() {
	return i2c.state;
}

/** Get number of character left to write or read. 
 * Non-zero value when i2c_getState is i2c_state_free indicates error. Use i2c_getStatus() to analysis. 
 * @return Number of character left to send or read, in bytes
 */
uint16_t i2c_getProgress() {
	return i2c.dataEnd - i2c.dataPtr;
}



ISR(TWI_vect) {
	i2c.status = TWSR & 0xF8;
	switch (TWSR & 0xF8) {
		case i2c_status_master_start:
		case i2c_status_master_repeatedStart:
			TWDR = i2c.deviceAddr;
			TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
			break;
		
		case i2c_status_masterWrite_addrAck:
		case i2c_status_masterWrite_addrNak:
		case i2c_status_masterWrite_dataAck:
		case i2c_status_masterWrite_dataNak:
			if (i2c.dataPtr != i2c.dataEnd) { //Transmiit in progress
				TWDR = *(i2c.dataPtr++);
				TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
			} else{ //All bytes sent
				i2c.state = i2c_state_free;
				if (i2c.flag & i2c_flag_holdControl) {
					TWCR = (1 << TWEN); //Only send the data and disable interrupt, do not clear INT flag so the hardware holds the bus
				} else {
					TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); //Stop to release bus, disable interrupt to end transaction
				}
			}
			break;
		
		case i2c_status_masterRead_addrAck:
			TWCR = (1 << TWINT) | ((i2c.dataPtr != (i2c.dataEnd-1) ? 1 : 0) << TWEA) | (1 << TWEN) | (1 << TWIE); //If last byte, return NAK when receive the data to inform the slave stopping sending data
			break;
		case i2c_status_masterRead_addrNak:
			i2c.state = i2c_state_free;
			TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); //Error: Stop and release the bus
			break;
		case i2c_status_masterRead_dataAck:
			*(i2c.dataPtr++) = TWDR;
			TWCR = (1 << TWINT) | ((i2c.dataPtr != (i2c.dataEnd-1) ? 1 : 0) << TWEA) | (1 << TWEN) | (1 << TWIE);
			break;
		case i2c_status_masterRead_dataNak:
			*(i2c.dataPtr++) = TWDR;
			i2c.state = i2c_state_free;
			if (i2c.flag & i2c_flag_holdControl) {
				TWCR = (1 << TWEN); //Only read the data and disable interrupt, do not clear INT flag so the hardware holds the bus
			} else {
				TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); //Stop to release bus, disable interrupt to end transaction
			}
			break;
		
		case i2c_status_error:
			i2c.state = i2c_state_free;
			TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); //Reset internal hardware
			break;
		default:
			/* Error? */
			i2c.state = i2c_state_free;
			TWCR = (1 << TWINT) | (1 << TWEN); //Just clear the I flag
	}
}