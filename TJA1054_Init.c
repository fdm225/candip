#include <avr/io.h>
#include "TJA1054.h"

void TJA1054_Init(void)
{
    /* Set PB0+PB1 as output and PB2 as input with pullup */
    DDRB |= (1 << DDB0) | (1 << DDB1);

    /* Set Enable and STB high for normal operating mode */
    PORTB |= (1 << EN) | (1 << STB);
}