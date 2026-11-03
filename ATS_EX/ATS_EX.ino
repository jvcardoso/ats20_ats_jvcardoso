// ----------------------------------------------------------------------
// ATS_EX (Extended) Firmware for ATS-20 and ATS-20+ receivers.
// Based on PU2CLR sources.
// Inspired by closed-source swling.ru firmware.
// For more information check README file in my github repository:
// http://github.com/goshante/ats20_ats_ex
// ----------------------------------------------------------------------
// By Goshante
// 02.2024
// http://github.com/goshante
// ----------------------------------------------------------------------

#include <SI4735.h>
#include <EEPROM.h>
#include <util/atomic.h>
#include <Tiny4kOLED.h>
#include <PixelOperatorBold.h> 

#include "font14x24sevenSeg.h"
#include "Rotary.h"
#include "SimpleButton.h"
#include "patch_ssb_compressed.h"

#include "defs.h"
#include "globals.h"
#include "Utils.h"
#include "ReceiverState.h"
#include "RdsText.h"

static_assert(STATE_SETTINGS_COUNT == SettingsIndex::SETTINGS_MAX, "Settings layout mismatch");

void showStatus(bool cleanFreq = false);
void applyBandConfiguration(bool extraSSBReset = false);

bool isSSB()
{
    return g_currentMode > AM && g_currentMode < FM;
}

bool encoderButtonPressed()
{
    return !(PINC & (1 << (ENCODER_BUTTON - 14)));
}

int getSteps()
{
    if (isSSB())
    {
        if (g_stepIndex >= g_amTotalSteps)
            return flashRead16(&g_tabStep[g_stepIndex]);

        return flashRead16(&g_tabStep[g_stepIndex]) * 1000;
    }

    if (g_stepIndex >= g_amTotalSteps)
        g_stepIndex = 0;

    return flashRead16(&g_tabStep[g_stepIndex]);
}

int getLastStep()
{
    if (isSSB())
        return g_amTotalSteps + g_ssbTotalSteps - 1;

    return g_amTotalSteps - 1;
}

int16_t ssbFrequencyKHzFloor()
{
    uint16_t shifted = uint16_t(g_currentBFO + STATE_MAX_BFO_HZ);
    return int16_t(g_currentFrequency) + shifted / 1000 - STATE_MAX_BFO_KHZ;
}

int16_t ssbFrequencyKHzCeil()
{
    uint16_t shifted = uint16_t(g_currentBFO + STATE_MAX_BFO_HZ + 999);
    return int16_t(g_currentFrequency) + shifted / 1000 - STATE_MAX_BFO_KHZ;
}

uint16_t ssbRoundedFrequencyKHz()
{
    uint16_t shifted = uint16_t(g_currentBFO + STATE_MAX_BFO_HZ + 500);
    return g_currentFrequency + shifted / 1000 - STATE_MAX_BFO_KHZ;
}

uint16_t amSeekSpacing()
{
    uint16_t step = g_stepIndex >= g_amTotalSteps ? 1 : flashRead16(&g_tabStep[g_stepIndex]);
    return step > 10 ? 10 : step;
}

bool frequencyInBand(uint16_t frequency)
{
    return frequency >= g_bandList[g_bandIndex].minimumFreq
        && frequency <= g_bandList[g_bandIndex].maximumFreq;
}

bool bandFrequenciesValid()
{
    for (uint8_t i = 0; i <= g_lastBand; i++)
        if (g_bandList[i].currentFreq < g_bandList[i].minimumFreq
            || g_bandList[i].currentFreq > g_bandList[i].maximumFreq)
            return false;
    return true;
}

// --------------------------
// ------- Main logic -------
// --------------------------

//Initialize controller
void setup()
{
    //We need to save more space with this
    DDRB |=  (1 << DDB5);   //13 pin
    DDRD &= ~(1 << ENCODER_PIN_A);
    PORTD |= (1 << ENCODER_PIN_A);
    DDRD &= ~(1 << ENCODER_PIN_B);
    PORTD |= (1 << ENCODER_PIN_B);
    g_voltagePinConnnected = analogRead(BATTERY_VOLTAGE_PIN) > 300;

    oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);
    oled.clear();
    oled.on();
    oled.setFont(DEFAULT_FONT);

    bool resetSettings = encoderButtonPressed();

    //Don't use digitalRead()
    //Registers save us more space
    if (resetSettings)
    {
        oled.print("  EEPROM RESET");
        oled.setCursor(0, 2);
        for (uint8_t i = 0; i < 16; i++)
        {
            if (!encoderButtonPressed())
            {
                resetSettings = false;
                oled.clear();
                break;
            }
            oled.print("-"); //Just fancy animation
            delay(60);
        }
    }
    if (!resetSettings)
    {
        oledPrint(" ATS-20 RECEIVER", 0, 0, DEFAULT_FONT, true);
        oledPrint("ATS_EX v1.18-r3", 4, 2);
        oledPrint("JVCARDOSO 2026", 8, 4);
        oledPrint("Custom firmware", 4, 6);
        delay(2000);
    }
    oled.clear();

    //Encoder interrupts
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

    g_si4735.getDeviceI2CAddress(RESET_PIN);
    g_si4735.setup(RESET_PIN, MW_BAND_TYPE);

    delay(500);

    bool restored = !resetSettings && readAllReceiverInformation();
    if (restored && isSSB())
        loadSSBPatch();

    // Keep the AVR clock at its nominal rate so millis() and delay() stay accurate.

    applyBandConfiguration();
    g_si4735.setVolume(g_volume);
    oled.setContrast(uint8_t(g_Settings[SettingsIndex::Brightness].param) * 2);
    if (!restored)
        saveAllReceiverInformation();
    g_settingsDirty = false;

    //Draw main screen
    oled.clear();
    showStatus();
}

uint8_t volumeEvent(uint8_t event, uint8_t pin)
{
    if (g_muteVolume)
    {
        if (!BUTTONEVENT_ISDONE(event))
        {
            if ((BUTTONEVENT_SHORTPRESS != event) || (VOLUME_BUTTON == pin))
                doVolume(1);
        }
        return event;
    }

    if (!g_muteVolume)
    {
#if (0 != VOLUME_DELAY)
#if (VOLUME_DELAY > 1)
        static uint8_t count;
        if (BUTTONEVENT_FIRSTLONGPRESS == event)
        {
            count = 0;
        }
#endif
        if (BUTTONEVENT_ISLONGPRESS(event))
            if (BUTTONEVENT_LONGPRESSDONE != event)
            {
#if (VOLUME_DELAY > 1)
                if (count++ == 0)
#endif
                    doVolume(VOLUME_BUTTON == pin ? 1 : -1);
#if (VOLUME_DELAY > 1)
                count = count % VOLUME_DELAY;
#endif
            }
#else
        if (BUTTONEVENT_FIRSTLONGPRESS == event)
            event = BUTTONEVENT_SHORTPRESS;
#endif
    }
    return event;
}

uint8_t simpleEvent(uint8_t event, uint8_t)
{
    if (BUTTONEVENT_FIRSTLONGPRESS == event)
        event = BUTTONEVENT_SHORTPRESS;
    return event;
}

