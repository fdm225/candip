#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "USART0.h"

#define USART_RX_BUFFER_SIZE 128
#define USART_TX_BUFFER_SIZE 128
#define USART_RX_BUFFER_MASK (USART_RX_BUFFER_SIZE - 1)
#define USART_TX_BUFFER_MASK (USART_TX_BUFFER_SIZE - 1)

#if (USART_RX_BUFFER_SIZE & USART_RX_BUFFER_MASK)
#error RX buffer size is not a power of 2
#endif
#if (USART_TX_BUFFER_SIZE & USART_TX_BUFFER_MASK)
#error TX buffer size is not a power of 2
#endif

static volatile uint8_t USART_RxBuf[USART_RX_BUFFER_SIZE];
static volatile uint8_t USART_RxHead;
static volatile uint8_t USART_RxTail;
static volatile uint8_t USART_TxBuf[USART_TX_BUFFER_SIZE];
static volatile uint8_t USART_TxHead;
static volatile uint8_t USART_TxTail;

void USART0_Init(uint16_t bps)
{
    UBRR0H = (uint8_t)(bps >> 8);                  
    UBRR0L = (uint8_t)bps;
	
    UCSR0B = ((1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0)); 
    UCSR0C = (1 << URSEL0) | (0 << USBS0) | (3 << UCSZ00);
	
    USART_RxTail = 0;
    USART_RxHead = 0;
    USART_TxTail = 0;
    USART_TxHead = 0;
}

ISR(USART0_RXC_vect)
{
    uint8_t data = UDR0;                 
    uint8_t tmphead = (USART_RxHead + 1) & USART_RX_BUFFER_MASK;
    USART_RxHead = tmphead;

    if (tmphead == USART_RxTail) {
        /* Buffer overflow handling */
    }
	
    USART_RxBuf[tmphead] = data;
}

ISR(USART0_UDRE_vect)
{
    if (USART_TxHead != USART_TxTail) {
        uint8_t tmptail = (USART_TxTail + 1) & USART_TX_BUFFER_MASK;
        USART_TxTail = tmptail;
        UDR0 = USART_TxBuf[tmptail];
    } else {
        UCSR0B &= ~(1 << UDRIE0);
    }
}

uint8_t USART0_Receive(void)
{
    while (USART_RxHead == USART_RxTail);
    uint8_t tmptail = (USART_RxTail + 1) & USART_RX_BUFFER_MASK;
    USART_RxTail = tmptail;
    return USART_RxBuf[tmptail];
}

void USART0_Transmit_Buff_Insert(uint8_t data)
{
    uint8_t tmphead = (USART_TxHead + 1) & USART_TX_BUFFER_MASK; 
    while (tmphead == USART_TxTail);
    USART_TxBuf[tmphead] = data;
    USART_TxHead = tmphead;
}

void USART0_Transmit(uint8_t data)
{
    uint8_t newLineFlag = 0;

    if (data == '\n' || data == '\r') {
        data = '\r';
        newLineFlag = 1;
    }

    USART0_Transmit_Buff_Insert(data);
  
    if (newLineFlag) {
        USART0_Transmit_Buff_Insert('\n');
    }
  
    UCSR0B |= (1 << UDRIE0);
}

uint8_t DataInReceiveBuffer(void)
{
    return (USART_RxHead != USART_RxTail); 
}