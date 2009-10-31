CC=avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=atmega644p
OBJ2HEX=avr-objcopy
FILES=avr644.c serial.c sdcard.c fat32.c lcd.c
TARGET=avr644
ADFLAGS=-p m644p -c usbasp

.PHONY: fuses prog erase


prog:
	$(CC) $(CFLAGS) $(FILES) -o $(TARGET) -O
	$(OBJ2HEX) -R .eeprom -O ihex $(TARGET) $(TARGET).hex
	$(OBJ2HEX) -j .eeprop -O ihex $(TARGET) $(TARGET).eeprom
	avrdude $(ADFLAGS) -V -F -U flash:w:$(TARGET).hex:i
#	avrdude $(ADFLAGS) -U eeprom:w:$(TARGET).eeprom:i

erase:
	avrdude $(ADFLAGS) -F -e
clean:
	rm -f *.hex *.obj *.o

fuses:
	avrdude $(ADFLAGS) -F -U lfuse:w:0xE2:m #http://www.engbedded.com/cgi-bin/fc.cgi 
	avrdude $(ADFLAGS) -F -U hfuse:w:0x99:m

# avrdude $(ADFLAGS) -F -U lfuse:w:0xE2:m # 8MHz
