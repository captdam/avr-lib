/** LCD1602 driver with MCU-side buffer. 
 * Use 4-bit D-bus. 
 * User should define following macro before include this lib: 
 * LCD1602_DPORT:  Which port does the D-bus connected to? 
 * LCD1602_DPIN:   Which pins of that port does the D-bus connected to? 0 for 0-3, 4 for 4-7.
 * LCD1602_RSPORT: Which port dees RS signal connected to? 
 * LCD1602_RSPIN:  Which pin of that port dees RS signal connected to? 
 * LCD1602_RWPORT: Which port does RW signal connected to? (DO not define if connected to GND) 
 * LCD1602_RWPIN:  Which pin of that port dees RW signal connected to? (DO not define if connected to GND) 
 * LCD1602_ENPORT: Which port does EN signal connected to? 
 * LCD1602_ENPIN:  Which pin of that port dees EN signal connected to? 
 */

/** Init LCD1602 module usign lib config. 
 */
void lcd1602_init();

/** Write a character into the buffer. 
 * @param row Which row: 0 or 1
 * @param column Which column: 0 to 15
 * @param data Charactor to write
 */
void lcd1602_writec(uint8_t row, uint8_t column, char data);

/** Write a string into the buffer. 
 * User should make sure string will not exceed the buffer: column + cnt <= 16
 * @param row Which row to start: 0 or 1
 * @param column Which column to start: 0 to 15
 * @param cnt Number of character in the string
 * @param data String to write
 */
void lcd1602_writes(uint8_t row, uint8_t column, uint8_t cnt, char* data);

/** Read from buffer character by character. 
 * Call this in a time event (lower than 25kHz) to refresh the LCD. 
 * This routine will send data/cmd to LCD1602 module. This routine also step the internal buffer 
 * index so it will read next character in the buffer next time calling this routine. 
 */
void lcd1602_evt();

/** TODO:
 * Read busy flag
 * 
 */