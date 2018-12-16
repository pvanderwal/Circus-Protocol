#pragma once

#include <digitalWriteFast.h>
#include <Circus.h>

#ifdef __cplusplus
extern "C" {
#endif

void setupCounterDebounce(uint8_t pin, uint8_t pullUp, uint8_t mode){
	debounce = debounce;
	_counterPin = pin;
	pinModeFast(pin, INPUT);
	digitalWriteFast(pin, pullUp);
}

#ifdef __cplusplus
}
#endif
