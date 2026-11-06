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
PORT      ?= $(firstword $(wildcard /dev/cu.usbserial-*))
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

size:
	@avr-size --format=avr --mcu=atmega162 $(TARGET).elf 2>nul || avr-size -B $(TARGET).elf 2>/dev/null || avr-size $(TARGET).elf

clean:
	rm -f $(OBJ) $(TARGET).elf $(TARGET).hex $(TARGET).map

flash: $(TARGET).hex
	@if [ -z "$(PORT)" ]; then echo "Error: No /dev/cu.usbserial-* device found. Plug in the programmer or specify PORT=/dev/cu.xxx"; exit 1; fi
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -V -U flash:w:$<:i

# Compare the chip's flash against canTest.hex (use after a flash that did not boot)
verify: $(TARGET).hex
	@if [ -z "$(PORT)" ]; then echo "Error: No /dev/cu.usbserial-* device found. Plug in the programmer or specify PORT=/dev/cu.xxx"; exit 1; fi
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -U flash:v:$<:i

.PHONY: all clean flash verify size