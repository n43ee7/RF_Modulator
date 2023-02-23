// Clock Library

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// 16 MHz external crystal oscillator


#ifndef CLOCK_H_
#define CLOCK_H_

void initSystemClockTo40Mhz(void);
void initSystemClockTo80Mhz(void);

#endif
