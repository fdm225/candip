#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "SJA1000.h"

void CAN_Transmit(uint16_t id, uint8_t dataLength, 
                  uint8_t byte1, uint8_t byte2,
                  uint8_t byte3, uint8_t byte4,
                  uint8_t byte5, uint8_t byte6,
                  uint8_t byte7, uint8_t byte8)
{
    /* 1. Wait until SJA1000 Transmit Buffer is released (TBS = 1) */
    while (!(CAN_STATUS & TBS));

    /* 2. Configure Standard Frame Format (FF=0, RTR=0) with DLC */
    CAN_TX_FRMINFO = (dataLength & 0x0F);

    /* 3. Load 11-bit Standard Identifier into ID1 and ID2 */
    CAN_TX_ID1 = (uint8_t)(id >> 3);
    CAN_TX_ID2 = (uint8_t)((id & 0x07) << 5);

    /* 4. Pack payload bytes into SJA1000 TX mailbox */
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 0) = byte1;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 1) = byte2;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 2) = byte3;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 3) = byte4;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 4) = byte5;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 5) = byte6;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 6) = byte7;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 7) = byte8;

    /* 5. Command Transmission Request */
    CAN_CMD = TXREQ;

    /* 6. Wait for Transmission Complete before returning */
    while (!(CAN_STATUS & TBS));
}