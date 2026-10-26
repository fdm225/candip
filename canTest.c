#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "USART0.h"
#include "SJA1000.h"
#include "TJA1054.h"

static int uart_putchar(char c, FILE *stream)
{
    (void)stream;
    if (c == '\n') {
        USART0_Transmit('\r');
    }
    USART0_Transmit((uint8_t)c);
    return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

int main(void)
{
    USART0_Init(BAUD115200);
    CAN_Reg_Init();
    TJA1054_Init();

    stdout = &mystdout;

    /* 100 kbps initialization for Audi Infotainment bus */
    SJA1000_Init(0x03, 0x4D, READWRITE);

    /* Disable CAN interrupts to keep synchronous testing deterministic */
    GICR &= ~(1 << INT0);
    CAN_INTEN = 0x00;
    sei();

    printf("\n==================================================\n");
    printf(" CANDIP Dual-Display Test: Top Line + Center FIS  \n");
    printf("==================================================\n");

    uint32_t loop_count = 0;

    while (1) {
        /* ========================================================== */
        /* PART 1: TOP ROW DISPLAY (Concert II Radio Emulation)       */
        /* ========================================================== */

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

        /* 5. Radio Text: Second Row / Top-Left (ID 0x263, 8 ASCII chars) */
        CAN_Transmit(0x263, 8, 'A', 'U', 'D', 'I', ' ', ' ', 'T', 'T');
        _delay_ms(5);

        /* 6. Radio Text: Top Row / Top-Right (ID 0x261, 8 ASCII chars) */
        CAN_Transmit(0x261, 8, 'H', 'E', 'L', 'L', 'O', ' ', ' ', ' ');
        _delay_ms(5);


        /* ========================================================== */
        /* PART 2: CENTER DISPLAY (Navigation DDP over TP 1.6)       */
        /* ========================================================== */

        /* 7. OSEK NM Navigation Keep-Alive (ID 0x43B, DLC 6) */
        CAN_Transmit(0x43B, 6, 0x08, 0x02, 0xC0, 0x01, 0x00, 0x00, 0, 0);
        _delay_ms(5);

        /* 8. Navigation Power Status: On = 1 (ID 0x604, DLC 8) */
        CAN_Transmit(0x604, 8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
        _delay_ms(5);

        /* 9. TP 1.6 Connect Setup to Cluster on static channel (ID 0x6C0, DLC 6) */
        /* A0 = CS, BlockSize=1, T1=82h, T2=82h, T3=41h, T4=C5h */
        CAN_Transmit(0x6C0, 6, 0xA0, 0x01, 0x82, 0x82, 0x41, 0xC5, 0, 0);
        _delay_ms(5);

        /* 10. DDP Display_Request for Area B (Main Center Screen) (ID 0x6C0, DLC 8) */
        /* 10 = DT (AR=0, EOM=1, SN=0)
           02 = Display_Request
           52 = Priority (52h)
           3B = SG_Address (Navigation Node 3Bh)
           07 = Info_Typ (07h = Active Target Guidance / Zielfuehrung)
           'N', 'A', 'V' = Message Heading */
        CAN_Transmit(0x6C0, 8, 0x10, 0x02, 0x52, 0x3B, 0x07, 'N', 'A', 'V');
        _delay_ms(5);

        /* 11. DDP Screen Layout: Command 0x69 (Draw Bitmap) - Frame 1 (ID 0x6C0, DLC 8) */
        /* 20 = DT (AR=1, EOM=0, SN=0)
           09 = Screen Layout Instruction
           00 = Info_ID
           69 = Command ID 0x69 (Bitmaps zeichnen)
           06 = Length (6 data bytes follow)
           20, 00 = X coordinate (32 px)
           20 = Y coordinate low byte (32 px) */
        CAN_Transmit(0x6C0, 8, 0x20, 0x09, 0x00, 0x69, 0x06, 0x20, 0x00, 0x20);
        _delay_ms(5);

        /* 12. DDP Screen Layout: Bitmap ID + Info_Finished - Frame 2 (ID 0x6C0, DLC 5) */
        /* 11 = DT (AR=0, EOM=1, SN=1)
           00 = Y coordinate high byte
           01, 00 = Bitmap Nr. 0x0001 (Straight Arrow, Little-Endian)
           08 = Info_Finished (Commit screen layout) */
        CAN_Transmit(0x6C0, 5, 0x11, 0x00, 0x01, 0x00, 0x08, 0, 0, 0);
        _delay_ms(5);

        /* 13. TP 1.6 Disconnect (ID 0x6C0, DLC 1) */
        CAN_Transmit(0x6C0, 1, 0xA8, 0, 0, 0, 0, 0, 0, 0);


        /* ========================================================== */
        /* PART 3: CADENCE PACING & DIAGNOSTICS                       */
        /* ========================================================== */
        loop_count++;
        if ((loop_count % 5) == 0) {
            printf("Dual loops: %lu\n", loop_count);
        }

        /* Active transmission took ~65 ms (13 frames * 5 ms).
           Delay 735 ms to maintain the verified ~800 ms OEM refresh cadence. */
        _delay_ms(735);
    }

    return 0;
}