uint8_t bandEvent(uint8_t event, uint8_t pin)
{
#if (0 != BAND_DELAY)
    static uint8_t count;
    if (BUTTONEVENT_ISLONGPRESS(event) && !g_settingsActive)
    {
        if (BUTTONEVENT_LONGPRESSDONE != event)
        {
            if (BUTTONEVENT_FIRSTLONGPRESS == event)
            {
                count = 0;
            }
            if (count++ == 0)
            {
                if (BAND_BUTTON == pin)
                {
                    if (g_bandIndex < g_lastBand)
                        bandSwitch(true);
                }
                else
                {
                    if (g_bandIndex)
                        bandSwitch(false);
                }
            }
            count = count % BAND_DELAY;
        }
    }
#else
    if (BUTTONEVENT_FIRSTLONGPRESS == event)
        event = BUTTONEVENT_SHORTPRESS;
#endif
    return event;
}

// Handle encoder direction
void rotaryEncoder()
{
    uint8_t encoderStatus = g_encoder.process();
    if (encoderStatus)
    {
        int8_t direction = encoderStatus == DIR_CW ? 1 : -1;
        if (g_seekInProgress)
            g_seekStop = true;
        else if ((direction > 0 && g_encoderCount < 127) || (direction < 0 && g_encoderCount > -127))
            g_encoderCount += direction;
    }
}

//Saves more flash image size
void updateSSBCutoffFilter()
{
    // Auto mode: If SSB bandwidth 2 KHz or lower - it's better to enable cutoff filter
    if (g_currentMode == CW)
        g_si4735.setSSBSidebandCutoffFilter(0);
    else if (g_Settings[SettingsIndex::CutoffFilter].param == 0)
        g_si4735.setSSBSidebandCutoffFilter((g_bandwidthSSB[g_bwIndexSSB].idx == 0 || g_bandwidthSSB[g_bwIndexSSB].idx == 4 || g_bandwidthSSB[g_bwIndexSSB].idx == 5) ? 0 : 1);
    else
        g_si4735.setSSBSidebandCutoffFilter(g_Settings[SettingsIndex::CutoffFilter].param - 1);
}

// Copy only validated snapshots into the live receiver state.
void saveAllReceiverInformation()
{
    ReceiverState state;
    state.volume = g_muteVolume ? g_muteVolume : g_si4735.getVolume();
    state.band = g_bandIndex;
    state.mode = g_currentMode;
    state.ssbBandwidth = g_bwIndexSSB;
    state.bfo = isSSB() ? g_currentBFO : 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        state.bands[i].frequency = g_bandList[i].currentFreq;
        state.bands[i].step = g_bandList[i].currentStepIdx;
        state.bands[i].bandwidth = g_bandList[i].bandwidthIdx;
    }
    for (uint8_t i = 0; i < SettingsIndex::SETTINGS_MAX; i++)
        state.settings[i] = g_Settings[i].param;
    if (!bandFrequenciesValid())
    {
        g_settingsDirty = false;
        return;
    }
    storeReceiverState(EEPROM, state);
    g_settingsDirty = false;
}

bool readAllReceiverInformation()
{
    ReceiverState state;
    if (!loadReceiverState(EEPROM, state))
        return false;
    g_volume = state.volume;
    g_bandIndex = state.band;
    g_currentMode = state.mode;
    g_bwIndexSSB = state.ssbBandwidth;
    g_currentBFO = state.bfo;
    for (uint8_t i = 0; i < 4; i++)
    {
        g_bandList[i].currentFreq = state.bands[i].frequency;
        g_bandList[i].currentStepIdx = state.bands[i].step;
        g_bandList[i].bandwidthIdx = state.bands[i].bandwidth;
    }
    for (uint8_t i = 0; i < SettingsIndex::SETTINGS_MAX; i++)
        g_Settings[i].param = state.settings[i];
    return true;
}

void resetEepromDelay()
{
    g_storeTime = millis();
    g_settingsDirty = true;
}

// Consume one signed step, keeping any events arriving during processing.
int8_t takeEncoderStep()
{
    int8_t step = 0;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (g_encoderCount)
        {
            step = g_encoderCount > 0 ? 1 : -1;
            g_encoderCount -= step;
        }
    }
    return step;
}

//Draw frequency. 
//BFO and main frequency produce actual frequency that is displayed on LCD
//Too sensitive logic, do not change
void showFrequency(bool cleanDisplay = false)
{
    if (g_settingsActive)
        return;

    char unit[4];
    char freqDisplay[7];
    char ssbSuffix[4];
    static uint8_t prevLen = 0;
    uint16_t khzBFO = 0, tailBFO = 0;
    uint8_t off = (isSSB() ? -5 : 4) + 8;

    unit[0] = 'K';
    unit[1] = 'H';
    unit[2] = 'z';
    unit[3] = 0x0;

    ssbSuffix[0] = '.';
    ssbSuffix[1] = '0';
    ssbSuffix[2] = '0';
    ssbSuffix[3] = '\0';

    if (g_bandIndex == FM_BAND_TYPE)
    {
        convertToChar(freqDisplay, g_currentFrequency, 5, 3, '.', '/');
        unit[0] = 'M';
    }
    else
    {
        if (g_bandIndex == SW_BAND_TYPE)
            showBandTag();

        if (!isSSB())
        {
            bool swMhz = g_Settings[SettingsIndex::SWUnits].param == 1;
            convertToChar(freqDisplay, g_currentFrequency, 5, (g_bandIndex == SW_BAND_TYPE && swMhz) ? 2 : 0, '.', '/');
            if (g_bandIndex == SW_BAND_TYPE && swMhz)
                unit[0] = 'M';
        }
        else
        {
            splitFreq(khzBFO, tailBFO);
            //utoa(freqDisplay, khzBFO);
            convertToChar(freqDisplay, khzBFO, ilen(khzBFO));
        }
    }

    uint8_t len = isSSB() ? ilen(khzBFO) : ilen(g_currentFrequency);
    if (cleanDisplay)
    {
        oledPrint("/////////", 0, 3, FONT14X24SEVENSEG); // This character is an empty space in my seven seg font.
    }
    else if (isSSB() && len > prevLen && len == 5)
        oledPrint("   ", 102, 4, DEFAULT_FONT);

    oledPrint(freqDisplay, off, 3, FONT14X24SEVENSEG);

    if (isSSB())
    {
        //utoa((ilen(tailBFO) == 1) ? &ssbSuffix[2] : &ssbSuffix[1], tailBFO);
        convertToChar((ilen(tailBFO) == 1) ? &ssbSuffix[2] : &ssbSuffix[1], tailBFO, ilen(tailBFO));
        ssbSuffix[3] = 0;
        oledPrint(ssbSuffix);
        if (len != prevLen && len < prevLen)
            oledPrint("/");
    }

    if (g_Settings[SettingsIndex::UnitsShow].param == 1 && (!isSSB() || len < 5))
        oledPrint(unit, 102, 4, DEFAULT_FONT);
        
    prevLen = len;
}

