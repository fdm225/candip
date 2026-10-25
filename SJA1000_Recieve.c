#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "USART0.h"
#include "SJA1000.h"

uint8_t foundFlag = 0;

void SJA1000_Print_Frame_Rcv_IRQ(void)
{
    uint16_t id = ((uint16_t)CAN_RX_BUF1 << 3) + (CAN_RX_BUF2 >> 5);
    uint8_t len = CAN_RX_FRMINFO & 0x0F;

    printf("%x", id);
    for (int i = 0; i < (int)len; i++) {
        printf(" %x", *(volatile uint8_t *)(CAN_RX_DATA_BASE + i));
    }
    printf("\n");
}

void SJA1000_Auto_Baud_Rcv_IRQ(void)
{
    if (!foundFlag) {
        printf("BTR0: %x BTR1: %x\n", CAN_TMG0, CAN_TMG1);
        SJA1000_Init(CAN_TMG0, CAN_TMG1, READWRITE);
        foundFlag = 1;
    } else {
        printf("%x %x: ", CAN_TMG0, CAN_TMG1);
    }
}