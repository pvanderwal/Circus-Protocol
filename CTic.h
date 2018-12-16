 /**** The following should be defined by user's code prior to #include <Tic.h> ****

The following are optional, user should define in their code if needed before including circus.h/tic.h
You can define up to 4 timers 

#define TIMERS 4   // the number of timers you want to use, if you don't want timers don't #define 

#define TIMER_1_MACRO digitalWrite2(pin, HIGH)      
etc.

If not using Circus, user needs to create a global variable for each timer being used
Timer variables need to be named _timer1, _timer2, etc. and must be a uint16_t 
Circus code takes care of creating these variables

// not used #define COUNTERS 2  // the number of counters you want to use, if you don't want counters don't #define 

//Tic code can create two counter ISRs for you
#define COUNTER_1_MODE   RISING, FALLING, LOW, CHANGE // INT0  Arduino pin 2  
    or    								// define mode or debounce, but not both
#define COUNTER_1_DEBOUNCE  0, 1, or 2  // Arduino pin 2 (portD PD2)  0=Low, 1=High, 2=Change

#define COUNTER_2_MODE   RISING, FALLING, LOW, CHANGE // INT1 Arduino pin 3 
    or   								// define mode or debounce, but not both
#define COUNTER_2_DEBOUNCE  0, 1, or 2  // Arduino pin 3 (portD PD3)

// counter 3 and 4 are debounce only
#define COUNTER_3_DEBOUNCE  0, 1, or 2  // Arduino pin 4 (portD PD4)
#define COUNTER_4_DEBOUNCE  0, 1, or 2  // Arduino pin 5 (portD PD5)

 // Note: Debounce uses TIC timer to do a software debounce.
 
#define DEBOUNCE_TIME #mTics    // should be a power of 2, i.e. 16, 32, 64, etc.
								// defaults to 64 mTics

//if you want to enable a pullup resistor
#define COUNTER_1_PULLUP   //ditto for counters 2,3, and/or 4						
//This works with either MODE or DEBOUNCE
*/

/* Naming Conventions
* global variables: _camelCase
* Macro variables: StartCaps	used for macros that simplify long variable names 
* Structures & Unions Start_Caps
* Macro constants: ALLCAPS
*/


#ifndef TICTIMER_H
#define TICTIMER_H

// the follwoing are needed to provide variable types, etc.
#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif



uint16_t volatile static Tic;


#if TIMERS >= 1
uint8_t volatile timersRun;
void timerControl(void) __attribute__((weak));
void timer1Function(void) __attribute__((weak));
#if TIMERS >= 2
void timer2Function(void) __attribute__((weak));
#endif
#if TIMERS >= 3
void timer3Function(void) __attribute__((weak));
#endif
#if TIMERS >= 4
void timer4Function(void) __attribute__((weak));
#endif
#endif

#ifdef COUNTER_1_DEBOUNCE
#define DEBOUNCE1 2
#else 
#define DEBOUNCE1 0
#endif
#ifdef COUNTER_2_DEBOUNCE
#define DEBOUNCE2 4
#else 
#define DEBOUNCE2 0
#endif
#ifdef COUNTER_3_DEBOUNCE
#define DEBOUNCE3 8
#else 
#define DEBOUNCE3 0
#endif
#ifdef COUNTER_4_DEBOUNCE
#define DEBOUNCE4 16
#else 
#define DEBOUNCE4 0
#endif

#define DEBOUNCE_MASK DEBOUNCE1 + DEBOUNCE2 + DEBOUNCE3 + DEBOUNCE4
#if DEBOUNCE_MASK > 1
#define DO_DEBOUNCE
#endif


#ifndef DEBOUNCE_TIME
#define DEBOUNCE_TIME 0x3f
#endif

#ifdef COUNTER_1_MODE 
void ISR0(void) __attribute__((weak));
#endif
#ifdef COUNTER_2_MODE 
void ISR1(void) __attribute__((weak));
#endif

void ticSetup(void);

void waitT(uint16_t) __attribute__((weak));
uint16_t nowT(void);
uint16_t milliT(void);


//#ifdef MICROTIC  
uint8_t volatile static microTic;
uint16_t microT(void);
void delayMicroTics(uint16_t);
//#endif

#ifdef __cplusplus
}
#endif


#endif /* TICTIMER_H */