//This function is called by station seek logic
void showFrequencySeek(uint16_t freq)
{
    if (!frequencyInBand(freq))
        return;
    g_currentFrequency = freq;
    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    resetEepromDelay();
    showFrequency();
}

bool checkStopSeeking()
{
    return g_seekStop || encoderButtonPressed();
}

void doSeek()
{
    if (encoderButtonPressed())
        return;

    uint16_t low = g_bandList[g_bandIndex].minimumFreq;
    uint16_t high = g_bandList[g_bandIndex].maximumFreq;
    uint16_t start = g_currentFrequency;
    if (start < low)
        start = low;
    else if (start > high)
        start = high;

    // Advance by the seek grid, never by the manual tuning step.
    uint16_t spacing = g_currentMode == FM ? FM_SEEK_SPACING : amSeekSpacing();
    if (g_seekDirection)
        start = start > high - spacing ? low : start + spacing;
    else
        start = start < low + spacing ? high : start - spacing;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        g_seekStop = false;
        g_seekInProgress = true;
        g_encoderCount = 0;
    }
    g_processFreqChange = false;

    g_si4735.setFrequency(start);
    g_currentFrequency = start;
    g_bandList[g_bandIndex].currentFreq = start;
    g_si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, g_seekDirection);
    // PU2CLR 2.1.8 getFrequency() sends TUNE_STATUS with CANCEL=1.
    showFrequencySeek(g_si4735.getFrequency());
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        g_seekInProgress = false;
    }
}

//Update and draw main screen UI. 
//basicUpdate - update minimum as possible
//cleanFreq   - force clean frequency line
void showStatus(bool cleanFreq)
{
    showFrequency(cleanFreq);
    showModulation();
    showStep();
    showBandwidth();
    showCharge(true);
    showVolume();
}

void updateLowerDisplayLine()
{
    oledPrint(_literal_EmptyLine, 0, 6, DEFAULT_FONT);
    showModulation();
    showStep();
    showCharge(true);
}

//Converts settings value to UI value
void SettingParamToUI(char* buf, uint8_t idx)
{
    int8_t param = g_Settings[idx].param;
    switch (g_Settings[idx].type)
    {
    case SettingType::ZeroAuto:
        if (param == 0)
        {
            buf[0] = 'A';
            buf[1] = 'U';
            buf[2] = 'T';
            buf[3] = 0x0;
        }
        else
            convertToChar(buf, param, 3);

        break;

    case SettingType::Num:
        convertToChar(buf, abs(param), 3);
        if (param < 0)
            buf[0] = '-';
        break;

    case SettingType::SwitchAuto:
        if (param == 0)
        {
            buf[0] = 'A';
            buf[1] = 'U';
            buf[2] = 'T';       }
        else if (param == 1)
        {
            buf[0] = 'O';
            buf[1] = 'n';
            buf[2] = ' ';
        }
        else
        {
            buf[0] = 'O';
            buf[1] = 'f';
            buf[2] = 'f';
        }
        buf[3] = 0x0;
        break;

    case SettingType::Switch:
        if (idx == SettingsIndex::DeEmp)
        {
            if (param == 0)
            {
                buf[0] = '5';
                buf[1] = '0';
                buf[2] = 'u';
            }
            else
            {
                buf[0] = '7';
                buf[1] = '5';
                buf[2] = 'u';
            }
        }
        else if (idx == SettingsIndex::SWUnits)
        {
            if (param == 0)
                buf[0] = 'K';
            else
                buf[0] = 'M';
            buf[1] = 'H';
            buf[2] = 'z';
        }
        else if (idx == SettingsIndex::SSM)
        {
            if (param == 0)
            {
                buf[0] = 'R';
                buf[1] = 'S';
                buf[2] = 'S';
            }
            else
            {
                buf[0] = 'S';
                buf[1] = 'N';
                buf[2] = 'R';
            }
        }
        else if (idx == SettingsIndex::CWSwitch)
        {
            if (param == 0)
                buf[0] = 'L';
            else
                buf[0] = 'U';

            buf[1] = 'S';
            buf[2] = 'B';
        }
        else
        {
            if (param == 0)
            {
                buf[0] = 'O';
                buf[1] = 'f';
                buf[2] = 'f';
            }
            else
            {
                buf[0] = 'O';
                buf[1] = 'n';
                buf[2] = ' ';
            }
        }
        buf[3] = 0x0;
        break;
    }
}

// If full false - update only value
void DrawSetting(uint8_t idx, bool full)
{
    if (!g_settingsActive)
        return;

    char buf[5];
    uint8_t place = idx - ((g_SettingsPage - 1) * 6);
    uint8_t yOffset = place > 2 ? (place - 3) * 2 : place * 2;
    uint8_t xOffset = place > 2 ? 60 : 0;
    if (full)
        oledPrint(g_Settings[idx].name, 5 + xOffset, 2 + yOffset, DEFAULT_FONT, idx == g_SettingSelected && !g_SettingEditing);
    SettingParamToUI(buf, idx);
    oledPrint(buf, 35 + xOffset, 2 + yOffset, DEFAULT_FONT, idx == g_SettingSelected && g_SettingEditing);
}

//Update and draw settings UI
void showSettings()
{
    for (uint8_t i = 0; i < 6 && i + ((g_SettingsPage - 1) * 6) < SettingsIndex::SETTINGS_MAX; i++)
        DrawSetting(i + ((g_SettingsPage - 1) * 6), true);
}

void showSettingsTitle()
{
    oledPrint("   SETTINGS  ", 0, 0, DEFAULT_FONT, true);
    oled.invertOutput(true);
    oled.write('0' + g_SettingsPage);
    oled.print("/");
    oled.write('0' + g_SettingsMaxPages);
    oled.invertOutput(false);
}

void switchSettingsPage()
{
    g_SettingsPage++;
    g_SettingsPage = (g_SettingsPage > g_SettingsMaxPages) ? 1 : g_SettingsPage;
    g_SettingSelected = 6 * (g_SettingsPage - 1);
    g_SettingEditing = false;
    oled.clear();
    showSettingsTitle();
    showSettings();
}

//Switch between main screen and settings mode
void switchSettings()
{
    oled.clear();
    if (g_settingsActive)
    {
        g_SettingsPage = 1;
        showSettingsTitle();
        g_SettingSelected = 0;
        g_SettingEditing = false;
        showSettings();
    }
    else
    {
        if (g_settingsDirty)
            saveAllReceiverInformation();
        showStatus();
    }
}

//Draw curremt modulation
void showModulation()
{
    oledPrint(g_bandModeDesc[g_currentMode], 0, 0, DEFAULT_FONT, g_cmdBand && g_currentMode == FM);
    oled.print(" ");
    if (isSSB() && g_Settings[SettingsIndex::Sync].param == 1)
        oledPrint("S", -1, -1, LastFont, true);
    else
        oled.print(" ");

    showBandTag();
}

