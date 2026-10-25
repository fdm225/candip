#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "USART0.h"
#include "SJA1000.h"
#include "BAUDRATES.h"

int g_currentBaudRate = 0;

void CAN_Reg_Init(void)
{
    MCUCR  = 0xC0;                      
    EMCUCR = 0x44;
  
    MCUCR |= 0x02;   /* Set ISC01 */
    MCUCR &= 0xFE;   /* Clear ISC00 -> INT0 responds to falling edge */
    GICR  |= 0x40;   /* Enable INT0 */
}

void SJA1000_Init(uint8_t BTR0, uint8_t BTR1, uint8_t mode)
{
    printf("%d %x %x\n", g_currentBaudRate, BTR0, BTR1); 

    while (!(CAN_MODECTRL & RESREQ)) {
        CAN_MODECTRL = RESREQ;
    }

    CAN_CLKDIV = PCANMODE | COMPBYPASS;    
    CAN_INTEN  = 0x00;           
  
    CAN_AC0 = 0x00;           
    CAN_AC1 = 0x00;
    CAN_AC2 = 0x00;
    CAN_AC3 = 0x00;

    CAN_AM0 = 0xFF;          
    CAN_AM1 = 0xFF;
    CAN_AM2 = 0xFF;
    CAN_AM3 = 0xFF;

    CAN_TMG0 = BTR0;           
    CAN_TMG1 = BTR1;

    CAN_OCR   = 0x1A;
    CAN_INTEN = TIEN | RIEN | ALIEN;

    while (CAN_MODECTRL & RESREQ) {
        if (mode == READWRITE)
            CAN_MODECTRL = 0x00;
        else
            CAN_MODECTRL = 0x02;
    }
}

void SJA1000_Auto_Baud_Err_IRQ(void)
{
    if (g_currentBaudRate < Num_Baud_Rate) {
        uint8_t btr0 = pgm_read_byte(&Baud_Rate_BTR0[g_currentBaudRate]);
        uint8_t btr1 = pgm_read_byte(&Baud_Rate_BTR1[g_currentBaudRate]);
        SJA1000_Init(btr0, btr1, LISTEN);
        g_currentBaudRate++;
    }
}