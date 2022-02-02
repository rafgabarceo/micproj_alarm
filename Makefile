CC=avr-gcc
CFLAGS= -Os -DF_CPU=16000000UL -mmcu=atmega328p
LIB= i2c_master.c liquid_crystal_i2c.c 

all: main.hex

i2c_master.o: ./c/i2c_master.c
	avr-gcc -c -Os -DF_CPU=16000000UL -mmcu=atmega328p c/i2c_master.c -o i2c_master.o

liquid_crystal_i2c.o: ./c/liquid_crystal_i2c.c
	avr-gcc -c -Os -DF_CPU=16000000UL -mmcu=atmega328p c/liquid_crystal_i2c.c -o liquid_crystal_i2c.o

ds1302.o: ./c/ds1302.c
	avr-gcc -c -Os -DF_CPU=16000000UL -mmcu=atmega328p c/ds1302.c -o ds1302.o

main.o: ./c/main.c
	avr-gcc -c -Os -DF_CPU=16000000UL -mmcu=atmega328p c/main.c -o main.o 

main.out: main.o liquid_crystal_i2c.o i2c_master.o ds1302.o
	avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p i2c_master.o liquid_crystal_i2c.o ds1302.o main.o -o main.out

main.hex: main.out
	avr-objcopy -O ihex -R .eeprom main.out main.hex

install: main.hex
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:main.hex

clean:
	rm ds1302.o i2c_master.o liquid_crystal_i2c.o main.hex main.o main.out