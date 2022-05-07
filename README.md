# avr-lib

Functions that I used for my own embedded system development.

I'm mainly working with Mega328/P and Mega2560 MCU; hence, code I wrote in this library are designed for them. However, the code should be portable to other AVR cores wuth similar architecture.

__UART__
- [X] Manual sender (Software should wait Tx complete and load next character)
- [X] Manual receiver (Software should wait Rx complete and fetch character)
- [X] Auto sender (Library reload characters from buffer space in ISR)
- [ ] Auto receiver (Library place incoming characters in a buffer space in ISR)

__I2C__
- [ ] Manual master transmitter mode (Software should wait I2C event and decide what to do)
- [ ] Manual master receiver mode (Software should wait I2C event and decide what to do)
- [ ] Manual slave transmitter mode (Software should wait I2C event and decide what to do)
- [ ] Manual slave receiver mode (Software should wait I2C event and decide what to do)
- [X] Auto master mode (Library decide what to do in ISR)
- [ ] Auto slave mode (Library decide what to do in ISR)
- [ ] I2C error

__Timer/PWM__
- [ ] ToDo