#ifndef _BAUDRATES_H_
#define _BAUDRATES_H_

#include <stdint.h>
#include <avr/pgmspace.h>

extern const uint8_t Num_Baud_Rate;
extern const uint8_t Baud_Rate_BTR0[] PROGMEM;
extern const uint8_t Baud_Rate_BTR1[] PROGMEM;

#endif /* _BAUDRATES_H_ */