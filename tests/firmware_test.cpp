#include "Arduino.h"
#include <algorithm>
#include <array>
#include <assert.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "SimpleButton.h"
#include "Rotary.h"
#include "defs.h"
#include "ReceiverState.h"
#include "RdsText.h"

uint8_t DDRB, DDRC, DDRD, PORTB, PORTC, PORTD, PINB = 255, PINC = 255, PIND = 255, CLKPR;
uint32_t testMillis;
int testDelayCount;
int testAnalogSample = 600;
void (*testDelayHook)(uint32_t) = nullptr;
struct DCfont {};
const DCfont testFont;
const DCfont* const FONT8X16POB = &testFont;
const uint8_t tiny4koled_init_128x64br[] = {0};
void rotaryEncoder();

struct TestStorage
{
    std::array<uint8_t, 1024> bytes;
    int operations = 0;
    int physicalWrites = 0;
    int cutAfter = -1;
    TestStorage() { bytes.fill(255); }
    uint8_t read(uint16_t address) { return bytes.at(address); }
    void update(uint16_t address, uint8_t value)
    {
        if (operations++ == cutAfter)
            throw std::runtime_error("power interrupted between byte operations");
        if (bytes.at(address) != value)
            ++physicalWrites;
        bytes.at(address) = value;
    }
    template<class T> void get(uint16_t address, T& value)
    { memcpy(&value, bytes.data() + address, sizeof(value)); }
    template<class T> void put(uint16_t address, const T& value)
    {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&value);
        for (size_t i = 0; i < sizeof(value); i++) update(address + i, data[i]);
    }
} EEPROM;

struct SI4735
{
    uint8_t volume = 45;
    uint16_t frequency = 1476;
    int bfo = 0;
    int rdsConfigCalls = 0;
    int setFrequencyCalls = 0;
    int lastSeekFmSpacing = 0;
    int lastSeekAmSpacing = 0;
    bool hardwareSeeking = false;
    int cancelCommands = 0;
    int seekCalls = 0;
    int rdsStatusCalls = 0;
    uint16_t seekStartFrequency = 0;
    uint16_t currentStep = 10;
    uint16_t finalSeekFrequency = 0;
    bool failFrequencyRead = false;
    uint16_t frequencyReadValue = 0;
    std::vector<uint16_t> frequencyRequests;
    std::vector<int> bfoRequests;
    int lastSidebandCutoff = -1;
    uint16_t seekCallbackFrequency = 1477;
    bool rotateDuringSeek = false;
    bool seekStopped = false;
    bool received = false;
    char station[9] = "STATION";
    char information[33] = "station information beyond 16";
    char program[65] = "program information beyond 16 columns";
    void (*volumeHook)() = nullptr;
    uint8_t getVolume() { return volume; }
    uint8_t getCurrentVolume() { return volume; }
    void setVolume(uint8_t value) { volume = value; }
    void volumeUp() { if (volume < 63) ++volume; if (volumeHook) volumeHook(); }
    void volumeDown() { if (volume) --volume; }
    void setFrequency(uint16_t value) { frequency = value; ++setFrequencyCalls; frequencyRequests.push_back(value); }
    uint16_t getFrequency()
    {
        // PU2CLR 2.1.8 implements this read using getStatus(0, 1).
        if (hardwareSeeking)
        {
            hardwareSeeking = false;
            ++cancelCommands;
            if (finalSeekFrequency) frequency = finalSeekFrequency;
        }
        if (failFrequencyRead)
        {
            failFrequencyRead = false;
            return frequencyReadValue;
        }
        return frequency;
    }
    void frequencyUp() { frequency += currentStep; }
    void frequencyDown() { frequency -= currentStep; }
    void setSSBBfo(int value) { bfo = value; bfoRequests.push_back(value); }
    void setSSBSidebandCutoffFilter(uint8_t value) { lastSidebandCutoff = value; }
    void setSeekFmSpacing(uint16_t value) { lastSeekFmSpacing = value; }
    template<class... Args> void setSSB(uint16_t low, uint16_t high, uint16_t value, Args...)
    { frequency = std::max(low, std::min(high, value)); }
    void setAM(uint16_t low, uint16_t high, uint16_t value, uint16_t step)
    { currentStep = step; frequency = std::max(low, std::min(high, value)); }
    void setFM(uint16_t low, uint16_t high, uint16_t value, uint16_t step)
    { currentStep = step; frequency = std::max(low, std::min(high, value)); }
    template<class... Args> void setRdsConfig(Args...) { ++rdsConfigCalls; }
    bool getRdsReceived() { return received; }
    bool getRdsSync() { return received; }
    uint8_t getNumRdsFifoUsed() { return received ? 3 : 0; }
    char* getRdsStationName() { return received ? station : nullptr; }
    char* getRdsStationInformation() { return received ? information : nullptr; }
    char* getRdsProgramInformation() { return received ? program : nullptr; }
#define RADIO_NOOP(name) template<class... Args> void name(Args...) {}
    RADIO_NOOP(getDeviceI2CAddress)
    RADIO_NOOP(setup)
    RADIO_NOOP(setSSBAudioBandwidth)
    RADIO_NOOP(setTuneFrequencyAntennaCapacitor)
    RADIO_NOOP(setSeekFmLimits)
    RADIO_NOOP(setFifoCount)
    RADIO_NOOP(setFmBandwidth)
    RADIO_NOOP(setFMDeEmphasis)
    RADIO_NOOP(setSSBAutomaticVolumeControl)
    RADIO_NOOP(setSSBDspAfc)
    RADIO_NOOP(setSSBAvcDivider)
    RADIO_NOOP(setAmSoftMuteMaxAttenuation)
    RADIO_NOOP(setSSBSoftMute)
    RADIO_NOOP(setBandwidth)
    RADIO_NOOP(setAvcAmMaxGain)
    RADIO_NOOP(setSeekAmLimits)
    void setSeekAmSpacing(uint16_t value) { lastSeekAmSpacing = value; }
    void setFrequencyStep(uint16_t value) { currentStep = value; }
    RADIO_NOOP(setAutomaticGainControl)
    void getRdsStatus() { ++rdsStatusCalls; }
#undef RADIO_NOOP
    void seekStationProgress(void (*show)(uint16_t), bool (*stop)(), uint8_t)
    {
        ++seekCalls;
        seekStartFrequency = frequency;
        hardwareSeeking = true;
        frequency = seekCallbackFrequency;
        if (show)
            show(seekCallbackFrequency);
        if (rotateDuringSeek)
        {
            const uint8_t phases[] = {1, 0, 2, 3};
            for (uint8_t phase : phases)
            {
                PIND = (PIND & ~12) | (phase << 2);
                rotaryEncoder();
            }
        }
        seekStopped = stop && stop();
    }
};

