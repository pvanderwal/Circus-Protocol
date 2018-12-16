/*  
/*************************************************************************
Title:    Tic timer library for Circus Ring
Author:   Peter VanDerWal
File:    
Software: 
Hardware: Currently works with Atmega328, could probably work with any AVR using built in timers, 
License:  GNU General Public License Version 2.0 

Copyright 2018 Peter VanDerWal 
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2.0 as published by
    the Free Software Foundation
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
    
*************************************************************************
    
	Tic timer implements the concept of a 16 bit daily clock
	So 1 day = 65536 Tics (max count with 16 bits)
	Since there are 86400 seconds in a day that means each Tic should ideally == 1.318359375 seconds, 
	With 1024 milliTics per Tic, that means approx 776.723 milliTics per second
	
	Assuming a precise 16 MHz clock:
	86400 seconds per day * 16mHz = 1,382,400,000,000 cycles per day
	65536 Tics per day, 21093750 cycles per Tic, 20599.365234375 cycles per milliTic
	since it's not possible to measure 20599.365234375 cycles, we round down.
	20599 cycles per milliTic is doable using a Timer1 interrupt.

	The standard Arduino libraries provide a microS() function with 4 microsceond resolution.
	This is an inconvienient time for the Tic timer.
	however it can (optionally) provide a microT() function with a 5 microsecond resolution using Timer 2 and freeing up Timer 1 and potentially Timer 0
	
	What this library actually provides (Assuming a precise 16Mhz clock):
	if you #define MICROTIC
		you get the microT() function that returns time in 5 microsecond increments
			1 microT every 80 clock cycles == 5 micro seconds
			1 milliTic == 1.28 milliseconds : ==  256 microT : == 20480 cycles
			1 Tic = 1.31072 seconds : == 1024 milliT : == 20971520 cycles
		Clock runs approx 500 seconds fast per day, or approx 400-450 seconds for a typical mini-pro using a ceramic oscillator (tend to run slow)
		Need to do TimeHacks at least once every 3 minutes to keep clocks synced
	#else
		You get a more precise Tic clock, but lose the microT() function
			1 miliTic == 1.2874375 miliseconds : 1 miliTic == 20599 cycles
			1 Tic == 1.318336 seconds : 1 Tic == 1024 milliT : 1 Tic == 21093376 clock cycles
		clock runs approx 1.5 seconds slow per day  +/- accuracy of the 16Mhz clock
		only *need* to do timehacks a few times per day to keep clock synced.	Doesn't hurt to sync more often
/**/ 

#include <Tic.h>

void ticSetup() {
#ifdef COUNTER_1_MODE 
	pinMode(2, INPUT);
	attachInterrupt( 0, ISR0, COUNTER_1_MODE );
#endif
#ifdef COUNTER_1_DEBOUNCE
	pinMode(2, INPUT);
#endif
#ifdef COUNTER_1_PULLUP 
	digitalWrite(2, HIGH);
#endif
#ifdef COUNTER_2_MODE 
	pinMode(3, INPUT);
	attachInterrupt( 0, ISR1, COUNTER_2_MODE );
#endif
#ifdef COUNTER_2_DEBOUNCE
	pinMode(3, INPUT);
#endif
#ifdef COUNTER_2_PULLUP 
	digitalWrite(3, HIGH);
#endif
#ifdef COUNTER_3_DEBOUNCE
	pinMode(4, INPUT);
#endif
#ifdef COUNTER_3_PULLUP 
	digitalWrite(4, HIGH);
#endif
#ifdef COUNTER_4_DEBOUNCE
	pinMode(5, INPUT);
#endif
#ifdef COUNTER_4_PULLUP 
	digitalWrite(5, HIGH);
#endif

    cli();                      
#ifdef MICROTIC
	//use microT(), higher precision, but not as accurate over long durations 
    TCCR2A = 0;     // set entire TCCR2A register to 0                
    TCCR2B = 0;     // set entire TCCR2B register to 0            
    TCNT2  = 0;     // initialize counter value to 0                        
    TCCR2B |= (1 << WGM12);    // turn on CTC mode 
    TCCR2B |= (1 << CS10);     // Set CS10 bit for no prescaler
    OCR2A = 80;  // no prescaller = 1 interupt every 81 cycles = 1 microT (~5 microseconds), microT * 256 = milliTic * 1024 = 1 Tic, 1.5 second error per day
    TIMSK2 |= (1 << OCIE2A);   // enable timer compare interrupt
#else
	// use miliTic interrupt, more accurate but lose microT()
    TCCR1A = 0;     // set entire TCCR1A register to 0                
    TCCR1B = 0;     // set entire TCCR1B register to 0            
    TCNT1  = 0;     // initialize counter value to 0                        
    TCCR1B |= (1 << WGM12);    // turn on CTC mode 
    TCCR1B |= (1 << CS10);     // Set CS10 bit for no prescaler
    OCR1A = 20598;  // no prescaller = 1 interupt every 20599 cycles = 1 mTic * 1024 = 1 Tic, 1.5 second error per day
    TIMSK1 |= (1 << OCIE1A);   // enable timer compare interrupt
#endif
    sei();//enable interrupts  
}



