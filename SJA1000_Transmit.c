#include <avr/io.h>
#include <avr/interrupt.h>
#include "SJA1000.h"

void CAN_Transmit(uint16_t id, uint8_t dataLength, 
                  uint8_t byte1, uint8_t byte2,
                  uint8_t byte3, uint8_t byte4,
                  uint8_t byte5, uint8_t byte6,
                  uint8_t byte7, uint8_t byte8)
{
    uint16_t timeout = 50000;

    /* 1. Wait until Transmit Buffer is free (TBS = 1), with timeout guard */
    while (!(CAN_STATUS & TBS)) {
        if (--timeout == 0) {
            return; // Bus error / arbitration fail-safe: don't hang MCU
        }
    }

    /* 2. Load Frame Info (Standard Frame Format FF=0, RTR=0, DLC) */
    CAN_TX_FRMINFO = (dataLength & 0x0F);

    /* 3. Load 11-bit Standard ID */
    CAN_TX_ID1 = (uint8_t)(id >> 3);
    CAN_TX_ID2 = (uint8_t)((id & 0x07) << 5);

    /* 4. Pack Data */
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 0) = byte1;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 1) = byte2;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 2) = byte3;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 3) = byte4;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 4) = byte5;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 5) = byte6;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 6) = byte7;
    *(volatile uint8_t *)(CAN_TX_DATA_BASE + 7) = byte8;

    /* 5. Issue Transmit Request */
    CAN_CMD = TXREQ;
}