#include "globals.h"
static_assert(STATE_SETTINGS_COUNT == SettingsIndex::SETTINGS_MAX, "Test layout mismatch");
std::string lastRdsLine;
int displayPrintCalls = 0;
std::vector<std::string> displayTexts;
struct Display
{
    template<class... Args> void begin(Args...) {}
    void clear() {}
    void on() {}
    void off() {}
    void setFont(const DCfont*) {}
    void invertOutput(bool) {}
    void setContrast(uint8_t) {}
    void setCursor(int, int) {}
    void print(const char* text) { lastRdsLine = text; ++displayPrintCalls; displayTexts.push_back(text); }
} oled;
#include "Utils.h"
void showStatus(bool = false) {}
void showFrequency(bool = false) {}
void showSMeter() {}
void showSettings() {}
void showSettingsTitle() {}
void switchSettingsPage() {}
void DrawSetting(uint8_t, bool) {}
void updateLowerDisplayLine() {}
void loadSSBPatch() { g_ssbLoaded = true; }
void applyBandConfiguration(bool = false);
#include "FirmwareFunctions.inc"

void resetRuntime()
{
    static const std::array<Band, 4> bands = {g_bandList[0], g_bandList[1], g_bandList[2], g_bandList[3]};
    static const std::array<SettingsItem, STATE_SETTINGS_COUNT> settings = [] {
        std::array<SettingsItem, STATE_SETTINGS_COUNT> value;
        std::copy(g_Settings, g_Settings + STATE_SETTINGS_COUNT, value.begin());
        return value;
    }();
    std::copy(bands.begin(), bands.end(), g_bandList);
    std::copy(settings.begin(), settings.end(), g_Settings);
    testMillis = 0; g_storeTime = 0; g_settingsDirty = false;
    g_encoderCount = 0; g_seekStop = false; g_seekInProgress = false; g_currentBFO = 0;
    g_bandIndex = MW_BAND_TYPE; g_currentMode = FM; g_ssbLoaded = false;
    g_volume = DEFAULT_VOLUME; g_muteVolume = 0; g_bwIndexSSB = 4; g_stepIndex = 3;
    g_cmdVolume = g_cmdStep = g_cmdBw = g_cmdBand = false;
    g_settingsActive = g_SettingEditing = g_sMeterOn = g_displayRDS = g_rdsSwitchPressed = false;
    g_displayOn = true; g_processFreqChange = false; g_lastFreqChange = g_lastAdjustmentTime = 0;
    g_currentFrequency = 1476; g_si4735 = SI4735();
    testAnalogSample = 600;
    testDelayHook = nullptr;
    PINB = PINC = PIND = 255;
    btn_AGC = SimpleButton(AGC_BUTTON); btn_Step = SimpleButton(STEP_BUTTON);
    btn_VolumeUp = SimpleButton(VOLUME_BUTTON); btn_Mode = SimpleButton(MODE_SWITCH);
    btn_Encoder = SimpleButton(ENCODER_BUTTON);
    g_encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);
}

void ssb(int offset)
{
    g_bandIndex = SW_BAND_TYPE; g_currentMode = USB; g_ssbLoaded = true;
    g_currentFrequency = 7100; g_currentBFO = offset;
    g_bandList[SW_BAND_TYPE].currentFreq = 7100;
    g_bandList[SW_BAND_TYPE].currentStepIdx = 8;
}

void pin(uint8_t number, bool high)
{
    uint8_t& input = number < 8 ? PIND : number < 14 ? PINB : PINC;
    uint8_t bit = number < 8 ? number : number < 14 ? number - 8 : number - 14;
    if (high) input |= 1 << bit; else input &= ~(1 << bit);
}

void shortPress(uint8_t number)
{
    pin(number, false); handleButtons();
    testMillis += 16; handleButtons();
    pin(number, true); handleButtons();
    testMillis += 16; handleButtons();
    testMillis += 80; handleButtons();
}

