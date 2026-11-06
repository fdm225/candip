# Project Guidelines
- MCU: Microchip/Atmel ATmega162
- Clock: 16.000 MHz (F_CPU=16000000UL)
- Toolchain: avr-gcc / GNU Make
- Architecture: Bare-metal C with memory-mapped CAN controllers (SJA1000, TJA1054).
- Prefer deterministic polled timing or lightweight ISRs; avoid dynamic memory allocation (malloc).