//Draw current band
void showBandTag()
{
    if (g_sMeterOn || g_displayRDS)
        return;

    int16_t effectiveFrequencyKHz = isSSB()
        ? ssbFrequencyKHzFloor() : int16_t(g_currentFrequency);
    bool isCB = effectiveFrequencyKHz >= CB_LIMIT_LOW
        && effectiveFrequencyKHz < CB_LIMIT_HIGH;
    oledPrint(isCB ? "CB" : bandTags[g_bandIndex], 0, 6, DEFAULT_FONT, g_cmdBand && g_currentMode != FM);
}

//Draw volume level
void showVolume()
{
    if (g_settingsActive)
        return;

    char buf[3];
    if (g_muteVolume == 0)
        convertToChar(buf, g_si4735.getCurrentVolume(), 2, 0, 0);
    else
    {
        buf[0] = ' ';
        buf[1] = 'M';
        buf[2] = 0;
    }

    oledPrint(buf, (128 - (8 * 2) + 2 - 6), 0, DEFAULT_FONT, g_cmdVolume);
}

//Draw battery charge
//This feature requires hardware mod
//Voltage divider made of two 10 KOhm resistors between + and GND of Li-Ion battery
//Solder it to A2 analog pin
void showCharge(bool forceShow)
{
    if (!g_voltagePinConnnected)
        return;

    // mV, Percent
    //This values represent voltage values in ATMega328p analog units with reference voltage 3.30v
    //Voltage pin reads voltage from voltage divider, so it have to be 1/2 of Li-Ion battery voltage
    constexpr const uint8_t rows = 10;
    static const uint16_t dischargeTable[rows][2] RADIO_PROGMEM =
    {
        { 643, 100 },  //4.15v
        { 620, 95  },  //4.05v
        { 604, 90  },  //3.90v
        { 581, 80  },  //3.75v
        { 573, 60  },  //3.70v
        { 558, 40  },  //3.60v
        { 542, 20  },  //3.50v
        { 503, 15  },  //3.25v
        { 496, 5  },   //3.20v
        { 488, 0  },   //3.15v
    };

    auto getBatteryPercentage = [&](uint16_t currentSamples) -> uint8_t
    {
        if (currentSamples >= flashRead16(&dischargeTable[0][0]))
            return 100;

        if (currentSamples <= flashRead16(&dischargeTable[rows - 1][0]))
            return 0;

        for (uint8_t i = 0; i < rows - 1; ++i) 
        {
            if (currentSamples >= flashRead16(&dischargeTable[i + 1][0]) && currentSamples <= flashRead16(&dischargeTable[i][0]))
            {
                uint16_t voltageDiff = flashRead16(&dischargeTable[i][0]) - flashRead16(&dischargeTable[i + 1][0]);
                uint16_t percentageDiff = flashRead16(&dischargeTable[i][1]) - flashRead16(&dischargeTable[i + 1][1]);
                uint16_t voltageOffset = currentSamples - flashRead16(&dischargeTable[i + 1][0]);
                return flashRead16(&dischargeTable[i + 1][1]) + (percentageDiff * voltageOffset + voltageDiff / 2) / voltageDiff;
            }
        }

        return 0;
    };

    static uint32_t lastChargeShow = 0;
    static int16_t averageSamples = -1; // Outside the ADC range, including a valid zero sample.

    int sample = analogRead(BATTERY_VOLTAGE_PIN);
    if (averageSamples < 0)
    {
        averageSamples = sample;
    }

    if ((millis() - lastChargeShow) > 10000 || forceShow)
    {
        char buf[4];
        buf[3] = 0;
        int16_t percents = getBatteryPercentage(averageSamples);

        uint8_t il = ilen(percents) < 3 ? 2 : 3;
        convertToChar(buf, percents, il);

        if (il < 3)
            buf[2] = '%';

        if (!g_settingsActive && !g_sMeterOn && !g_displayRDS)
            oledPrint(buf, 102, 6, DEFAULT_FONT);
        lastChargeShow = millis();
        averageSamples = sample;
    }

    averageSamples = (averageSamples + sample) / 2;
}

#if USE_RDS
void showRDS()
{
    static uint16_t lastFrequency = 0;
    static uint32_t lastPoll = 0;
    if (g_currentMode != FM || !g_displayRDS || g_settingsActive)
    {
        lastFrequency = 0;
        g_rdsSwitchPressed = false;
        return;
    }

    bool changed = lastFrequency != g_currentFrequency;
    if (changed)
    {
        memset(g_RDSCells, 0, sizeof(g_RDSCells));
        g_rdsActiveInfo = RDSActiveInfo::StationName;
        lastFrequency = g_currentFrequency;
    }
    if (!changed && !g_rdsSwitchPressed && millis() - lastPoll < 300)
        return;
    if (g_rdsSwitchPressed)
        g_rdsActiveInfo = (g_rdsActiveInfo + 1) % 3;
    g_rdsSwitchPressed = false;
    lastPoll = millis();

    g_si4735.getRdsStatus();
    if (g_si4735.getRdsReceived() && g_si4735.getRdsSync() && g_si4735.getNumRdsFifoUsed() > 1)
    {
        const char* cells[] = {g_si4735.getRdsStationName(),
            g_si4735.getRdsStationInformation(), g_si4735.getRdsProgramInformation()};
        for (uint8_t i = 0; i < 3; i++)
            if (cells[i])
                g_RDSCells[i] = cells[i];
    }
    char line[17];
    formatRdsLine(line, g_RDSCells[g_rdsActiveInfo]);
    oledPrint(line, 0, 6, DEFAULT_FONT);
}
#endif

//Draw steps (with units)
void showStep()
{
    if (g_sMeterOn || g_displayRDS)
        return;

    char buf[5];
    if (g_currentMode == FM)
    {
        if (flashRead8(&g_tabStepFM[g_FMStepIndex]) == 100)
        {
            buf[0] = ' ';
            buf[1] = ' ';
            buf[2] = '1';
            buf[3] = 'M';
            buf[4] = 0x0;
        }
        else
        {
            convertToChar(buf, flashRead8(&g_tabStepFM[g_FMStepIndex]) * 10, 3);
            buf[3] = 'k';
            buf[4] = '\0';
        }
    }
    else
    {
        if (flashRead16(&g_tabStep[g_stepIndex]) == 1000)
        {
            buf[0] = ' ';
            buf[1] = ' ';
            buf[2] = '1';
            buf[3] = 'M';
            buf[4] = 0x0;
        }
        else if (isSSB() && g_stepIndex >= g_amTotalSteps)
            convertToChar(buf, flashRead16(&g_tabStep[g_stepIndex]), 4);
        else
        {
            convertToChar(buf, flashRead16(&g_tabStep[g_stepIndex]), 3);
            buf[3] = 'k';
            buf[4] = '\0';
        }
    }

    uint8_t off = 50;
    oledPrint("St:", off - 16, 6, DEFAULT_FONT, g_cmdStep);
    oledPrint(buf, off + 8, 6, DEFAULT_FONT, g_cmdStep);
}

