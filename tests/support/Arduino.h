#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t DDRB, DDRC, DDRD, PORTB, PORTC, PORTD, PINB, PINC, PIND, CLKPR;
extern uint32_t testMillis;
extern int testAnalogSample;
inline uint32_t millis() { return testMillis; }
inline void delay(uint32_t ms) { testMillis += ms; }
inline int analogRead(uint8_t) { return testAnalogSample; }
inline void noInterrupts() {}
inline void interrupts() {}
inline uint8_t digitalPinToInterrupt(uint8_t pin) { return pin; }
inline void attachInterrupt(uint8_t, void (*)(), uint8_t) {}
#define HIGH 1
#define LOW 0
#define A2 16
#define DDB5 5
#define CHANGE 1
// Host tests execute serially; the AVR build uses the real util/atomic.h.
#define ATOMIC_BLOCK(mode) for (bool atomicOnce = true; atomicOnce; atomicOnce = false)
#define ATOMIC_RESTORESTATE 0