int shortButtonEvents = 0;
uint8_t countShortButtonEvents(uint8_t event, uint8_t)
{
    if (event == BUTTONEVENT_SHORTPRESS)
        ++shortButtonEvents;
    return event;
}

void testBatteryInitialSample()
{
    resetRuntime();
    g_voltagePinConnnected = true;
    testAnalogSample = 600;
    showCharge(true);
    assert(lastRdsLine == "88%");
    testAnalogSample = 0;
    showCharge(true);
    showCharge(true);
    assert(lastRdsLine == " 0%");
    testAnalogSample = 600;
    showCharge(true);
    assert(lastRdsLine == " 0%"); // A real ADC zero must not be treated as uninitialized.
    showCharge(true);
    assert(lastRdsLine == "88%");
    std::cout << "PASS startup battery: first display uses the initial ADC sample\n";
}

void testShortReleaseDebounce()
{
    testMillis = 0;
    shortButtonEvents = 0;
    pin(STEP_BUTTON, true);
    SimpleButton button(STEP_BUTTON);
    pin(STEP_BUTTON, false); button.checkEvent(countShortButtonEvents);
    testMillis += 16; button.checkEvent(countShortButtonEvents);
    pin(STEP_BUTTON, true); testMillis += 16; button.checkEvent(countShortButtonEvents);
    pin(STEP_BUTTON, false); testMillis += 16; button.checkEvent(countShortButtonEvents);
    pin(STEP_BUTTON, true); testMillis += 16; button.checkEvent(countShortButtonEvents);
    testMillis += 16; button.checkEvent(countShortButtonEvents);
    assert(shortButtonEvents == 1);
    testMillis += 48; button.checkEvent(countShortButtonEvents);
    assert(shortButtonEvents == 1);
    testMillis += 16; button.checkEvent(countShortButtonEvents);

    pin(STEP_BUTTON, false); button.checkEvent(countShortButtonEvents);
    testMillis += 16; button.checkEvent(countShortButtonEvents);
    pin(STEP_BUTTON, true); testMillis += 16; button.checkEvent(countShortButtonEvents);
    testMillis += 16; button.checkEvent(countShortButtonEvents);
    assert(shortButtonEvents == 2);
    std::cout << "PASS buttons: short-release debounce with contact bounce\n";
}

void testResetConfirmation()
{
    resetRuntime(); EEPROM = TestStorage(); setup();
    g_si4735.volume = 45;
    saveAllReceiverInformation();
    ReceiverState saved;
    assert(loadReceiverState(EEPROM, saved) && saved.volume == 45);

    resetRuntime();
    pin(ENCODER_BUTTON, false);
    testDelayHook = [](uint32_t) { PINC |= 1 << (ENCODER_BUTTON - 14); };
    setup();
    testDelayHook = nullptr;
    assert(g_volume == 45);
    assert(g_currentMode == AM);

    resetRuntime();
    pin(ENCODER_BUTTON, false);
    testDelayCount = 0;
    testDelayHook = [](uint32_t) {
        if (++testDelayCount == 16)
            PINC |= 1 << (ENCODER_BUTTON - 14);
    };
    setup();
    testDelayHook = nullptr;
    assert(g_volume == 45);
    assert(g_currentMode == AM);
    std::cout << "PASS EEPROM reset: early and final-tick release preserve settings\n";
}

void testMenuWear()
{
    resetRuntime(); EEPROM = TestStorage(); setup();
    int writes = EEPROM.physicalWrites;
    g_settingsActive = true; switchSettings();
    g_settingsActive = false; switchSettings();
    assert(EEPROM.physicalWrites == writes);

    resetRuntime(); EEPROM = TestStorage(); setup();
    writes = EEPROM.physicalWrites;
    g_settingsActive = true; g_SettingEditing = false;
    g_SettingsPage = 1; g_SettingSelected = 0;
    g_encoderCount = 1;
    loop();
    assert(g_SettingSelected == 1 && !g_settingsDirty);
    testMillis += STORE_TIME + 1; loop();
    assert(EEPROM.physicalWrites == writes);

    resetRuntime(); EEPROM = TestStorage(); setup();
    writes = EEPROM.physicalWrites;
    g_settingsActive = true; g_SettingEditing = true;
    g_SettingsPage = 1; g_SettingSelected = SettingsIndex::Brightness;
    g_encoderCount = 1;
    loop();
    assert(g_settingsDirty);
    g_settingsActive = false; switchSettings();
    assert(!g_settingsDirty && EEPROM.physicalWrites > writes);
    std::cout << "PASS EEPROM wear: menu navigation and empty close do not write\n";
}

