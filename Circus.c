/*************************************************************************
Title:    Circus Ring library 
Author:   Peter VanDerWal
File:    
Software: 
Hardware: Currently works with Atmega328, could probably work with any AVR with a built-in UART, 
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

    
DESCRIPTION:
    An interrupt driven 'token' ring network using AVR built in UARTs at TTL levels with Cat-5 wiring
    
USAGE:
    Refer to the header file Circus.h for a description of the routines. 
    See also example generic_node.c.
                    
LICENSE:
    Copyright (C) 2015 Peter VanDerWal, GNU General Public License Version 3

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                       
   Must define nodeID (NID)
   Valid nodeIDs: 0x10, 0x20, 0x30 ... 0xF0
   0x00 = All nodes, mostly used for TimeHack: 0x0F
   
*************************************************************************/
//#include <avr/io.h>
//#include <avr/interrupt.h>
//#include <avr/pgmspace.h>

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <Circus.h>

#ifndef DEADTIME
#define DEADTIME 5
#endif


//#include "Uart.h"

// =============================================== UART Definitions ===================================================
 #define UART0_RECEIVE_INTERRUPT   USART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01

//UART


/* Token structure:
0x00:	Low byte of int data
0x01:	High byte of int data
0x02:	targetID, R/W, 3bit registerID
0x03:	crc
*/

// pete Token is not exposed to user, rewrite without macros
union {
  uint16_t uIntData;
  int16_t intData;
  uint8_t buffer[5];
} volatile static Token;
#define UartError Token.buffer[4] 	// allows a one byte overrun,

volatile uint8_t RxIdx;
//volatile uint16_t * Tic;

volatile uint8_t _deadtime;
static volatile uint8_t Crc;
static volatile uint8_t TxCrc;
static volatile uint8_t TxIdx;

const uint8_t CRCSEED=0x88;

Circus_Data_Array CDA;

// Errors are returned by exclusive or-ing the error code with the register nibble changing target node to current node
#define BUFFER_ERROR 0x0B
#define CRC_ERROR 0x0C
#define  UART_ERROR 0x0D

/*************************************************************************
Function: circus_init()
Purpose:  initialize UART and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
//pete  Mixing and matching defined constants for UART, clean this up, maybe define myself?
void circus_init()
{   
    uint16_t baudrate = F_CPU / (16 * BAUD)- 1 ;

	UBRR0H  = (baudrate >> 8);
	UBRR0L  = baudrate;

	// not using 2x speed att, 1x better has better noise immunity less critical timing
	UCSR0A = 0;	

    /* Enable USART receiver and transmitter and receive complete interrupt */
    UCSR0B = _BV(RXCIE0)|(1<<RXEN0)|(1<<TXEN0);

	// Set frame format = 8-N-1
	UCSR0C = 0x06;
	
	if (DEBOUNCE_TIME) {
		pinModeFast(DEBOUNCE_PIN, INPUT);
		digitalWriteFast(DEBOUNCE_PIN, DEBOUNCE_PULLUP);
	}
	setupTic();
	
	
}/* circus_init */

void nodeControl(uint8_t target_Reg) { /* empty */ }

/*************************************************************************
Function: Circus()
Purpose:  Processes tokens, Runs between instances of Loop() 
Input:    none
Returns:  none
**************************************************************************/
void Circus(void) 
{
	uint8_t target = 0;
//if yield works then Circus() is only called when RxIdx > 3
	// disable rx interrupt	
	UCSR0B &= ~_BV(RXCIE0);

	if (RxIdx = 4) { //rx only complete token, no overrun
		uint8_t crc = crc8(crc8(crc8(CRCSEED,Token.buffer[0]),Token.buffer[1]),Token.buffer[2]) ;
/*		if (UartError) {
			Token.buffer[0] = UartError;
			Token.buffer[2] = NID + (UART_ERROR^(Token.buffer[2]&0x0f));
		} else */
		if ( crc == Token.buffer[3] ) { // valid CRC
			uint8_t Tid = (Token.buffer[2]&0xF0);
			if ( NID == Tid || !Tid ) {		// if addressed to this node
				uint8_t reg = Token.buffer[2]&0x07;
				uint16_t reply = CDA.uintD[reg]; //set reply to data at requested register
				target = Token.buffer[2];
				if ( target & 0x08) { //if "store" data.  Note: Both Store or Get, returned value will be previous data at selected location
					CDA.uintD[reg]=Token.uIntData;
				} else {
				}
				if (NID == Tid) {
					Token.uIntData=reply; 
					if (!reg) {					// if reading or setting register[0]
						CIRCUS_f_NEWSTAT = 0;   // clear NewStat flag since reply contains original value for register[0]
					}					
				}
			}
			
		} else { //crc error
			Token.buffer[0] = crc;  			//calculated crc
			Token.buffer[1] = Token.buffer[3];	//received crc
			Token.buffer[2] = NID + (CRC_ERROR^Token.buffer[2]&0x0f);
		}
	} else {  //buffer overrun
		Token.buffer[0]=RxIdx;
		Token.buffer[1]=TxIdx;
		Token.buffer[2] = NID + (BUFFER_ERROR^Token.buffer[2]&0x0f); 
	}
	TxCrc = crc8(crc8(crc8(CRCSEED,Token.buffer[0]),Token.buffer[1]),Token.buffer[2]);
	TxIdx = 0;
	//enable tx interrupt
	UCSR0B &= _BV(UDRIE0);
	RxIdx = 0;
	//enable rx interrupt
	UCSR0B &= _BV(RXCIE0);
	
	if (target)					//user defined nodeControl is only called when node token is addressed to current node
		nodeControl(target);	//nodeControl what action (if any) is needed)
}

