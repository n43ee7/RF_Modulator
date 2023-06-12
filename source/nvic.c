// NVIC Library

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration: -


#include <tm4c123gh6pm.h>
#include "inc/nvic.h"

void enableNvicInterrupt(uint8_t vectorNumber)
{
    volatile uint32_t* p = (uint32_t*) &NVIC_EN0_R;
    vectorNumber -= 16;
    p += vectorNumber >> 5;
    *p = 1 << (vectorNumber & 31);
}

void disableNvicInterrupt(uint8_t vectorNumber)
{
    volatile uint32_t* p = (uint32_t*) &NVIC_DIS0_R;
    vectorNumber -= 16;
    p += vectorNumber >> 5;
    *p = 1 << (vectorNumber & 31);
}

void setNvicInterruptPriority(uint8_t vectorNumber, uint8_t priority)
{
    volatile uint32_t* p = (uint32_t*) &NVIC_PRI0_R;
    vectorNumber -= 16;
    uint32_t shift = 5 + (vectorNumber & 3) * 8;
    p += vectorNumber >> 2;
    *p &= ~(7 << shift);
    *p |= priority << shift;
}