void showSMeter()
{
    static uint32_t sMeterUpdated = 0;
    if (millis() - sMeterUpdated < 100)
        return;

    g_si4735.getCurrentReceivedSignalQuality();
    uint8_t rssi = g_si4735.getCurrentRSSI();
    rssi = rssi > 64 ? 64 : rssi;

    int sMeterValue = rssi / (64 / 16);
    char buf[17];
    for (uint8_t i = 0; i < sizeof(buf) - 1; i++)
        buf[i] = i < sMeterValue ? '|' : ' ';
    buf[sizeof(buf) - 1] = 0x0;

    oledPrint(buf, 0, 6, DEFAULT_FONT);
    sMeterUpdated = millis();
}

//Draw bandwidth (Ignored for CW mode)
void showBandwidth()
{
    const char* bw;
    if (isSSB())
    {
        bw = g_bandwidthSSB[g_bwIndexSSB].desc;
        if (g_currentMode == CW)
            bw = "    ";
    }
    else if (g_currentMode == AM)
    {
        bw = g_bandwidthAM[g_bwIndexAM].desc;
    }
    else
    {
        bw = g_bandwidthFM[g_bwIndexFM];
    }

    oledPrint(bw, 45, 0, DEFAULT_FONT, g_cmdBw);
}

uint16_t getNextSWSuBband(bool up)
{
    int16_t frequencyKHz = isSSB()
        ? (up ? ssbFrequencyKHzFloor() : ssbFrequencyKHzCeil())
        : int16_t(g_currentFrequency);

    for (uint8_t i = 0; i < g_SWSubBandCount; i++)
    {
        uint8_t n = g_SWSubBandCount - 1 - i;
        if (!up && int16_t(flashRead16(&SWSubBands[n])) < frequencyKHz)
            return flashRead16(&SWSubBands[n]);
        else if (up && int16_t(flashRead16(&SWSubBands[i])) > frequencyKHz)
            return flashRead16(&SWSubBands[i]);
    }

    return 0;
}

void bandSwitch(bool up)
{
    uint16_t nextSW = getNextSWSuBband(up);
    
    if (g_bandIndex == SW_BAND_TYPE && nextSW != 0)
    {
        g_currentFrequency = nextSW;
        g_bandList[g_bandIndex].currentFreq = nextSW;
        resetEepromDelay();

        g_currentBFO = 0;
        if (isSSB())
            updateBFO();
        g_si4735.setFrequency(nextSW);
        agcSetFunc(); //Re-apply to remove noize
        showFrequency();
    }
    else
    {
        if (isSSB())
            g_bandList[g_bandIndex].currentFreq = ssbRoundedFrequencyKHz();

        if (g_currentMode == FM)
            g_bandList[g_bandIndex].currentStepIdx = g_FMStepIndex;
        else
            g_bandList[g_bandIndex].currentStepIdx = g_stepIndex;

        if (up)
        {
            if (g_bandIndex < g_lastBand)
                g_bandIndex++;
            else
                g_bandIndex = 0;
        }
        else
        {
            if (g_bandIndex > 0)
                g_bandIndex--;
            else
                g_bandIndex = g_lastBand;
        }

        if (g_sMeterOn)
        {
            g_sMeterOn = false;
            oledPrint(_literal_EmptyLine, 0, 6, DEFAULT_FONT);
        }

#if USE_RDS
        if (g_displayRDS)
        {
            g_displayRDS = false;
            oledPrint(_literal_EmptyLine, 0, 6, DEFAULT_FONT);
        }
#endif

        g_currentBFO = 0;
        if (isSSB())
            updateBFO();
        applyBandConfiguration();
    }
}

// This function is required for using SSB. Si473x controllers do not support SSB by-default.
// But we can patch internal RAM of Si473x with special patch to make it work in SSB mode.
// Patch must be applied every time we enable SSB after AM or FM.
void loadSSBPatch()
{
    // This works, but i am not sure it's safe
    //g_si4735.setI2CFastModeCustom(700000);
    g_si4735.setI2CFastModeCustom(500000);
    g_si4735.queryLibraryId(); //Do we really need this? Research it.
    g_si4735.patchPowerUp();
    delay(50);
    g_si4735.downloadCompressedPatch(ssb_patch_content, sizeof(ssb_patch_content), cmd_0x15, sizeof(cmd_0x15));
    g_si4735.setSSBConfig(g_bandwidthSSB[g_bwIndexSSB].idx, 1, 0, 1, 0, 1);
    g_si4735.setI2CStandardMode();
    g_ssbLoaded = true;
}

#if USE_RDS
void setRDSConfig(uint8_t bias)
{
    g_si4735.setRdsConfig(1, bias, bias, bias, bias);
}
#endif

//Update receiver settings after changing band and modulation
void applyBandConfiguration(bool extraSSBReset)
{
    if (g_bandIndex == FM_BAND_TYPE)
        g_currentMode = FM;
    else if (!g_ssbLoaded)
        g_currentMode = AM;
    int8_t& step = g_bandList[g_bandIndex].currentStepIdx;
    if (!validStep(step, g_bandIndex, g_currentMode))
        step = 0;
    g_stepIndex = step;
    uint16_t frequency = g_bandList[g_bandIndex].currentFreq;

    g_si4735.setTuneFrequencyAntennaCapacitor(uint16_t(g_bandIndex == SW_BAND_TYPE));
    if (g_bandIndex == FM_BAND_TYPE)
    {
        g_currentMode = FM;
        g_si4735.setTuneFrequencyAntennaCapacitor(0);
        g_si4735.setFM(g_bandList[g_bandIndex].minimumFreq,
            g_bandList[g_bandIndex].maximumFreq,
            frequency,
            flashRead8(&g_tabStepFM[g_bandList[g_bandIndex].currentStepIdx]));
        g_si4735.setSeekFmLimits(g_bandList[g_bandIndex].minimumFreq, g_bandList[g_bandIndex].maximumFreq);
        g_si4735.setSeekFmSpacing(FM_SEEK_SPACING);
        g_ssbLoaded = false;
#if USE_RDS
        setRDSConfig(g_Settings[SettingsIndex::RDSError].param);
#endif
        g_si4735.setFifoCount(1);
        g_bwIndexFM = g_bandList[g_bandIndex].bandwidthIdx;
        g_si4735.setFmBandwidth(g_bwIndexFM);
        g_si4735.setFMDeEmphasis(g_Settings[SettingsIndex::DeEmp].param == 0 ? 1 : 2);
    }
    else
    {
        uint16_t minFreq = g_bandList[g_bandIndex].minimumFreq;
        uint16_t maxFreq = g_bandList[g_bandIndex].maximumFreq;
        if (g_bandIndex == SW_BAND_TYPE)
        {
            minFreq = SW_LIMIT_LOW;
            maxFreq = SW_LIMIT_HIGH;
        }

        if (g_ssbLoaded)
        {
            if (extraSSBReset)
                loadSSBPatch();

            //Call this before to call crazy volume after AM when SVC is off
            g_si4735.setSSBAutomaticVolumeControl(g_Settings[SettingsIndex::SVC].param);
            g_si4735.setSSB(minFreq,
                maxFreq,
                frequency,
                g_bandList[g_bandIndex].currentStepIdx >= g_amTotalSteps ? 0 : flashRead16(&g_tabStep[g_bandList[g_bandIndex].currentStepIdx]),
                g_currentMode == CW ? g_Settings[SettingsIndex::CWSwitch].param + 1 : g_currentMode);
            updateSSBCutoffFilter();
            g_si4735.setSSBAutomaticVolumeControl(g_Settings[SettingsIndex::SVC].param);
            g_si4735.setSSBDspAfc(g_Settings[SettingsIndex::Sync].param == 1 ? 0 : 1);
            g_si4735.setSSBAvcDivider(g_Settings[SettingsIndex::Sync].param == 0 ? 0 : 3); //Set Sync mode
            g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SettingsIndex::SoftMute].param);
            g_si4735.setSSBAudioBandwidth(g_currentMode == CW ? g_bandwidthSSB[0].idx : g_bandwidthSSB[g_bwIndexSSB].idx);
            updateBFO();
            g_si4735.setSSBSoftMute(g_Settings[SettingsIndex::SSM].param);
        }
        else
        {
            g_currentMode = AM;
            g_si4735.setAM(minFreq,
                maxFreq,
                frequency,
                g_bandList[g_bandIndex].currentStepIdx >= g_amTotalSteps ? 0 : flashRead16(&g_tabStep[g_bandList[g_bandIndex].currentStepIdx]));
            g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SettingsIndex::SoftMute].param);
            g_bwIndexAM = g_bandList[g_bandIndex].bandwidthIdx;
            g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
        }

        agcSetFunc();
        g_si4735.setAvcAmMaxGain(g_Settings[SettingsIndex::AutoVolControl].param);
        g_si4735.setSeekAmLimits(minFreq, maxFreq);
        g_si4735.setSeekAmSpacing(amSeekSpacing());
    }

    g_currentFrequency = frequency;
    if (g_currentMode == FM)
        g_FMStepIndex = g_bandList[g_bandIndex].currentStepIdx;
    else
        g_stepIndex = g_bandList[g_bandIndex].currentStepIdx;

    if (!g_settingsActive)
        showStatus(true);
    resetEepromDelay();
}

