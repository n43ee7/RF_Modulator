// RF Baseband signal generator | University of Texas at Arlington
// (c) Nabeel Nayyar All Rights reserved
// Base-band Modulator Console
//
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
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
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
#define TWO_32 4294967296

int WRITE_Q = CHAN_Q_START; // Channel A = I OUTPUT
int WRITE_I = CHAN_I_START; // Channel B = Q OUTPUT

// ================================ Lookup Tables Guides ===============================
// Defining the Lookup tables for the Instance
uint16_t LUT_I[4096];
uint16_t LUT_Q[4096];

// For Channel I
int IntCn_I = 0; int Inc_B_I = 1;
uint32_t phi_I = 0; int fO_I = 10000;

// For Channel Q
int IntCn_Q = 0; int Inc_B_Q = 1;
uint32_t phi_Q = 0; int fO_Q = 10000;


// ============================== Modulation Guides ===================================
// Enumeration data structure for Modulation for modulation guide
enum mode { raw, dc, sine, bpsk, qpsk, psk8, qam16 };
enum mode mode;
// Modulation Symbol Guide as follows:
uint32_t symbolsPerMod[7] = {1, 1, 1, 1, 2, 3, 4};
uint32_t loopValPerMod[7] = {1, 1, 4096, 2, 4, 8, 16};
uint32_t loopVal = 1;
int loopval = 1;

// Channel Q Gain
#define Q_GAIN ((4095 - 175) / 2)
// Channel I Gain
#define I_GAIN ((4095 - 190) / 2)

int32_t I_Bits = 0;
int32_t Q_Bits = 0;

int32_t OOK_I[2] = {0 , I_GAIN};
int32_t OOK_Q[2] = {0, 0};

int32_t BPSK_I[2] = {I_GAIN, -I_GAIN};
int32_t BPSK_Q[2] = {0, 0};

int32_t QPSK_I[2] = {I_GAIN, -I_GAIN};
int32_t QPSK_Q[2] = {Q_GAIN, -Q_GAIN};

int32_t PSK8_I[8] = {I_GAIN*1.00, I_GAIN*0.71, -I_GAIN*0.71, I_GAIN*0.00,
                     I_GAIN*0.71, -I_GAIN*0.00, -I_GAIN*1.00, -I_GAIN*0.71};
int32_t PSK8_Q[8] = {Q_GAIN*0.00, Q_GAIN*0.71, Q_GAIN*0.71, Q_GAIN*1.00,
                     -Q_GAIN*0.71, -Q_GAIN*1.00, -Q_GAIN*0.00, -Q_GAIN*0.71};

int32_t QUAM16_I[4] = {-I_GAIN*1.00, -I_GAIN*0.33, I_GAIN*1.00, I_GAIN*0.33};
int32_t QUAM16_Q[4] = {-Q_GAIN*1.00, -Q_GAIN*0.33, Q_GAIN*1.00, Q_GAIN*0.33};

int32_t QUAM64_I[8] = {-I_GAIN*1.00, -I_GAIN*0.71, -I_GAIN*0.14, -I_GAIN*0.43,
                      I_GAIN*1.00, I_GAIN*0.71, I_GAIN*0.14, I_GAIN*0.43};
int32_t QUAM64_Q[8] = {-Q_GAIN*1.00, -Q_GAIN*0.71, -Q_GAIN*0.14, -Q_GAIN*0.43,
                      Q_GAIN*1.00, Q_GAIN*0.71, Q_GAIN*0.14, Q_GAIN*0.43};
// ===================================================================================
// Tone Modulation Command Vars
bool ToneMode = false;

// ===================================================================================
// Filter Variables
bool RRCFilter = false;