uint8_t crc8(uint8_t crc_data, uint8_t crc) {
    uint8_t i;
    i = (crc_data ^ crc) & 0xff;
    crc = 0;
    if (i & 1)   crc ^= 0x5e;
    if (i & 2)   crc ^= 0xbc;
    if (i & 4)   crc ^= 0x61;
    if (i & 8)   crc ^= 0xc2;
    if (i & 0x10)  crc ^= 0x9d;
    if (i & 0x20)  crc ^= 0x23;
    if (i & 0x40)  crc ^= 0x46;
    if (i & 0x80)  crc ^= 0x8c;
    return(crc);
}

//*********************************** Timers ******************************************************//

void timerControl() {
	uint8_t doTimers = _timersRun & CIRCUS_TimersEnabled; 	//bit mask, 1 = timer# is_enable AND has not run today
	if ( doTimers & 0x01 && Tic >= CDA.uintD[1]) { // has this timer run today?  If not, is it time to run yet?
		if (CDA.control.timer1) TIMER_1(1); 	// is timer still allowed to run?
		_timersRun &= ~0x01;					// this Timer is done for today, set it's bit to zero
	}
	if (TIMERS >= 2) {
		if (doTimers & 0x02 && Tic >= CDA.uintD[2]) {
			if (CDA.control.timer2) TIMER_2(2);
			_timersRun &= ~0x02;
		}
	}
	if (TIMERS >= 3){
		if (doTimers & 0x04 && Tic >= CDA.uintD[3]) {
			if (CDA.control.timer3) TIMER_3(3);
			_timersRun &= ~0x04;
		}
	}
	if (TIMERS >= 4){
		if (doTimers & 0x08 && Tic >= CDA.uintD[4]) {
			if (CDA.control.timer4) TIMER_4(4);
			_timersRun &= ~0x08;
		}		
	}
}



//***********************************  Hooks into the Arduino environment ****************************************//

void initVariant() {
    circus_init();
}

void yield(void) {		//yield runs before loop
    if (RxIdx>3)
		Circus(); //process token
	if (TIMERS && _timersRun)
		timerControl();
}

//********************************************  Transmit and Receive ISRs  *******************************************//

/*************************************************************************
Function: UART Receive interrupt
Purpose:  called when the UART has received a character
**************************************************************************/
ISR (UART0_RECEIVE_INTERRUPT)        
{

    if (_deadtime) {
		_deadtime = DEADTIME;
	} else {  //dead time expired, either a new token or lost data 
		RxIdx = 0 ;  //reset to begining of token
	}

    // Pete fix this?
	UartError |= UART0_STATUS & (_BV(FE0)|_BV(DOR0) ) ;    //check for UART Framing and/or Data Over Run errors 

	//pete no need for seperate tx buffer, set Ringmaster to only allow one token at a time, 
	//or send slow enough to process without over running.
	Token.buffer[RxIdx++]=UART0_DATA;
	return;
} // UART recieve ISR
	
/*************************************************************************
Function: UART Data Register Empty interrupt
Purpose:  called when the UART is ready to transmit the next uint8_t
**************************************************************************/
ISR (UART0_TRANSMIT_INTERRUPT) 
{
	if (TxIdx < 4) { //transmit data
		UART0_DATA = Token.buffer[TxIdx++];  
	}else{
        /* tx buffer empty, disable UDRE interrupt */
        UCSR0B &= ~_BV(UDRIE0);
    }
}
