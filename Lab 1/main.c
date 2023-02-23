// CSE 4377/5377 | Baseband Modulator
// University of Texas at Arlington
// (c) Nabeel Nayyar

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz (Formerly 80MHz Setting)

// Hardware configuration:
// DAC on SPI0 Interface:
//   > MOSI on PA5 (SSI0Tx)
//   > ~CS on PA3  (SSI0Fss)
//   > SCLK on PA2 (SSI0Clk)
//   > ~LDAC on PA4

// Device includes, defines, and assembler directives
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <tm4c123gh6pm.h>
#include "inc/clock.h"
#include "inc/gpio.h"
#include "inc/nvic.h"
#include "inc/spi0.h"
#include "inc/uart0.h"
#include "inc/wait.h"

// System Constrains
#define MAX_CHARS 80    // Maximum String Characters

#define FCYC 40e6       // FC Cycles for SPI
#define FDAC 20e6       // F Cycles for DAC
#define FS 100000       // FS Sample Rate


// > Hardware Defined Pins DAC Control
#define CS      PORTA,  3       // PA3 [ACT LOW]
#define SCK     PORTA,  2       // PA2
#define SDI     PORTA,  5       // PA5
#define LDAC    PORTA,  4       // PA4 [ACT LOW]

// > DAC RAW Write Directives
//  Writing to Channel I (A) First 4 bits is [A/B X GA SHUT]
//  ([0011]000000000000)2 = (12288)10
//  Writing to Channel Q (B) First 4 bits is [A/B X GA SHUT]
//  ([1011]000000000000)2 = (45056)10
#define CHAN_I_START 12288
#define CHAN_Q_START 45056
#define D_RES_MAX 4095          // DAC Maximum Resolution
#define D_RES_MIN 0             // DAC Minimum Resolution
#define D_VREF 2.048            // DAC Voltage Reference

int WRITE_Q = CHAN_Q_START; // Channel A = I OUTPUT
int WRITE_I = CHAN_I_START; // Channel B = Q OUTPUT

// Defining the Lookup tables for the Instance
uint16_t LUT_I[4095];
uint16_t LUT_Q[4095];

// Declaring the Instances of functions declared in this scope
void initHw();
void processShell();
void initSymbolTimer(void);
void setSymbolRate(float sampleRate);
void symbolTimerIsr();
void RAWModulator(char *OPTION, int N);
void DCModulator(char *OPTION, float DC);
void SineModulator(char *OPTION);


// Code Main Routine
int main(void) {
    initHw();   // Running Hardware Setup instance

    // Display Header
    putsUart0("===============================\n\r");
    putsUart0("CSE 4377 > Modulator Console\n\r");
    putsUart0("===============================\n\r");

    // Main instance of the Program
    while (true) {
        processShell(); // Shell UI Instance
    }
}


// Initialize Hardware Setup Routine
void initHw() {
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Setup UART0 baud rate
    initUart0();
    setUart0BaudRate(115200, FCYC);

    // Enable GPIO Port of A for instantiated pins
    enablePort(PORTA);

    // Initialize SPI0
    initSpi0(USE_SSI0_FSS);
    setSpi0BaudRate(FDAC, FCYC);
    setSpi0Mode(0, 0);


    // Setting CS and LDAC High
    setPinValue(CS, true);
    setPinValue(LDAC, true);

    // Initialize symbol timer
    initSymbolTimer();
}

