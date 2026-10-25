#include <avr/io.h>
#include <avr/interrupt.h>
#include "SJA1000.h"

ISR(INT0_vect)
{
    /* Reading CAN_INT clears all internal flags */
    volatile uint8_t dummy = CAN_INT;
    (void)dummy;
    CAN_CMD = RRB;
}