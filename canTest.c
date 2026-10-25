#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "USART0.h"
#include "SJA1000.h"
#include "TJA1054.h"

int main(void)
{
    USART0_Init(BAUD115200);
    CAN_Reg_Init();
    TJA1054_Init();

    /* 100 kbps for Audi Infotainment bus */
    SJA1000_Init(0x03, 0x4D, READWRITE);

    /* Disable interrupts to keep polled timing deterministic */
    GICR &= ~(1 << INT0);
    CAN_INTEN = 0x00;
    sei();

    printf("\nEmulating OEM Concert II Radio\n");

    uint32_t loop_count = 0;

    while (1) {
        /* 1. OSEK Direct Network Management - Radio Alive (ID 0x439, DLC 6) */
        CAN_Transmit(0x439, 6, 0x19, 0x02, 0x40, 0x02, 0x00, 0x00, 0, 0);
        _delay_ms(5);

        /* 2. Cluster Display Allocation Control (ID 0x660, DLC 8) */
        CAN_Transmit(0x660, 8, 0x01, 0x01, 0x4A, 0x19, 0x2E, 0x27, 0x00, 0x00);
        _delay_ms(5);

        /* 3. Display Handshake / Status (ID 0x661, DLC 8) */
        CAN_Transmit(0x661, 8, 0x83, 0x01, 0x12, 0xA0, 0x00, 0x00, 0x00, 0x00);
        _delay_ms(5);

        /* 4. Display Sub-channel Status (ID 0x663 & 0x664, DLC 8) */
        CAN_Transmit(0x663, 8, 0x01, 0x01, 0x12, 0xA0, 0x00, 0x00, 0x12, 0x00);
        _delay_ms(5);
        CAN_Transmit(0x664, 8, 0x01, 0x02, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00);
        _delay_ms(5);

        /* 5. Radio Text: Top Row (ID 0x261, 8 ASCII chars) */
        CAN_Transmit(0x261, 8, 'H', 'E', 'L', 'L', 'O', ' ', ' ', ' ');
        _delay_ms(5);

        /* 6. Radio Text: Second Row (ID 0x263, 8 ASCII chars) */
        CAN_Transmit(0x263, 8, 'A', 'U', 'D', 'I', ' ', ' ', 'T', 'T');

        loop_count++;
        if ((loop_count % 5) == 0) {
            printf("Radio loops: %lu\n", loop_count);
        }

        /* Replicate OEM Radio ~800 ms cadence */
        _delay_ms(770);
    }

    return 0;
}