// Sub-routine for creating a shell instance on UART
void processShell() {
    bool knownCommand = false; bool end; char c;
    static char strInput[MAX_CHARS+1];
    char* token; static uint8_t count = 0;

    if (kbhitUart0()) {
        c = tolower(getcUart0());
        end = (c == 13) || (count == MAX_CHARS);
        if (!end) {
            if ((c == 8 || c == 127) && count > 0){
                count--;
            } if (c >= ' ' && c < 127) {
                strInput[count++] = c;
            }
        } else {
            strInput[count] = '\0';
            count = 0;
            token = strtok(strInput, " ");

            // COMMAND:: raw i|q RAW_VALUE
            if (strcmp(token, "raw") == 0) {
                knownCommand = true;
                char *OPTION;
                int N;
                // add code to process command
                OPTION = strtok(NULL, " ");
                N = atoi(strtok(NULL, " "));
                if (strcmp(OPTION, "i") == 0 || strcmp(OPTION, "q") == 0){
                    RAWModulator(OPTION, N);
                }
            }

            // dc a|b DC []
            if (strcmp(token, "dc") == 0) {
                knownCommand = true;
                char *OPTION;
                float DC;
                OPTION = strtok(NULL, " ");
                DC = atof(strtok(NULL, " "));
                if (strcmp(OPTION, "i") == 0 || strcmp(OPTION, "q") == 0){
                    DCModulator(OPTION, DC);
                }
            }

            // sine a|b FREQ [AMPL [PHASE [DC] ] ]
            if (strcmp(token, "sine") == 0) {
                knownCommand = true;
                char *OPTION;
                OPTION = strtok(NULL, " ");
                if (strcmp(OPTION, "i") == 0 || strcmp(OPTION, "q") == 0){
                    SineModulator(OPTION);
                }


            }

            // tone FREQ [AMPL [PHASE [DC] ] ]
            if (strcmp(token, "tone") == 0) {
                knownCommand = true;
                // add code to process command
            }

            // mod bpsk|qpsk|8psk|16qam
            if (strcmp(token, "mod") == 0) {
                knownCommand = true;
                // add code to process command
            }

            // filter FILTER
            if (strcmp(token, "filter") == 0) {
                knownCommand = true;
                // add code to process command
            }


            // COMMAND: reboot
            if (strcmp(token, "reboot") == 0) {
                knownCommand = true;
                NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            }

            if (!knownCommand){
                putsUart0("[!] Invalid command\n\r");
            }

            // COMMAND: help + Invalid Command Directory
            if (strcmp(token, "help") == 0) {
                putsUart0("Commands:\n\r");
                putsUart0("  dc       i|q DC\n\r");
                putsUart0("  sine     i|q FREQ [AMPL [PHASE [DC] ] ]\n\r");
                putsUart0("  tone     FREQ [AMPL [PHASE [DC] ] ]\n\r");
                putsUart0("  mod      ook|bpsk|qpsk|8psk|16qam|64qam\n\r");
                putsUart0("  filter   rrc|off\n\r");
                putsUart0("  raw      i|q RAW\n\r");
                putsUart0("  reboot\n\r");
                putsUart0("\n\r");
                putsUart0("  where FREQ = [-Fs/2, Fs/2] Hz\n\r");
                putsUart0("        AMPL = [0, 0.5] V\n\r");
                putsUart0("        DC   = [-0.5, 0.5] V\n\r");
                putsUart0("        RAW  = [0, 4095] LSb\n\r");
            }
        }
    }
}

// Must leave this timer on to ensure UI commands like DC are updated
void initSymbolTimer(void) {
    // Enable clocks
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    _delay_cycles(3);

    // Configure Timer 1 as the time base
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER1_TAILR_R = round(FCYC/FS);                 // set load value to match sample rate
    //TIMER1_TAILR_R = 2000;
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts for timeout in timer module
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    enableNvicInterrupt(INT_TIMER1A);                // turn-on interrupt 37 (TIMER1A) in NVIC

}

void setSymbolRate(float sampleRate) {
    TIMER1_TAILR_R = round(FCYC/sampleRate);

}

// Symbol timer called by timer 1
void symbolTimerIsr() {
    // Trigger LDAC
    setPinValue(LDAC, false);
    _delay_cycles(10);
    setPinValue(LDAC, true);
    // Write to SPI Port
    SSI0_DR_R = WRITE_Q;
    SSI0_DR_R = WRITE_I;

    // Disable the interrupt
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;
}

// Writing RAW values to DAC -> I/Q [4095, 0]
void RAWModulator(char *OPTION, int N) {

    // Write to Channel I
    if (strcmp(OPTION, "i") == 0){
        // Calculate the padded 16 bit Address to write to DAC
        WRITE_I = CHAN_I_START + N;
    }

    if (strcmp(OPTION, "q") == 0){
        // Calculate the padded 16 bit Address to write to DAC
        WRITE_Q = CHAN_Q_START + N;

    }
}

void DCModulator(char *OPTION, float DC){
    int DACval = 0;
    // Calculating the RAW DAC value for the input DC voltage
    DACval = ((-1*DC) + 0.5) * (4095);
    // Writing the RAW DAC value to the DAC
    RAWModulator(OPTION, DACval);
}

void SineModulator(char *OPTION) {

    // LUT elements
    int i; double Max = 4096; double temp;

    // Selecting
    if (strcmp(OPTION, "i") == 0 ){
        // LUT Population For DAC I (A)
        for (i = 0; i <= 4095; i++){
            temp = sin(((double) i /Max) * 2 * M_PI);
            LUT_I[i] = (uint16_t) ((4095 - 190) / 2) * temp + (2135 + 190);
        }
    }
    else if(strcmp(OPTION, "q") == 0){
    // LUT For DAC Q (B)
    for (i = 0; i <= 4095; i++){
        temp = cos(((double) i /Max) * 2 * M_PI);
        LUT_Q[i] = (uint16_t) ((4095 - 175) / 2) * temp + (2135 + 175);
    }
    // Write the Sin function to the Channel
    }
}