// (uint8_t)TimersEnabled is a global variable defined by Circus code and is a bit mask for which timers should run each day
//Tic timer copies TimersEnabled to timersRun at midnight.  When each timer executes it's bit in timersRun is set to zero
//user program can disable a timer at any time by changing it's bit in TimersEnabled
//user code needs to call this with something like:  if (timersRun) timerControl();
#if TIMERS >= 1
void timerControl() {
	if (timersRun & 0x01 && Tic >= Timer1) {  		// has this timer run today?  If not, is it timen ,  to run yet?
		if (TimersEnabled & 0x01) TIMER_1_MACRO; // if timer is still allowed to run, jump to timer1Function()
		timersRun &= ~0x01;							// this Timer is done for today, set it's bit to zero
	}
#if TIMERS >= 2
	if (timersRun & 0x02 && Tic >= Timer2) {
		if (TimersEnabled & 0x02) TIMER_2_MACRO;
		timersRun &= ~0x02;
	}
#endif
#if TIMERS >= 3
	if (timersRun & 0x04 && Tic >= Timer3) {
		if (TimersEnabled & 0x04) TIMER_3_MACRO;
		timersRun &= ~0x04;
	}
#endif
#if TIMERS >= 4
	if (timersRun & 0x08 && Tic >= Timer4) {
		if (TimersEnabled & 0x08) TIMER_4_MACRO;
		timersRun &= ~0x08;
	}		
#endif
}
#endif	

#ifdef WEEKDAY
uint8_t dayOfWeek() {
    return DayOfWeek;
}
#endif

uint16_t nowT() {
	uint16_t rval;
	uint8_t statusReg = SREG;
	cli(); 
	rval = Tic;
	SREG = statusReg;
	return rval;
}

uint16_t milliT(){
	uint16_t rval;
	uint8_t statusReg = SREG;
	cli(); 
	rval = mTic;
	SREG = statusReg;
	return rval;
}


/* waitT limited to 65535 mTic (84 seconds) */
void waitT(uint16_t mTics) {
    uint16_t now = mTic;
    while (mTics) {
        if (now != mTic) {
            now = mTic;
            mTics-- ;
#ifdef CIRCUS
			Circus();
#endif
        }
    }
}

#ifdef COUNTER_1_MODE 
void ISR0() {
	counter1++;
}
#endif
#ifdef COUNTER_2_MODE 
void ISR1() {
	counter2++;
}
#endif



uint16_t microT() {
	return microTic * 5;
}

void delayMicroTics(uint16_t mTics) {
    uint8_t now = microTic;
	mTics = (mTics + 2) / 5;
    while (mTics) {
        if (now != microTic) {
            now = microTic;
            --mTics;
	    }
    }
}

#ifdef MICROTIC
ISR(TIMER2_COMPA_vect){ //timer2 interrupt ~197.5 kHz,  every 5 microseconds	
	++microTic;
	if (!microTic){
#else
ISR(TIMER1_COMPA_vect){ //timer1 interrupt ~777 Hz, 	
#endif

		mTic++;  
#ifdef DO_DEBOUNCE
		if (!(mTicLo & DEBOUNCE_TIME )){ // default 64 mTic debounce
			static uint8_t oldStatus
			uint8_t newStat = (PIND & DEBOUNCE_MASK);
			uint8_t difference = newStat ^ oldStatus ; //result of XOR will be zero unless a bit has changed
			//byte newStat = digitalRead(WATER_METER);
			if(difference) {
#ifdef COUNTER_1_DEBOUNCE
				if (difference &  DEBOUNCE1) {
#if COUNTER_1_DEBOUNCE = 0
					if (oldStatus &  DEBOUNCE1)) Counter1++;
#elif COUNTER_1_DEBOUNCE = 1				
					if (newStat &  DEBOUNCE1) Counter1++;
#elif COUNTER_1_DEBOUNCE = 2
					Counter1++;
#endif
				}
#endif			
#ifdef COUNTER_2_DEBOUNCE
				if (difference &  DEBOUNCE2) {
#if COUNTER_2_DEBOUNCE = 0
					if (oldStatus &  DEBOUNCE2)) Counter2++;
#elif COUNTER_2_DEBOUNCE = 1				
					if (newStat &  DEBOUNCE2) Counter2++;
#elif COUNTER_2_DEBOUNCE = 2
					Counter2++;
#endif
				}
#endif
#ifdef COUNTER_3_DEBOUNCE
				if (difference &  DEBOUNCE3) {
#if COUNTER_3_DEBOUNCE = 0
					if (oldStatus &  DEBOUNCE3)) Counter3++;
#elif COUNTER_3_DEBOUNCE = 1				
					if (newStat &  DEBOUNCE3) Counter3++;
#elif COUNTER_3_DEBOUNCE = 2
					Counter3++;
#endif
				}
#endif	
#ifdef COUNTER_4_DEBOUNCE
				if (difference &  DEBOUNCE4) {
#if COUNTER_4_DEBOUNCE = 0
					if (oldStatus &  DEBOUNCE4)) Counter4++;
#elif COUNTER_4_DEBOUNCE = 1				
					if (newStat &  DEBOUNCE4) Counter4++;
#elif COUNTER_4_DEBOUNCE = 2
					Counter4++;
#endif
				}
#endif
				oldStatus = newStat;
			}
		}
	
#endif	
		if (!(mTic & 0x03ff)){
			Tic++;
			if (!Tic) {  /* midnight */
#ifdef WEEKDAY
				DayOfWeek++;  //increment day of week after midnight
				if (DayOfWeek > 7) DayOfWeek = 1;
#endif
#ifdef TIMERS
				timersRun = TimersEnabled;
#endif
			}
#ifdef TIC_MACRO
			TIC_MACRO
#endif			
		}
#ifdef CIRCUS
		if (deadTime) deadTime--;
#endif
#ifdef MICROTIC
	}
#endif
}