void testStorage()
{
    resetRuntime(); EEPROM = TestStorage(); setup();
    ReceiverState initial;
    assert(loadReceiverState(EEPROM, initial));
    assert(initial.volume == 25 && initial.mode == AM);
    assert(validReceiverState(initial));
    for (int offset : {250, -250, 1250, -1250, 14500, -14500, 15000, -15000})
    {
        resetRuntime(); ssb(offset); saveAllReceiverInformation();
        resetRuntime(); setup();
        assert(g_currentFrequency * 1000L + g_currentBFO == 7100000L + offset);
        assert(g_si4735.frequency * 1000L + g_currentBFO == 7100000L + offset);
        assert(g_stepIndex == 8);
        doSync(1);
        assert(g_currentFrequency * 1000L + g_currentBFO == 7100000L + offset);
        assert(g_si4735.frequency * 1000L + g_currentBFO == 7100000L + offset);
    }
    const TestStorage good = EEPROM;
    ReceiverState newer;
    assert(loadReceiverState(EEPROM, newer));
    newer.volume = 63;
    for (int cut = 0; cut < int(sizeof(newer)) + 5; cut++)
    {
        TestStorage interrupted = good;
        interrupted.operations = 0; interrupted.cutAfter = cut;
        try { storeReceiverState(interrupted, newer); } catch (const std::runtime_error&) {}
        ReceiverState loaded;
        if (loadReceiverState(interrupted, loaded))
            assert(loaded.volume == newer.volume || loaded.volume == 45);
    }
    // Every single-bit payload error is rejected before it reaches live arrays.
    for (size_t i = 0; i < sizeof(newer); i++)
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            TestStorage corrupt = good;
            corrupt.bytes[STATE_DATA_ADDRESS + i] ^= 1 << bit;
            ReceiverState loaded;
            assert(!loadReceiverState(corrupt, loaded));
        }
    for (int field = 0; field < 9; field++)
    {
        ReceiverState bad = newer;
        if (field == 0) bad.band = 255;
        if (field == 1) bad.ssbBandwidth = -1;
        if (field == 2) bad.bands[2].frequency = 65535;
        if (field == 3) bad.settings[SettingsIndex::BFO] = 61;
        if (field == 4) bad.bands[2].step = -1;
        if (field == 5) bad.bfo = 15001;
        if (field == 6) { bad.bands[2].frequency = 1710; bad.bfo = -15000; }
        if (field == 7) bad.settings[SettingsIndex::BFO] = -61;
        if (field == 8) bad.bfo = -15001;
        TestStorage invalid; storeReceiverState(invalid, bad);
        ReceiverState loaded;
        assert(!loadReceiverState(invalid, loaded));
    }
    for (uint8_t format : {uint8_t(118), uint8_t(2), uint8_t(3), uint8_t(4), uint8_t(5)})
    {
        if (format == STATE_FORMAT)
            continue;
        TestStorage other = good;
        other.bytes[STATE_FORMAT_ADDRESS] = format;
        ReceiverState loaded; assert(!loadReceiverState(other, loaded));
    }
    EEPROM = good; EEPROM.bytes[STATE_FORMAT_ADDRESS] = 118;
    resetRuntime(); setup(); assert(g_volume == 25 && g_currentMode == AM);
    EEPROM = good; resetRuntime(); pin(ENCODER_BUTTON, false); setup();
    assert(g_volume == 25 && g_currentMode == AM && g_currentBFO == 0);
    std::cout << "PASS EEPROM: round trips, signed BFO, reset, corruption, interrupted writes, schemas\n";
}

void testControls()
{
    resetRuntime(); EEPROM = TestStorage(); setup();
    doVolume(1); assert(g_settingsDirty);
    testMillis += STORE_TIME + 1; loop();
    resetRuntime(); setup(); assert(g_volume == 26);
    testMillis = UINT32_MAX - 100;
    doVolume(1); testMillis += STORE_TIME + 1; loop();
    assert(!g_settingsDirty);
    ssb(250); g_settingsDirty = false;
    bandSwitch(true); assert(g_currentFrequency == 7200);
    assert(g_bandList[2].currentFreq == 7200 && g_currentBFO == 0 && g_settingsDirty);
    saveAllReceiverInformation(); resetRuntime(); setup(); assert(g_currentFrequency == 7200);
    ssb(250); int calls = g_si4735.rdsConfigCalls;
    doBFOCalibration(1); assert(g_si4735.rdsConfigCalls == calls);
    pin(AGC_BUTTON, false); handleButtons();
    testMillis += 16; handleButtons();
    testMillis += 320; handleButtons();
    assert(g_Settings[SettingsIndex::Sync].param == 1 && g_displayOn);
    for (int i = 0; i < 4; i++) { testMillis += 48; handleButtons(); }
    assert(g_Settings[SettingsIndex::Sync].param == 1);
    pin(AGC_BUTTON, true); handleButtons(); testMillis += 80; handleButtons();
    testMillis += 80; handleButtons(); shortPress(AGC_BUTTON); assert(!g_displayOn);
    switchCommand(&g_cmdStep); assert(g_cmdStep);
    pin(STEP_BUTTON, false); handleButtons();
    testMillis += 16; handleButtons(); testMillis += 320; handleButtons();
    pin(STEP_BUTTON, true); handleButtons(); testMillis += 16; handleButtons();
    assert(g_sMeterOn && !g_cmdStep && g_lastAdjustmentTime == 0);
    resetRuntime();
    g_muteVolume = 25; g_si4735.volume = 0;
    volumeEvent(BUTTONEVENT_FIRSTLONGPRESS, AVC_BUTTON);
    assert(g_muteVolume == 0 && g_si4735.volume == 25);
    // A queued step received while volume processing is not cleared by loop().
    resetRuntime(); g_cmdVolume = true; g_encoderCount = 2;
    g_si4735.volumeHook = [] { ++g_encoderCount; };
    loop(); assert(g_encoderCount == 2);
    g_si4735.volumeHook = nullptr;
    assert(takeEncoderStep() == 1 && takeEncoderStep() == 1 && takeEncoderStep() == 0);
    g_encoderCount = -3;
    for (int i = 0; i < 3; i++) assert(takeEncoderStep() == -1);
    assert(takeEncoderStep() == 0);
    // Feed real quadrature transitions through Rotary.cpp and the firmware ISR.
    const uint8_t clockwise[] = {1, 0, 2, 3};
    const uint8_t counterclockwise[] = {2, 0, 1, 3};
    auto turn = [](const uint8_t* phases) {
        for (uint8_t i = 0; i < 4; i++)
        {
            PIND = (PIND & ~12) | (phases[i] << 2);
            rotaryEncoder();
        }
    };
    g_seekStop = false;
    turn(clockwise); turn(clockwise);
    assert(g_encoderCount == 2 && !g_seekStop);
    for (int i = 0; i < 200; i++) turn(clockwise);
    assert(g_encoderCount == 127);
    for (int i = 0; i < 400; i++) turn(counterclockwise);
    assert(g_encoderCount == -127);

    Rotary alternate(4, 5);
    int alternateCw = 0, alternateCcw = 0;
    for (uint8_t phase : clockwise)
    {
        PIND = (PIND & ~0x30) | (phase << 4);
        unsigned char dir = alternate.process();
        if (dir == DIR_CW) ++alternateCw;
        if (dir == DIR_CCW) ++alternateCcw;
    }
    assert(alternateCw == 1 && alternateCcw == 0);
    assert(!validStep(4, SW_BAND_TYPE, USB));
    assert(!validStep(5, MW_BAND_TYPE, AM));
    ssb(0);
    for (int i = 0; i < 24; i++)
    {
        doStep(1); assert(validStep(g_stepIndex, g_bandIndex, g_currentMode));
        assert(getSteps() > 0 && getSteps() <= 10000);
    }
    for (int i = 0; i < 24; i++)
    { doStep(-1); assert(validStep(g_stepIndex, g_bandIndex, g_currentMode)); }
    for (uint8_t band = 0; band < 4; band++)
        for (uint8_t mode = 0; mode < 5; mode++)
            for (int step = -128; step < 128; step++)
                if (validStep(step, band, mode))
                    assert(step >= 0 && step < (band == 3 ? 3 : 12));
    std::cout << "PASS controls: volume/SW persistence, BFO calibration, button gestures, encoder queue\n";
}