//Step value regulation
void doStep(int8_t v)
{
    if (g_currentMode == FM)
    {
        g_FMStepIndex = (v == 1) ? g_FMStepIndex + 1 : g_FMStepIndex - 1;
        if (g_FMStepIndex > g_lastStepFM)
            g_FMStepIndex = 0;
        else if (g_FMStepIndex < 0)
            g_FMStepIndex = g_lastStepFM;

        g_si4735.setFrequencyStep(flashRead8(&g_tabStepFM[g_FMStepIndex]));
        g_bandList[g_bandIndex].currentStepIdx = g_FMStepIndex;
        showStep();
    }
    else
    {
        do
        {
            g_stepIndex += v;
            if (g_stepIndex > getLastStep())
                g_stepIndex = 0;
            else if (g_stepIndex < 0)
                g_stepIndex = getLastStep();
        } while (!validStep(g_stepIndex, g_bandIndex, g_currentMode));
        g_bandList[g_bandIndex].currentStepIdx = g_stepIndex;
        if (g_stepIndex < g_amTotalSteps)
            g_si4735.setFrequencyStep(flashRead16(&g_tabStep[g_stepIndex]));
        if (!isSSB())
            g_si4735.setSeekAmSpacing(amSeekSpacing());
        showStep();
    }
}

void updateBFO()
{
    //Actually to move frequency forward you need to move BFO backwards, so just * -1
    // Startup validation and tuning enforce the bounds asserted in ReceiverState.h.
    int16_t calibratedBFO = int16_t(g_currentBFO
        + g_Settings[SettingsIndex::BFO].param * STATE_BFO_CALIBRATION_SCALE);
    g_si4735.setSSBBfo(-calibratedBFO);
}

//Volume control
void doVolume(int8_t v)
{
    if (g_muteVolume)
    {
        g_si4735.setVolume(g_muteVolume);
        g_muteVolume = 0;
    }
    else
    {
        if (v == 1)
            g_si4735.volumeUp();
        else
            g_si4735.volumeDown();
    }
    showVolume();
    resetEepromDelay();
}

//Helps to save more flash image size
void doSwitchLogic(int8_t& param, int8_t low, int8_t high, int8_t step)
{
    if (step)
        resetEepromDelay();
    param += step;
    if (param < low)
        param = high;
    else if (param > high)
        param = low;
}

void agcSetFunc()
{
    uint8_t att = g_Settings[SettingsIndex::ATT].param;
    uint8_t disableAgc = att > 0;
    uint8_t agcNdx;
    if (att > 1) 
        agcNdx = att - 1;
    else
        agcNdx = 0;
    g_si4735.setAutomaticGainControl(disableAgc, agcNdx);
}

//Settings: Attenuation
void doAttenuation(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::ATT].param, 0, 37, v);
    agcSetFunc();
}

//Settings: Soft Mute
void doSoftMute(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::SoftMute].param, 0, 32, v);

    if (g_currentMode != FM)
        g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SettingsIndex::SoftMute].param);
}

//Settings: Brightness
void doBrightness(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::Brightness].param, 5, 125, v);
    oled.setContrast(uint8_t(g_Settings[SettingsIndex::Brightness].param) * 2);
}

//Settings: SSB AVC Switch
void doSSBAVC(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::SVC].param, 0, 1, v);

    if (isSSB())
    {
        g_si4735.setSSBAutomaticVolumeControl(g_Settings[SettingsIndex::SVC].param);
        applyBandConfiguration(true);
    }
}

//Settings: Automatic Volume Control
void doAvc(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::AutoVolControl].param, 12, 90, 2 * v);

    if (g_currentMode != FM)
        g_si4735.setAvcAmMaxGain(g_Settings[SettingsIndex::AutoVolControl].param);
}

//Settings: Sync switch
void doSync(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::Sync].param, 0, 1, v);

    if (isSSB())
    {
        g_si4735.setSSBDspAfc(g_Settings[SettingsIndex::Sync].param == 1 ? 0 : 1);
        g_si4735.setSSBAvcDivider(g_Settings[SettingsIndex::Sync].param == 0 ? 0 : 3); //Set Sync mode
        applyBandConfiguration(true);
    }
}

//Settings: FM DeEmp switch (50 or 75)
void doDeEmp(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::DeEmp].param, 0, 1, v);

    if (g_currentMode == FM)
        g_si4735.setFMDeEmphasis(g_Settings[SettingsIndex::DeEmp].param == 0 ? 1 : 2);
}

//Settings: SW Units
void doSWUnits(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::SWUnits].param, 0, 1, v);
}

//Settings: SW Units
void doSSBSoftMuteMode(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::SSM].param, 0, 1, v);

    if (isSSB())
        g_si4735.setSSBSoftMute(g_Settings[SettingsIndex::SSM].param);
}

//Settings: SSB Cutoff filter
void doCutoffFilter(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::CutoffFilter].param, 0, 2, v);

    if (isSSB())
        updateSSBCutoffFilter();
}

