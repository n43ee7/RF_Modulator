// Clock Library

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// 16 MHz external crystal oscillator


#include <stdint.h>
#include <tm4c123gh6pm.h>
#include "inc/clock.h"


// Initialize system clock to 40 MHz using PLL and 16 MHz crystal oscillator
void initSystemClockTo40Mhz(void)
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);
}

// Initialize system clock to 80 MHz using PLL and 16 MHz crystal oscillator
void initSystemClockTo80Mhz(void)
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Change system clock of 80 MHz
    SYSCTL_RCC2_R = SYSCTL_RCC2_USERCC2 | SYSCTL_RCC2_DIV400 | SYSCTL_RCC2_OSCSRC2_MO | (2 << SYSCTL_RCC2_SYSDIV2_S);
}