void testFrequencyBehavior()
{
    resetRuntime();
    g_currentMode = FM; g_bandIndex = FM_BAND_TYPE;
    g_currentFrequency = 8400; g_bandList[FM_BAND_TYPE].currentFreq = 8400;
    g_si4735.frequency = 8405;
    int tuneCalls = g_si4735.setFrequencyCalls;
    showFrequencySeek(8405);
    assert(g_currentFrequency == 8405 && g_bandList[FM_BAND_TYPE].currentFreq == 8405);
    assert(g_si4735.frequency == 8405 && g_si4735.setFrequencyCalls == tuneCalls);
    g_bandList[FM_BAND_TYPE].currentFreq = 8400;
    applyBandConfiguration();
    assert(g_si4735.lastSeekFmSpacing == FM_SEEK_SPACING && FM_SEEK_SPACING == 10);

    g_bandIndex = SW_BAND_TYPE; g_currentMode = USB; g_ssbLoaded = true;
    g_stepIndex = 11; g_currentBFO = 0; g_currentFrequency = 1710;
    doFrequencyTuneSSB(-1);
    assert(g_currentFrequency == 30000 && g_currentBFO == 0);
    assert(g_bandList[SW_BAND_TYPE].currentFreq == 30000);
    g_currentFrequency = 30000; g_currentBFO = 0;
    doFrequencyTuneSSB(1);
    assert(g_currentFrequency == 1710 && g_currentBFO == 0);

    g_currentFrequency = 3500; g_currentBFO = -500;
    assert(ssbEffectiveFrequencyHz() == 3499500);
    assert(getNextSWSuBband(true) == 3500);
    g_currentBFO = 500;
    assert(getNextSWSuBband(false) == 3500);
    g_currentFrequency = 26200; g_currentBFO = -500;
    showBandTag(); assert(lastRdsLine == "SW");
    g_currentBFO = 0;
    showBandTag(); assert(lastRdsLine == "CB");

    g_currentFrequency = 7100; g_currentBFO = 15000; g_stepIndex = 11;
    doFrequencyTuneSSB(1);
    assert(g_currentFrequency == 7115 && g_currentBFO == 500);
    assert(ssbEffectiveFrequencyHz() == 7115500);
    g_Settings[SettingsIndex::BFO].param = 60;
    g_currentBFO = 15000; updateBFO();
    assert(g_si4735.bfo == -15600);
    g_currentFrequency = 7100; g_currentBFO = -500;
    assert(ssbRoundedFrequencyKHz() == 7100);

    g_currentMode = CW; g_bwIndexSSB = 4;
    updateSSBCutoffFilter();
    assert(g_si4735.lastSidebandCutoff == 0);

    resetRuntime();
    switchCommand(NULL);
    g_bandIndex = SW_BAND_TYPE; g_currentMode = USB; g_ssbLoaded = true;
    g_currentFrequency = 7100; g_currentBFO = -500;
    g_bandList[SW_BAND_TYPE].currentFreq = 7100; g_stepIndex = 8;
    switchCommand(&g_cmdBw);
    assert(g_cmdBw);
    shortPress(MODE_SWITCH);
    assert(g_currentMode == CW && !g_cmdBw && g_lastAdjustmentTime == 0);
    shortPress(MODE_SWITCH);
    assert(g_currentMode == AM && g_currentFrequency == 7100 && g_currentBFO == 0);
    assert(g_bandList[SW_BAND_TYPE].currentFreq == 7100);

    resetRuntime();
    g_currentMode = FM; g_bandIndex = FM_BAND_TYPE; g_currentFrequency = 8400;
    g_bandList[FM_BAND_TYPE].currentFreq = 8400; g_seekDirection = 1;
    g_si4735.frequency = 8400; g_si4735.seekCallbackFrequency = 8405;
    g_si4735.rotateDuringSeek = true;
    doSeek();
    assert(g_si4735.seekStopped && g_seekStop && !g_seekInProgress && g_encoderCount == 0);
    loop();
    assert(g_currentFrequency == 8405 && g_encoderCount == 0);

    std::cout << "PASS frequency: seek stop, FM grid/callback, SSB edges/subbands/BFO, CW and AM transitions\n";
}

