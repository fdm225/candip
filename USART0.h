#ifndef _USART0_H_
#define _USART0_H_

#include <stdint.h>

#define BAUD57600  7   /* @7.3728MHz internal RC-osc. */
#define BAUD115200 3   /* @7.3728MHz internal RC-osc. */

void USART0_Init(uint16_t bps);
uint8_t USART0_Receive(void);
void USART0_Transmit(uint8_t data);
void USART0_Transmit_Buff_Insert(uint8_t data);
uint8_t DataInReceiveBuffer(void);

#endif /* _USART0_H_ */