#if USE_RDS
//Settings: RDS Error Level
void doRDSErrorLevel(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::RDSError].param, 0, 3, v);

    if (g_currentMode == FM)
        setRDSConfig(g_Settings[SettingsIndex::RDSError].param);
}

#endif

//Settings: BFO Offset calibration
void doBFOCalibration(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::BFO].param,
        -STATE_MAX_BFO_CALIBRATION, STATE_MAX_BFO_CALIBRATION, v);

    if (isSSB())
    {
        updateBFO();
    }
}

//Settings: Frequency inits displayu switch
void doFreqUnits(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::UnitsShow].param, 0, 1, v);
}

//Settings: Scan button switch
void doScanSwitch(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::ScanSwitch].param, 0, 1, v);
}

//Settings: CW mode switch
void doCWSwitch(int8_t v)
{
    doSwitchLogic(g_Settings[SettingsIndex::CWSwitch].param, 0, 1, v);

    if (g_currentMode == CW)
        applyBandConfiguration(true);
}

#if USE_RDS
void doRDS()
{
    switchCommand(NULL);
    g_displayRDS = !g_displayRDS;

    if (g_displayRDS)
    {
        g_sMeterOn = false;
        oledPrint(_literal_EmptyLine, 0, 6, DEFAULT_FONT);
        g_si4735.getRdsStatus();
        showRDS();
    }
    else
        updateLowerDisplayLine();
}
#endif

//Prevents repeatable code for flash image size saving
void doBandwidthLogic(int8_t& bwIndex, uint8_t upperLimit, int8_t v)
{
    doSwitchLogic(bwIndex, 0, upperLimit, v);
    g_bandList[g_bandIndex].bandwidthIdx = bwIndex;
}

//Bandwidth regulation logic
void doBandwidth(int8_t v)
{
    if (isSSB())
    {
        doSwitchLogic(g_bwIndexSSB, 0, g_bwSSBMaxIdx, v);
        g_si4735.setSSBAudioBandwidth(g_bandwidthSSB[g_bwIndexSSB].idx);
        updateSSBCutoffFilter();
    }
    else if (g_currentMode == AM)
    {
        doBandwidthLogic(g_bwIndexAM, g_maxFilterAM, v);
        g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
    }
    else
    {
        doBandwidthLogic(g_bwIndexFM, 4, v);
        g_si4735.setFmBandwidth(g_bwIndexFM);
    }
    showBandwidth();
}

void switchCommand(bool* b)
{
    bool wasActive = g_cmdVolume || g_cmdStep || g_cmdBw || g_cmdBand;
    bool enable = b && !*b;
    g_cmdVolume = false;
    g_cmdStep = false;
    g_cmdBw = false;
    g_cmdBand = false;
    if (enable)
    {
        *b = true;
        g_lastAdjustmentTime = millis();
    }
    else
    {
        g_lastAdjustmentTime = 0;
    }

    if (wasActive || enable)
    {
        showVolume();
        showStep();
        showBandwidth();
        showModulation();
    }
}

bool clampSSBBand()
{
    bool wrapped = false;
    if (ssbFrequencyKHzCeil() > int16_t(g_bandList[g_bandIndex].maximumFreq))
    {
        g_currentFrequency = g_bandList[g_bandIndex].minimumFreq;
        wrapped = true;
    }
    else if (ssbFrequencyKHzFloor() < int16_t(g_bandList[g_bandIndex].minimumFreq))
    {
        g_currentFrequency = g_bandList[g_bandIndex].maximumFreq;
        wrapped = true;
    }

    if (wrapped)
    {
        g_currentBFO = 0;
        g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
        updateBFO();
        g_si4735.setFrequency(g_currentFrequency);
        showFrequency(true);
        showModulation();
        return true;
    }

    return false;
}

void doFrequencyTune(int8_t direction)
{
    g_seekDirection = direction == 1 ? 1 : 0;

    //Update frequency
    resetEepromDelay();
    if (g_currentMode == FM)
    {
        g_currentFrequency += flashRead8(&g_tabStepFM[g_FMStepIndex]) * direction; //g_si4735.getFrequency() is too slow
#if USE_RDS
        if (g_displayRDS)
            oledPrint(_literal_EmptyLine, 0, 6, DEFAULT_FONT);
#endif
    }
    else
        g_currentFrequency += flashRead16(&g_tabStep[g_stepIndex]) * direction;
    uint16_t bMin = g_bandList[g_bandIndex].minimumFreq, bMax = g_bandList[g_bandIndex].maximumFreq;

    //Special logic for fast and responsive frequency surfing
    if (g_currentFrequency > bMax)
        g_currentFrequency = bMin;
    else if (g_currentFrequency < bMin)
        g_currentFrequency = bMax;

    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    g_processFreqChange = true;
    g_lastFreqChange = millis();

    showFrequency();
}

void resetLowerLine()
{
    if (g_sMeterOn || g_displayRDS)
    {
        g_sMeterOn = false;
        g_displayRDS = false;
        updateLowerDisplayLine();
    }
}

//Special feature to make SSB feel like on expensive TECSUN receivers
//BFO is now part of main frequency in SSB mode
void doFrequencyTuneSSB(int8_t direction)
{
    g_seekDirection = direction == 1 ? 1 : 0;
    uint16_t previousBase = g_currentFrequency;
    int16_t newBFO = int16_t(g_currentBFO + (direction == 1 ? getSteps() : -getSteps()));
    while (newBFO > STATE_MAX_BFO_HZ)
    {
        g_currentFrequency += STATE_MAX_BFO_KHZ;
        newBFO -= STATE_MAX_BFO_HZ;
    }
    while (newBFO < -STATE_MAX_BFO_HZ)
    {
        g_currentFrequency -= STATE_MAX_BFO_KHZ;
        newBFO += STATE_MAX_BFO_HZ;
    }

    g_currentBFO = newBFO;
    g_lastFreqChange = millis();
    resetEepromDelay();
    // Check the effective frequency before sending either BFO or carrier commands.
    if (clampSSBBand())
        return;
    updateBFO();

    if (g_currentFrequency != previousBase)
    {
        g_si4735.setFrequency(g_currentFrequency);
        agcSetFunc(); //Re-apply to remove noize
        uint16_t tuned = g_si4735.getFrequency();
        if (frequencyInBand(tuned))
            g_currentFrequency = tuned;
    }

    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
    showFrequency();
}