void testSeekStartGrid()
{
    resetRuntime();
    g_currentMode = FM; g_bandIndex = FM_BAND_TYPE;
    g_currentFrequency = 8400; g_bandList[FM_BAND_TYPE].currentFreq = 8400;
    g_FMStepIndex = 2; g_bandList[FM_BAND_TYPE].currentStepIdx = 2;
    g_si4735.frequency = 8400; g_si4735.setFrequencyStep(100); // manual 1 MHz
    g_si4735.seekCallbackFrequency = 8450;
    doSeek();
    assert(g_si4735.seekStartFrequency == 8410);
    assert(g_currentFrequency == 8450 && g_bandList[FM_BAND_TYPE].currentFreq == 8450);

    g_seekDirection = 1;
    g_currentFrequency = 10800; g_bandList[FM_BAND_TYPE].currentFreq = 10800;
    g_si4735.frequency = 10800; g_si4735.seekCallbackFrequency = 6450;
    doSeek();
    assert(g_si4735.seekStartFrequency == 6400);

    g_seekDirection = 0;
    g_currentFrequency = 6400; g_bandList[FM_BAND_TYPE].currentFreq = 6400;
    g_si4735.frequency = 6400; g_si4735.seekCallbackFrequency = 10750;
    doSeek();
    assert(g_si4735.seekStartFrequency == 10800);

    resetRuntime();
    g_currentMode = AM; g_bandIndex = SW_BAND_TYPE; g_seekDirection = 1;
    g_currentFrequency = 1710; g_bandList[SW_BAND_TYPE].currentFreq = 1710;
    g_stepIndex = 6; g_bandList[SW_BAND_TYPE].currentStepIdx = 6;
    g_si4735.frequency = 1710; g_si4735.setFrequencyStep(1000); // manual 1 MHz
    g_si4735.seekCallbackFrequency = 1750;
    doSeek();
    assert(g_si4735.seekStartFrequency == 1720);

    resetRuntime();
    g_currentMode = FM; g_bandIndex = FM_BAND_TYPE; g_currentFrequency = 8400;
    g_bandList[FM_BAND_TYPE].currentFreq = 8400; g_si4735.frequency = 8400;
    int calls = g_si4735.seekCalls;
    pin(ENCODER_BUTTON, false);
    doSeek();
    assert(g_si4735.seekCalls == calls && !g_seekInProgress);
    pin(ENCODER_BUTTON, true);

    resetRuntime();
    g_currentMode = FM; g_bandIndex = FM_BAND_TYPE; g_currentFrequency = 8400;
    g_bandList[FM_BAND_TYPE].currentFreq = 8400; g_si4735.frequency = 8400;
    g_Settings[SettingsIndex::ScanSwitch].param = 1;
    pin(ENCODER_BUTTON, false);
    for (int i = 0; i < 25; i++) { testMillis += 16; handleButtons(); }
    assert(g_si4735.seekCalls == 0);
    pin(ENCODER_BUTTON, true);
    testMillis += 16; handleButtons();
    testMillis += 80; handleButtons();
    assert(g_si4735.seekCalls == 0);

    resetRuntime(); ssb(0); g_seekDirection = 0;
    doFrequencyTuneSSB(1);
    assert(g_seekDirection == 1);
    std::cout << "PASS seek grid: seek spacing independent of manual step, wrap, held button, SSB direction\n";
}

