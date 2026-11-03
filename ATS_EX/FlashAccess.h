#pragma once

#include <stdint.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#define RADIO_PROGMEM PROGMEM
inline uint8_t flashRead8(const uint8_t* value) { return pgm_read_byte(value); }
inline uint16_t flashRead16(const uint16_t* value) { return pgm_read_word(value); }
#else
#define RADIO_PROGMEM
inline uint8_t flashRead8(const uint8_t* value) { return *value; }
inline uint16_t flashRead16(const uint16_t* value) { return *value; }
#endif
