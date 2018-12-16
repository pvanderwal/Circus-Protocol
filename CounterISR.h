#pragma once

#include <stdint.h>
#include <digitalWriteFast.h>
#include <avr/interrupt.h>
#include <Circus.h>

#ifdef __cplusplus
extern "C" {
#endif

void ISR0() {
	CIRCUS_COUNTER++;
}

void setupCounterISR(uint8_t pin, uint8_t pullUp, uint8_t mode){
	pinModeFast(pin, INPUT);
	attachInterrupt( pin-2, ISR0, mode );
	//internal pullup resistor
	digitalWrite(pin, pullUp);	
}

#ifdef __cplusplus
}
#endif