void testInvalidFrequencyProtection()
{
    resetRuntime();
    g_currentMode = FM; g_bandIndex = FM_BAND_TYPE;
    g_currentFrequency = 8400; g_bandList[FM_BAND_TYPE].currentFreq = 8400;
    g_settingsDirty = false;
    showFrequencySeek(0);
    assert(g_currentFrequency == 8400);
    assert(g_bandList[FM_BAND_TYPE].currentFreq == 8400);
    assert(!g_settingsDirty);

    resetRuntime();
    g_currentMode = FM; g_bandIndex = FM_BAND_TYPE;
    g_currentFrequency = 8400; g_bandList[FM_BAND_TYPE].currentFreq = 8400;
    g_si4735.frequency = 8400;
    g_si4735.seekCallbackFrequency = 8450;
    g_si4735.failFrequencyRead = true;
    g_si4735.frequencyReadValue = 0;
    doSeek();
    assert(g_currentFrequency == 8450 && g_bandList[FM_BAND_TYPE].currentFreq == 8450);

    resetRuntime();
    g_currentMode = FM; g_bandIndex = FM_BAND_TYPE;
    g_currentFrequency = 8400; g_bandList[FM_BAND_TYPE].currentFreq = 8400;
    g_si4735.frequency = 8400;
    g_si4735.seekCallbackFrequency = 0;
    g_si4735.failFrequencyRead = true;
    g_si4735.frequencyReadValue = 0;
    doSeek();
    assert(g_currentFrequency == 8410 && g_bandList[FM_BAND_TYPE].currentFreq == 8410);

    resetRuntime(); ssb(15000); g_stepIndex = 11;
    g_si4735.frequency = 7100;
    g_si4735.failFrequencyRead = true;
    g_si4735.frequencyReadValue = 0;
    doFrequencyTuneSSB(1);
    assert(g_currentFrequency == 7115 && g_currentBFO == 500);
    assert(g_bandList[SW_BAND_TYPE].currentFreq == 7115);

    resetRuntime(); EEPROM = TestStorage(); setup();
    const TestStorage before = EEPROM;
    g_bandList[FM_BAND_TYPE].currentFreq = 0;
    g_settingsDirty = true;
    saveAllReceiverInformation();
    assert(EEPROM.bytes == before.bytes);
    assert(!g_settingsDirty);
    std::cout << "PASS invalid frequency: reads bounded, seek fallback, SSB fallback, save guard\n";
}

void testDisplayOffPolling()
{
#if USE_RDS
    resetRuntime();
    g_currentMode = FM; g_bandIndex = FM_BAND_TYPE;
    g_currentFrequency = 8500; g_bandList[FM_BAND_TYPE].currentFreq = 8500;
    g_displayRDS = true; g_displayOn = false;
    testMillis = 5000;
    int calls = g_si4735.rdsStatusCalls;
    loop();
    assert(g_si4735.rdsStatusCalls == calls);

    g_displayOn = true;
    loop();
    assert(g_si4735.rdsStatusCalls > calls);
    std::cout << "PASS display off: RDS polling is suspended until the OLED returns\n";
#else
    std::cout << "PASS display off: RDS disabled, no polling path compiled\n";
#endif
}

void testRds()
{
    char line[19]; memset(line, 'X', sizeof(line));
    formatRdsLine(line + 1, nullptr); assert(std::string(line + 1) == "...             ");
    assert(line[0] == 'X' && line[18] == 'X');
    formatRdsLine(line + 1, ""); assert(std::string(line + 1) == "...             ");
    char text[] = "12345678901234567890";
    formatRdsLine(line + 1, text); assert(std::string(line + 1) == "1234567890123456");
    assert(std::string(text) == "12345678901234567890");
    const char raw[] = {'A', 0x01, 'B', 0x7F, 'C', static_cast<char>(0x80), 'D', 0};
    formatRdsLine(line + 1, raw);
    assert(std::string(line + 1) == "A B C D         ");
#if USE_RDS
    resetRuntime(); switchCommand(NULL);
    switchCommand(&g_cmdBand);
    assert(g_cmdBand);
    g_currentMode = FM; doRDS();
    assert(g_displayRDS && !g_cmdBand && g_lastAdjustmentTime == 0);
    g_displayRDS = false;
    g_currentMode = AM; showRDS();
    g_currentMode = FM; g_displayRDS = true; g_currentFrequency = 8400;
    showRDS(); assert(lastRdsLine == "...             ");
    g_si4735.received = true; testMillis += 301; showRDS();
    assert(lastRdsLine == "STATION         ");
    const std::string original = g_si4735.information;
    g_rdsSwitchPressed = true; showRDS();
    assert(lastRdsLine == original.substr(0, 16));
    assert(original == g_si4735.information);
    g_si4735.received = false; g_currentFrequency = 8500; showRDS();
    assert(lastRdsLine == "...             ");
#endif
    std::cout << "PASS RDS: absent/empty/long text, bounds, printable filter, borrowed buffers, frequency change\n";
}

void testProductionFormatting()
{
    char field[10];
    memset(field, 'X', sizeof(field));
    convertToChar(field + 1, 8405, 5, 3, '.', '/');
    assert(std::string(field + 1) == "/84.05" && field[0] == 'X' && field[8] == 'X');
    convertToChar(field, 1476, 5);
    assert(std::string(field) == " 1476");
    convertToChar(field, 30000, 5, 2, '.', '/');
    assert(std::string(field) == "30.000");
    convertToChar(field, 0, 4);
    assert(std::string(field) == "   0");
    convertToChar(field, 65535, 5);
    assert(std::string(field) == "65535");
    assert(ilen(0) == 1 && ilen(9) == 1 && ilen(10) == 2 && ilen(99) == 2);
    assert(ilen(100) == 3 && ilen(999) == 3 && ilen(1000) == 4 && ilen(65535) == 5);
    resetRuntime();
    for (uint16_t base : {153, 520, 1710, 3500, 7100, 26200, 28000, 30000})
    {
        g_currentFrequency = base;
        for (int offset = -15000; offset <= 15000; offset++)
        {
            g_currentBFO = offset;
            int32_t hz = int32_t(base) * 1000 + offset;
            assert(ssbEffectiveFrequencyHz() == hz);
            assert(ssbFrequencyKHzFloor() == hz / 1000);
            assert(ssbFrequencyKHzCeil() == (hz + 999) / 1000);
            assert(ssbRoundedFrequencyKHz() == (hz + 500) / 1000);
            uint16_t khz, tail;
            splitFreq(khz, tail);
            assert(khz == hz / 1000 && tail == (hz % 1000) / 10);
        }
    }
    std::cout << "PASS production Utils: format bounds, decimal/zero padding and 240008 SSB pairs\n";
}