// Filter Coefficients [rrc, order=30, fs/fc = 8, alpha=0.25]
float h[31] = {0.0023, -0.0043, -0.0102, -0.0090, 0.0015, 0.0159, 0.0230, 0.0130,
                -0.0136, -0.0422, -0.0493, -0.0160, 0.0593, 0.1553, 0.2357, 0.2671,
                 0.2357,  0.1553,  0.0593, -0.0160, -0.0493, -0.0422, -0.0136, 0.0130,
                 0.0230,  0.0159,  0.0015, -0.0090, -0.0102, -0.0043, 0.0023};
// Zero Padded Modulation arrays
// > BPSK
float BPSK_I_Filter[8] = {I_GAIN, 0, 0, 0, -I_GAIN, 0, 0, 0};
float BPSK_Q_Filter[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// > QPSK
float QPSK_I_Filter[8] = {I_GAIN, 0, 0, 0, -I_GAIN, 0, 0, 0};
float QPSK_Q_Filter[8] = {Q_GAIN, 0, 0, 0, -Q_GAIN, 0, 0, 0};
// > 8PSK
float PSK8_I_Filter[33] = {I_GAIN*1.00, 0, 0, 0, I_GAIN*0.71, 0, 0, 0, -I_GAIN*0.71, 0, 0, 0, I_GAIN*0.00, 0, 0, 0,
                            I_GAIN*0.71, 0, 0, 0, -I_GAIN*0.00, 0, 0, 0, -I_GAIN*1.00, 0, 0, 0, -I_GAIN*0.71, 0, 0, 0,};
float PSK8_Q_Filter[33] = {Q_GAIN*0.00, 0, 0, 0, Q_GAIN*0.71, 0, 0, 0, Q_GAIN*0.71, 0, 0, 0, Q_GAIN*1.00, 0, 0, 0,
                            -Q_GAIN*0.71, 0, 0, 0, -Q_GAIN*1.00, 0, 0, 0, -Q_GAIN*0.00, 0, 0, 0, -Q_GAIN*0.71, 0, 0, 0,};
// > 16QAM
float QUAM16_I_Filter[16] = {-I_GAIN*1.00, 0, 0, 0, -I_GAIN*0.33, 0, 0, 0, I_GAIN*1.00, 0, 0, 0, I_GAIN*0.33, 0, 0, 0,};
float QUAM16_Q_Filter[16] = {-Q_GAIN*1.00, 0, 0, 0, -Q_GAIN*0.33, 0, 0, 0, Q_GAIN*1.00, 0, 0, 0, Q_GAIN*0.33, 0, 0, 0,};

// ===================================================================================
// Declaring the Instances of functions declared in this scope
void initHw();
void processShell();
void initSymbolTimer(void);
void setSymbolRate(float sampleRate);
void symbolTimerIsr();
void RAWModulator(char *OPTION, int N);
void DCModulator(char *OPTION, float DC);
void SineModulator(char *OPTION, int f, float AMP);
void ToneModulator(int f, float AMP);
void Modulator(char *OPTION, char *data);
void Filtermode();

// Code Main Routine
int main(void) {
    initHw();   // Running Hardware Setup instance

    // Display Header
    putsUart0("===============================================\n\r");
    putsUart0("         CSE 4377 > Modulator Console\n\r");
    putsUart0("===============================================\n\r");

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
                mode = raw;
                char *OPTION;int N;
                OPTION = strtok(NULL, " ");
                N = atoi(strtok(NULL, " "));
                if (strcmp(OPTION, "i") == 0 || strcmp(OPTION, "q") == 0){
                    RAWModulator(OPTION, N);
                }
            }

            // dc a|b DC []
            if (strcmp(token, "dc") == 0) {
                knownCommand = true;
                mode = dc;
                char *OPTION;float DC;
                OPTION = strtok(NULL, " ");
                DC = atof(strtok(NULL, " "));
                if (strcmp(OPTION, "i") == 0 || strcmp(OPTION, "q") == 0){
                    DCModulator(OPTION, DC);
                }
            }

            // sine a|b FREQ [AMPL [PHASE [DC] ] ]
            if (strcmp(token, "sine") == 0) {
                knownCommand = true;
                mode = sine;
                char *OPTION; int f; float AMP;
                OPTION = strtok(NULL, " ");
                f = atoi(strtok(NULL, " "));
                AMP = atof(strtok(NULL, " "));
                if (strcmp(OPTION, "i") == 0 || strcmp(OPTION, "q") == 0){
                    SineModulator(OPTION, f, AMP);
                }
            }

            // tone FREQ [AMPL [PHASE [DC] ] ]
            if (strcmp(token, "tone") == 0) {
                knownCommand = true; int f; float AMP;
                mode = sine;
                f = atoi(strtok(NULL, " "));
                AMP = atof(strtok(NULL, " "));
                ToneModulator(f, AMP);
            }

            // mod bpsk|qpsk|8psk|16qam
            if (strcmp(token, "mod") == 0) {
                knownCommand = true;
                char *OPTION; char *String;
                OPTION = strtok(NULL, " ");
                String = strtok(NULL, " ");
                Modulator(OPTION, String);
            }

            // filter FILTER
            if (strcmp(token, "filter") == 0) {
                knownCommand = true;
                char *OPTION;
                OPTION = strtok(NULL, " ");
                if (strcmp(OPTION, "rrc") == 0){
                    RRCFilter = true;
                } else if (strcmp(OPTION, "off") == 0){
                    RRCFilter = false;
                } else {
                    putsUart0("[!] Invalid Filter Setting. Try help.\n\r");
                }
            }

            if (strcmp(token,"sr")==0) {
                knownCommand = true;
                float SRate = atof(strtok(NULL, " "));
                setSymbolRate(SRate);
            }

            // COMMAND: reboot
            if (strcmp(token, "reboot") == 0) {
                knownCommand = true;
                NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            }

            if (!knownCommand){
                putsUart0("[!] Invalid command type 'help' lmao\n\r");
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
                putsUart0("  sr       SYMBOLRATE\n\r");
                putsUart0("  reboot\n\r");
                putsUart0("\n\r");
                putsUart0("  where FREQ = [-Fs/2, Fs/2] Hz\n\r");
                putsUart0("        AMPL = [0, 0.5] V\n\r");
                putsUart0("        DC   = [-0.5, 0.5] V\n\r");
                putsUart0("        RAW  = [0, 4095] LSb\n\r");
            }
        putsUart0("\n\r");
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
    //TIMER1_TAILR_R = round(FCYC/FS);                 // set load value to match sample rate
    TIMER1_TAILR_R = 2000;
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts for timeout in timer module
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    enableNvicInterrupt(INT_TIMER1A);                // turn-on interrupt 37 (TIMER1A) in NVIC

}

