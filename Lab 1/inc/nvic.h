// NVIC Library

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -


#ifndef NVIC_H_
#define NVIC_H_

#include <stdint.h>


void enableNvicInterrupt(uint8_t vectorNumber);
void disableNvicInterrupt(uint8_t vectorNumber);
void setNvicInterruptPriority(uint8_t vectorNumber, uint8_t priority);

#endif