void testContinuousSsbTuning()
{
    for (int direction : {-1, 1})
    {
        resetRuntime(); ssb(direction * 15000); g_stepIndex = 11;
        g_si4735.frequency = 7100;
        int calls = g_si4735.setFrequencyCalls;
        int32_t start = ssbEffectiveFrequencyHz();
        for (int i = 0; i < 20; i++)
        {
            doFrequencyTuneSSB(direction);
            assert(ssbEffectiveFrequencyHz() == start + direction * (i + 1) * 500);
        }
        assert(g_si4735.setFrequencyCalls - calls == 1);
        assert(abs(g_currentBFO) <= STATE_MAX_BFO_HZ);
    }

    // These valid starting frequencies cross the effective band edge on a 10 kHz step.
    for (int direction : {-1, 1})
    {
        resetRuntime(); ssb(direction * 9000); g_stepIndex = 3;
        g_currentFrequency = direction > 0 ? 29990 : 1720;
        g_bandList[SW_BAND_TYPE].currentFreq = g_currentFrequency;
        g_si4735.frequency = g_currentFrequency;
        doFrequencyTuneSSB(direction);
        uint16_t wrapped = direction > 0 ? 1710 : 30000;
        assert(g_currentFrequency == wrapped && g_currentBFO == 0);
        assert(g_si4735.frequencyRequests.size() == 1 && g_si4735.frequencyRequests[0] == wrapped);
        assert(g_si4735.bfoRequests.size() == 1 && g_si4735.bfoRequests[0] == 0);
    }

    for (int offset : {-15000, -14999, -500, 0, 500, 14999, 15000})
        for (int calibration = -60; calibration <= 60; calibration++)
        {
            g_currentBFO = offset;
            g_Settings[SettingsIndex::BFO].param = calibration;
            updateBFO();
            assert(g_si4735.bfo == -(offset + calibration * 10));
            assert(abs(g_si4735.bfo) <= 16383);
        }
    std::cout << "PASS SSB rollover: one carrier retune, both directions, wrap before chip commands, calibration bounds\n";
}

void testCommandTransitions()
{
    resetRuntime();
    int calls = displayPrintCalls;
    switchCommand(NULL);
    assert(displayPrintCalls == calls && g_lastAdjustmentTime == 0);
    testMillis = 1000;
    switchCommand(&g_cmdStep);
    assert(g_cmdStep && g_lastAdjustmentTime == 1000 && displayPrintCalls > calls);
    switchCommand(&g_cmdBand);
    assert(g_cmdBand && !g_cmdStep);
    switchCommand(NULL);
    calls = displayPrintCalls;
    switchCommand(NULL);
    assert(displayPrintCalls == calls && !g_cmdBand && g_lastAdjustmentTime == 0);

    // The actual showStep routine must draw after the lower line is cleared.
    g_sMeterOn = true;
    displayTexts.clear();
    shortPress(STEP_BUTTON);
    assert(g_cmdStep && !g_sMeterOn);
    assert(std::find(displayTexts.begin(), displayTexts.end(), "St:") != displayTexts.end());
    std::cout << "PASS command state: idle close has no redraw, exclusive modes, visible step after S-meter\n";
}

void testSeekProtocol()
{
    resetRuntime(); g_currentMode = AM; g_bandIndex = SW_BAND_TYPE;
    for (int8_t step = 0; step < 7; step++)
    {
        g_stepIndex = step; g_bandList[SW_BAND_TYPE].currentStepIdx = step;
        applyBandConfiguration();
        int spacing = g_si4735.lastSeekAmSpacing;
        assert(spacing == 1 || spacing == 5 || spacing == 9 || spacing == 10);
        doStep(1);
        spacing = g_si4735.lastSeekAmSpacing;
        assert(spacing == 1 || spacing == 5 || spacing == 9 || spacing == 10);
    }
    resetRuntime(); g_currentMode = FM; g_bandIndex = FM_BAND_TYPE;
    g_si4735.frequency = 8400; g_currentFrequency = 8400;
    g_si4735.seekCallbackFrequency = 8405;
    g_si4735.finalSeekFrequency = 8410;
    g_si4735.rotateDuringSeek = true;
    doSeek();
    assert(g_seekStop && !g_seekInProgress && !g_si4735.hardwareSeeking);
    assert(g_si4735.cancelCommands == 1 && g_encoderCount == 0);
    assert(g_currentFrequency == 8410 && g_bandList[FM_BAND_TYPE].currentFreq == 8410);
    loop(); assert(g_currentFrequency == 8410);
    std::cout << "PASS seek protocol: legal AM spacing, cancel and final-frequency synchronization\n";
}

int main()
{
    testBatteryInitialSample(); testShortReleaseDebounce();
    testResetConfirmation(); testMenuWear(); testStorage(); testControls();
    testFrequencyBehavior(); testSeekStartGrid();
    testInvalidFrequencyProtection(); testDisplayOffPolling(); testRds();
    testProductionFormatting(); testContinuousSsbTuning(); testCommandTransitions(); testSeekProtocol();
    std::cout << "PASS all host regressions (USE_RDS=" << USE_RDS << ")\n";
}
