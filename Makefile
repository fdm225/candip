MCU     = atmega162
F_CPU   = 16000000UL
TARGET  = canTest

SRC     = canTest.c \
          USART0.c \
          BAUDRATES.c \
          SJA1000_Init.c \
          TJA1054_Init.c \
          SJA1000_Interrupt.c \
          SJA1000_Recieve.c \
          SJA1000_Transmit.c

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size
AVRDUDE = avrdude

# Programmer settings (adjust to match your programmer e.g. usbasp, avrisp2, etc.)
PROGRAMMER = avrisp
PORT       = /dev/cu.usbserial-1220
BAUD       = 19200

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra -std=gnu99
LDFLAGS = -mmcu=$(MCU)

OBJ     = $(SRC:.c=.o)

all: $(TARGET).hex size

$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

size: $(TARGET).elf
	$(SIZE) --format=avr --mcu=$(MCU) $<

clean:
	rm -f $(OBJ) $(TARGET).elf $(TARGET).hex $(TARGET).map

flash: $(TARGET).hex
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -V -U flash:w:$<:i

.PHONY: all clean flash size