void setSymbolRate(float sampleRate) {
    TIMER1_TAILR_R = round(FCYC/sampleRate);
}

// Interrupt service routine for triggering write to I/Q channels of the DAC
void symbolTimerIsr() {
    // Trigger LDAC
    setPinValue(LDAC, false);
    _delay_cycles(10);
    setPinValue(LDAC, true);

    // Enums used used to Modulate in ISR
    if (mode == sine) {
        // Sine
        RAWModulator("i", LUT_I[IntCn_I]);
        RAWModulator("q", LUT_Q[IntCn_Q]);
    } else if (mode == bpsk) {
        //Bpsk
        RAWModulator("i", (-BPSK_I[IntCn_I] + 2135));
        RAWModulator("q", (-BPSK_Q[IntCn_Q] + 2135));
    } else if(mode==qpsk) {
        //qpsk (upper bit vs lower bit)
        I_Bits = IntCn_I & 1;                 // and with 01 to only get value of first bit
        Q_Bits = IntCn_Q & 2;                 // and with 10 to only get value of second bit
        Q_Bits = Q_Bits >> 1;
        RAWModulator("i", (-QPSK_I[I_Bits] + 2135));
        RAWModulator("q", (-QPSK_Q[Q_Bits] + 2135));
    } else if(mode == psk8) {
        //8psk
        RAWModulator("i", (-PSK8_I[IntCn_I] + 2135));
        RAWModulator("q", (-PSK8_Q[IntCn_Q] + 2135));
    } else if(mode == qam16) {
        //16qam (upper 2 bits vs lower 2 bits)
        I_Bits = IntCn_I & 3;               //and with 0011 to get only the lower 2 bits
        Q_Bits = IntCn_Q & 12;              //and with 1100 to get only the upper 2 bits
        Q_Bits = Q_Bits >> 2;
        RAWModulator("i", (-QUAM16_I[I_Bits] + 2135));
        RAWModulator("q", (-QUAM16_Q[Q_Bits] + 2135));
    }

    // Write on SPI Port
    SSI0_DR_R = WRITE_Q;
    SSI0_DR_R = WRITE_I;

    // Interrupt revolving counter for ISR Read and increment modes
    IntCn_I = (IntCn_I + Inc_B_I) % loopValPerMod[mode];   // Channel I
    IntCn_Q = (IntCn_Q + Inc_B_Q) % loopValPerMod[mode];   // Channel Q

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

// Generating a DC Signal on specified output
void DCModulator(char *OPTION, float DC){
    int DACval = 0;
    // Calculating the RAW DAC value for the input DC voltage
    DACval = ((-1*DC) + 0.5) * (4095);
    // Writing the RAW DAC value to the DAC
    RAWModulator(OPTION, DACval);
}


// Modulating a Sine wave according to the parameters
void SineModulator(char *OPTION, int f, float AMP) {
    // LUT elements
    int i; double Max = 4096; double temp;
    loopval = Max;

    // Calculating the Delta Phi for an increment in the phase
    if(strcmp(OPTION, "i") == 0) {
        phi_I = (f * TWO_32 ) / FS;
        Inc_B_I = phi_I >> 20;

    } else if (strcmp(OPTION, "q") == 0){
        phi_Q = (f * TWO_32 ) / FS;
        Inc_B_Q = phi_Q >> 20;
    }

    // Selecting Wave streaming mode
    if (strcmp(OPTION, "i") == 0){
        // LUT Population For DAC I (A)
        for (i = 0; i <= 4095; i++) {
            temp = sin(((double) i /Max) * 2 * M_PI);
            LUT_I[i] = (uint16_t) (((4095 - 190) / 2) * temp + (2135 + 0));
        }
        IntCn_I = 0;
        IntCn_Q = 0;
    }
    else if(strcmp(OPTION, "q") == 0){
       // Write the Sin function to the Channel
       // LUT For DAC Q (B)
       for (i = 0; i <= 4095; i++){
           temp = cos(((double) i /Max) * 2 * M_PI);
           LUT_Q[i] = (uint16_t) (((4095 - 175) / 2) * temp + (2135 + 0));
        }
       IntCn_I = 0;
       IntCn_Q = 0;
    }
}

// Tone Modulator for Outputting I/Q
void ToneModulator(int f, float AMP) {
    ToneMode = true;
    SineModulator("i", f, AMP);
    SineModulator("q", f, AMP);
}

// Modulating a Signal in any specified channel
void Modulator(char *OPTION, char *data) {
    if (strcmp(OPTION, "ook") == 0){
        //not required right now
    } else if (strcmp(OPTION, "bpsk") == 0) {
        mode = bpsk;
        IntCn_I = 0;
        IntCn_Q = 0;
    } else if (strcmp(OPTION, "qpsk") == 0) {
        mode = qpsk;
        IntCn_I = 0;
        IntCn_Q = 0;
    } else if (strcmp(OPTION, "8psk") == 0) {
        mode = psk8;
        IntCn_I = 0;
        IntCn_Q = 0;
    } else if (strcmp(OPTION, "16qam") == 0){
        mode = qam16;
        IntCn_I = 0;
        IntCn_Q = 0;
    } else if (strcmp(OPTION, "64qam") == 0){
        //not required right now
    } else {
        return;
    }
}

// Discontinued module
void Filtermode(){
}