void handleButtons()
{
    //Command-checkers
    if (BUTTONEVENT_SHORTPRESS == btn_Bandwidth.checkEvent(simpleEvent))
    {
        if (!g_settingsActive && g_currentMode != CW)
            switchCommand(&g_cmdBw);
    }
    if (BUTTONEVENT_SHORTPRESS == btn_BandUp.checkEvent(bandEvent))
    {
        if (!g_settingsActive)
        {
            resetLowerLine();
            switchCommand(&g_cmdBand);
        }
        else
        {
            switchSettingsPage();
        }
    }
    if (BUTTONEVENT_SHORTPRESS == btn_BandDn.checkEvent(bandEvent))
    {
        if (!g_settingsActive)
            switchCommand(NULL);
        g_settingsActive = !g_settingsActive;
        switchSettings();
    }
    if (BUTTONEVENT_SHORTPRESS == btn_VolumeUp.checkEvent(volumeEvent))
    {
        if (!g_settingsActive && g_muteVolume == 0)
            switchCommand(&g_cmdVolume);
    }
    if (BUTTONEVENT_SHORTPRESS == btn_VolumeDn.checkEvent(volumeEvent))
    {
        if (!g_cmdVolume)
        {
            uint8_t vol = g_si4735.getCurrentVolume();
            if (vol > 0 && g_muteVolume == 0)
            {
                g_muteVolume = vol;
                g_si4735.setVolume(0);
            }
            else if (g_muteVolume > 0)
            {
                g_si4735.setVolume(g_muteVolume);
                g_muteVolume = 0;
            }
            showVolume();
        }
    }
    if (BUTTONEVENT_SHORTPRESS == btn_Encoder.checkEvent(simpleEvent))
    {
        if (g_cmdBand)
        {
            g_cmdBand = false;
            showModulation();
        }
        else if (g_cmdStep)
        {
            g_cmdStep = false;
            showStep();
        }
        else if (g_cmdBw)
        {
            g_cmdBw = false;
            showBandwidth();
        }
        else if (g_cmdVolume)
        {
            g_cmdVolume = false;
            showVolume();
        }
        else if (g_settingsActive)
        {
            g_SettingEditing = !g_SettingEditing;
            DrawSetting(g_SettingSelected, true);
        }
        else if (g_displayRDS)
            g_rdsSwitchPressed = true;
        else if (isSSB() || g_Settings[SettingsIndex::ScanSwitch].param == 0)
        {
            if (!g_settingsActive)
            {
                resetLowerLine();
                switchCommand(&g_cmdStep);
            }
        }
        //Seek in SSB/CW is not allowed
        else if (g_currentMode == FM || g_currentMode == AM)
            doSeek();
    }

    uint8_t event = btn_AGC.checkEvent();
    if (BUTTONEVENT_SHORTPRESS == event)
    {
        if (!g_settingsActive || !g_displayOn)
        {
            g_displayOn = !g_displayOn;
            if (g_displayOn)
                oled.on();
            else
                oled.off();
        }
    }
    if (BUTTONEVENT_FIRSTLONGPRESS == event)
    {
        if (!g_settingsActive)
        {
            if (isSSB())
                doSync(1);
        }
    }
    event = btn_Step.checkEvent();
    if (BUTTONEVENT_SHORTPRESS == event)
    {
        if (!g_settingsActive)
        {
            resetLowerLine();
            switchCommand(&g_cmdStep);
        }
    }
    if (BUTTONEVENT_LONGPRESSDONE == event)
    {
        if (!g_settingsActive)
        {
            switchCommand(NULL);
            g_sMeterOn = !g_sMeterOn;
            if (g_sMeterOn)
            {
                g_displayRDS = false;
                showSMeter();
            }
            else
                updateLowerDisplayLine();
        }
    }
    if (BUTTONEVENT_SHORTPRESS == btn_Mode.checkEvent(simpleEvent))
    {
        if (!g_settingsActive)
        {
            //Do nothing on FM mode (unfortunately no NBFM patch), otherwise switch AM modulation
            if (g_currentMode != FM)
            {
                switchCommand(NULL);
                switch (g_currentMode)
                {
                case AM:
                    //Patch Si473x memory every time when enabling SSB
                    loadSSBPatch();
                    g_processFreqChange = false;
                    g_currentBFO = 0;
                    // fall through

                case LSB:
                    g_currentMode++;
                    break;

                case USB:
                    g_currentMode++;
                    g_cmdBw = false;
                    break;

                case CW:
                    g_currentMode = AM;
                    g_ssbLoaded = false;
                    g_currentFrequency = ssbRoundedFrequencyKHz();
                    g_bandList[g_bandIndex].currentFreq = g_currentFrequency;
                    if (g_stepIndex >= g_amTotalSteps)
                        g_stepIndex = 0;

                    g_currentBFO = 0;
                    break;
                }

                g_bandList[g_bandIndex].currentStepIdx = g_stepIndex;
                applyBandConfiguration();
            }
#if USE_RDS
            else
                doRDS();
#endif
        }
    }
}

void loop()
{
    int8_t direction = takeEncoderStep();
    bool skipButtonEvents = false;
    bool frequencyRecentlyUpdated = millis() - g_lastFreqChange < 70;

    //Faster frequency tune
    if (g_processFreqChange && !isSSB())
    {
        if (!frequencyRecentlyUpdated && direction == 0)
        {
            g_si4735.setFrequency(g_currentFrequency);
            g_processFreqChange = false;
        }
    }

    if (millis() - g_lastFreqChange >= 1000)
    {
#if USE_RDS
        showRDS();
#endif

        if (g_sMeterOn && !g_settingsActive)
            showSMeter();

        showCharge(false);
    }

    if (g_lastAdjustmentTime != 0 && millis() - g_lastAdjustmentTime > ADJUSTMENT_ACTIVE_TIMEOUT)
        switchCommand(NULL);

    //Encoder rotation check
    if (direction != 0)
    {
        if (g_lastAdjustmentTime != 0)
            g_lastAdjustmentTime = millis();

        if (g_settingsActive)
        {
            if (!g_SettingEditing)
            {
                int8_t prev = g_SettingSelected;
                int8_t next = g_SettingSelected;

                next += direction;

                uint8_t pageIdx = g_SettingsPage - 1;
                uint8_t maxOnThisPage = (pageIdx * 6) + 5;
                if (maxOnThisPage >= SettingsIndex::SETTINGS_MAX)
                    maxOnThisPage = SettingsIndex::SETTINGS_MAX - 1;

                if (next < pageIdx * 6)
                    g_SettingSelected = maxOnThisPage;
                else if (next > maxOnThisPage)
                    g_SettingSelected = pageIdx * 6;
                else
                    g_SettingSelected = next;

                DrawSetting(prev, true);
                DrawSetting(g_SettingSelected, true);
            }
            else
            {
                (*g_Settings[g_SettingSelected].manipulateCallback)(direction);
                DrawSetting(g_SettingSelected, false);
                delay(MIN_ELAPSED_TIME);
            }
        }
        else if (g_cmdVolume)
            doVolume(direction);
        else if (g_cmdStep)
            doStep(direction);
        else if (g_cmdBw)
            doBandwidth(direction);
        else if (g_cmdBand)
        {
            if (direction == 1)
                bandSwitch(true);
            else
                bandSwitch(false);
        }
        else if (isSSB())
        {
            doFrequencyTuneSSB(direction);
            skipButtonEvents = true;
        }
        else
        {
            doFrequencyTune(direction);
            skipButtonEvents = true;
        }
        if (!g_settingsActive)
            resetEepromDelay();
    }

    if (!skipButtonEvents)
        handleButtons();

    if (g_settingsDirty && (millis() - g_storeTime) > STORE_TIME)
        saveAllReceiverInformation();
}

//Overriding original main to save some space
int main(void)
{
    init();
    setup();
    while(1)
        loop();
    return 0;
}
