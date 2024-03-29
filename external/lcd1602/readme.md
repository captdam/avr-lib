# LCD1602 lib

## Use this lib

This lib assume user connects the LCD1602 module to MCU using 4-bit bus.

This lib has been tested on:
- [X] ATmega328/P
- [X] ATtiny44

This lib should be able to run on all 8-bit AVR MCUs. Note that, this lib relies on timer event to send data to LCD module; however, different MCU model may has different timer SFR layout, user needs to check the manual to configue the timer. For example, timer 0 SFRs on m328 and t44 has same name, so the C code of setting up the timer can be used on both platform; however, the actual address is different, and the name of ISR (using gcc-avr) is different.

User should first init this lib with ```lcd1602_init()```;

This lib allows the user to write content into lib buffer, with:
- ```lcd1602_writec(row, column, data)``` writes a character into the buffer;
- ```lcd1602_writes(row, column, cnt, data)``` writes a string into the buffer.

A time event (lower than 25kHz) should include ```lcd1602_evt()```. This call will automatically send the content to device one character at a time. 

See ```main.c``` for example. 

## Compile the code

When compile, user needs to define which pins are connected and CPU speed, example compile cmd is:
```
avr-gcc *.c -mmcu=atmega328 -O3 -o a.bin \
-DF_CPU=8000000UL \
-DLCD1602_DPORT=PORTD  -DLCD1602_DPIN=4  \
-DLCD1602_RSPORT=PORTD -DLCD1602_RSPIN=2 \
-DLCD1602_RWPORT=PORTD -DLCD1602_RWPIN=3 \
-DLCD1602_ENPORT=PORTB -DLCD1602_ENPIN=1 \
 \
&& avr-objdump -S a.bin > a.asm \
```
See ```lcd1602.h``` for detail description of the compile cmd.
