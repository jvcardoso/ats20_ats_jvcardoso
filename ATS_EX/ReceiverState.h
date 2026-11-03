#pragma once

#include <stdint.h>
#include "defs.h"

#ifdef __AVR__
#include <avr/pgmspace.h>
#define STATE_PROGMEM PROGMEM
#else
#define STATE_PROGMEM
#endif

// Versions 4 and 5 distinguish the base-frequency layout without/with RDS.
// Older records used effective kHz for SSB and included a CPU-divider setting.
constexpr uint8_t STATE_FORMAT = 4 + USE_RDS;
constexpr uint8_t STATE_APP_ID = 235;
constexpr int16_t STATE_MAX_BFO_HZ = 15000;
constexpr int16_t STATE_MAX_BFO_KHZ = STATE_MAX_BFO_HZ / 1000;
constexpr int8_t STATE_MAX_BFO_CALIBRATION = 60;
constexpr int16_t STATE_BFO_CALIBRATION_SCALE = 10;
static_assert(STATE_MAX_BFO_HZ % 1000 == 0, "Carrier rollover requires whole kHz");
static_assert(STATE_MAX_BFO_HZ + STATE_MAX_BFO_CALIBRATION * STATE_BFO_CALIBRATION_SCALE <= 16383,
    "BFO plus calibration exceeds the Si4735 range");
constexpr uint16_t STATE_DATA_ADDRESS = 1;
constexpr uint16_t STATE_CRC_ADDRESS = 999;
constexpr uint16_t STATE_FORMAT_ADDRESS = 1000;
constexpr uint8_t STATE_SETTINGS_COUNT = USE_RDS ? 15 : 14;

struct SavedBand
{
    uint16_t frequency;
    int8_t step;
    int8_t bandwidth;
} __attribute__((packed));

struct ReceiverState
{
    uint8_t volume;
    uint8_t band;
    uint8_t mode;
    int8_t ssbBandwidth;
    int16_t bfo;
    SavedBand bands[4];
    int8_t settings[STATE_SETTINGS_COUNT];
} __attribute__((packed));

static_assert(sizeof(ReceiverState) == 22 + STATE_SETTINGS_COUNT, "EEPROM layout changed");
static_assert(sizeof(ReceiverState) < 256, "CRC loop requires an 8-bit record length");

inline int8_t stateReadLimit(const int8_t* value)
{
#ifdef __AVR__
    return int8_t(pgm_read_byte(value));
#else
    return *value;
#endif
}

inline bool validStep(int8_t step, uint8_t band, uint8_t mode)
{
    if (step < 0)
        return false;
    if (band == 3)
        return step < 3;
    if (mode > 0 && mode < 4)
        return step < 4 || (step >= 7 && step < 12);
    return step < 7 && (band == 2 || step <= 4);
}

inline bool validReceiverState(const ReceiverState& state)
{
    if (state.volume > 63 || state.band > 3 || state.mode > 4
        || (state.band == 3) != (state.mode == 4)
        || uint8_t(state.ssbBandwidth) > 5
        || state.bfo < -STATE_MAX_BFO_HZ || state.bfo > STATE_MAX_BFO_HZ)
        return false;
    if ((state.mode == 0 || state.mode == 4) && state.bfo != 0)
        return false;
    if (!validStep(state.bands[state.band].step, state.band, state.mode))
        return false;

    for (uint8_t i = 0; i < 4; i++)
    {
        const SavedBand& band = state.bands[i];
        uint16_t minimum = i == 0 ? 153 : i == 1 ? 520 : i == 2 ? 1710 : 6400;
        uint16_t maximum = i == 0 ? 520 : i == 1 ? 1710 : i == 2 ? 30000 : 10800;
        if (band.frequency < minimum || band.frequency > maximum
            || uint8_t(band.step) >= (i == 3 ? 3 : 12)
            || uint8_t(band.bandwidth) > (i == 3 ? 4 : 6))
            return false;
        if (i == state.band && state.mode > 0 && state.mode < 4)
        {
            uint16_t lowerGapKHz = band.frequency - minimum;
            uint16_t upperGapKHz = maximum - band.frequency;
            if ((lowerGapKHz <= 15 && state.bfo < -int16_t(lowerGapKHz * 1000))
                || (upperGapKHz <= 15 && state.bfo > int16_t(upperGapKHz * 1000)))
                return false;
        }
    }

    static const int8_t limits[][2] STATE_PROGMEM =
    {
        {0, 37}, {0, 32}, {0, 1}, {0, 1}, {0, 1}, {12, 90},
        {5, 125}, {0, 1}, {0, 1}, {0, 2},
#if USE_RDS
        {0, 3},
#endif
        {-STATE_MAX_BFO_CALIBRATION, STATE_MAX_BFO_CALIBRATION}, {0, 1}, {0, 1}, {0, 1}
    };
    static_assert(sizeof(limits) / sizeof(limits[0]) == STATE_SETTINGS_COUNT,
        "Settings limits table mismatch");
    for (uint8_t i = 0; i < STATE_SETTINGS_COUNT; i++)
        if (state.settings[i] < stateReadLimit(&limits[i][0])
            || state.settings[i] > stateReadLimit(&limits[i][1]))
            return false;
    return true;
}

inline uint8_t receiverStateCrc(const ReceiverState& state)
{
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&state);
    uint8_t crc = 0;
    for (uint8_t i = 0; i < sizeof(state); i++)
    {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8; bit++)
            crc = (crc << 1) ^ ((crc & 0x80) ? 0x07 : 0);
    }
    return crc;
}

template<class Storage>
bool loadReceiverState(Storage& storage, ReceiverState& state)
{
    if (storage.read(0) != STATE_APP_ID || storage.read(STATE_FORMAT_ADDRESS) != STATE_FORMAT)
        return false;
    storage.get(STATE_DATA_ADDRESS, state);
    return storage.read(STATE_CRC_ADDRESS) == receiverStateCrc(state) && validReceiverState(state);
}

template<class Storage>
void storeReceiverState(Storage& storage, const ReceiverState& state)
{
    storage.update(STATE_FORMAT_ADDRESS, 0);
    storage.put(STATE_DATA_ADDRESS, state);
    storage.update(STATE_CRC_ADDRESS, receiverStateCrc(state));
    storage.update(0, STATE_APP_ID);
    storage.update(STATE_FORMAT_ADDRESS, STATE_FORMAT);
}
