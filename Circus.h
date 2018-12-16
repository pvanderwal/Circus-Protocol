/* user's sketch must define:
const uint8_t NID = ;				// Node ID, each node should have a different ID, high nibble only 0x10 - 0xF0
const uint16_t BAUD = ;
const uint8_t DEBOUNCE_TIME = 64;	// 0 = disable debounce counter, must be one of 0, 16,32,64 or 128
const uint8_t DEBOUNCE_PIN = 5;		// CIRCUS_COUNTER will increment every time this pin changes state:
									// low to high = +1, high to low = +1
const uint8_t DEBOUNCE_PULLUP = ;	// 1 = enable internal pullup resistor, 0 = disable internal pullup resistor

The following are optional, user should define it needed
You can define up to 4 timers and 1 counter. 

#define TIMERS 4   // the number of timers you want to use, if you don't want timers don't #define 
then for each timer you need to define a MACRO that either performs a simple action or calls a function()
the macro name must follow these examples:

#define TIMER_1_MACRO someFunctionName(value_if_needed)
#define TIMER_2_MACRO digitalWrite(pin, value)
etc.


COUNTERS 
If you define COUNTER then a macro variable called CIRCUS_COUNTER, will be established and linked to CDA.uintD[5]

If you want Circus code can create the counter ISR or debounce code for you.  At a minimum you must 
#define COUNTER pin# and either COUNTER_MODE or COUNTER_DEBOUNCE, if you don't then you'll need to define your own counter code/isr

If using COUNTER_DEBOUNCE then any pin can be used to trigger the count, the circus code will automatically create the debounce code
If using a debounce counter you can also define a DEBOUNCE_TIME delay.  If you don't define DEBOUNCE_TIME then it will default to 64 milliTics
Values used should be 16, 32, 64, 128, or 256.

i.e.:
#define COUNTER 5
#define COUNTER_DEBOUNCE
#define DEBOUNCE_TIME 128


If using COUNTER_MODE then COUNTER must be defined to use pins 2 or 3 only, Circus code will create the appropriate ISR triggered on the mode defined
COUNTER_MODE must be one of  RISING, FALLING, LOW, or CHANGE
i.e.

#define COUNTER 2
#define COUNTER_MODE CHANGE

You can also define COUNTER_PULLUP which enable an internal pull up resistor on the COUNTER pin

/**/

/* Naming Conventions
* global variables: _camelCase
* Macro variables: StartCaps	used for macros that simplify long variable names 
* Structures & Unions Start_Caps
* Macro constants: ALLCAPS
*/

#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef volatile union _Circus_Data_Array {
	uint8_t byteD[16];
	uint16_t uintD[8];
	int16_t intD[8];
	struct Control_Array {
		uint8_t timer1 : 1;
		uint8_t timer2 : 1;
		uint8_t timer3 : 1;
		uint8_t timer4 : 1;
		uint8_t newCmd : 1;			// Ringmaster has put new command in Cmd_Status byte
		uint8_t newStat : 1;		// node has new status in Cmd_Status byte, clears to zero when node reads CDA[0]
		uint8_t attention : 1;		// user defined attention flag, clears to zero when node reads CDA[0]
		uint8_t nodeEnabled : 1;	// user defined action
		uint8_t _cmdStat;			// Cmd_Status byte, used for passing command (Ringmaster to node) and/or status (node to RM) 
	} control;
} Circus_Data_Array;

extern Circus_Data_Array CDA;
extern volatile uint8_t RxIdx;
extern volatile uint8_t _timersRun;
//extern volatile uint8_t _timersEnabled;
extern volatile uint8_t _deadtime;

extern const uint8_t NID;
extern const uint16_t BAUD;
extern const uint8_t DEBOUNCE_TIME;		//zero disables debounce counter
extern const uint8_t DEBOUNCE_PIN;
extern const uint8_t DEBOUNCE_PULLUP;	//1= enable internal pullup resistor, 0=disable
extern const uint8_t TIMERS;
extern const void (*TIMER_1)(uint8_t);
extern const void (*TIMER_2)(uint8_t);
extern const void (*TIMER_3)(uint8_t);
extern const void (*TIMER_4)(uint8_t);
// Seed CRC with something other than zero
extern const uint8_t CRCSEED;  //crc8 returns zero if fed zeros, so start with some value other than zero

// Note, only ControlData (CDA[0]) and TicTime (CDA[7]) are fixed. CDA 1-6 are user configurable

#define CIRCUS_f_ENABLED CDA.control.nodeEnabled
#define CIRCUS_f_NEWCMD CDA.control.newCmd
#define CIRCUS_f_NEWSTAT CDA.control.newStat
#define CIRCUS_f_ATN CDA.control.attention
#define CIRCUS_CMDSTAT CDA.control._cmdStat
#define CIRCUS_BYTE CDA.byteD
#define CIRCUS_UINT CDA.uintD
#define CIRCUS_INT CDA.intD
#define Tic CDA.uintD[7]
#define CIRCUS_COUNTER CDA.uintD[5]


#define CIRCUS_TimersEnabled (CDA.byteD[0]&(_BV(TIMERS)-1))


void nodeControl(uint8_t) __attribute__ ((weak));

void circus_init();

void Circus(void);
uint8_t crc8( uint8_t, uint8_t);

void timerControl(void) __attribute__ ((weak));

//void setupDebounce(uint8_t, uint8_t, uint8_t);

#ifdef __cplusplus
}
#endif