#pragma once

#include <stdint.h>

// The OLED has 16 text columns. Library buffers are borrowed and never modified.
inline void formatRdsLine(char* line, const char* text)
{
    bool missing = !text || !*text;
    uint8_t i = 0;
    if (text)
        while (i < 16 && *text)
        {
            uint8_t c = uint8_t(*text++);
            line[i++] = (c >= 0x20 && c <= 0x7E) ? char(c) : ' ';
        }
    while (i < 16)
        line[i++] = ' ';
    if (missing)
        line[0] = line[1] = line[2] = '.';
    line[16] = '\0';
}

// Fill a 64-column RDS page. The OLED font wraps every 16 characters.
// NUL gaps become spaces; 0x0D terminates the text.
inline void formatRdsText(char* line, const char* text, uint8_t maxLen)
{
    bool ended = false;
    for (uint8_t i = 0; i < 64; i++)
    {
        uint8_t c = 0;
        if (!ended)
        {
            if (text && i < maxLen)
            {
                c = uint8_t(text[i]);
                if (c == 0x0D)
                {
                    ended = true;
                    c = 0;
                }
            }
        }
        if (c == 0)
            c = ' ';
        line[i] = (c >= 0x20 && c <= 0x7E) ? char(c) : ' ';
    }
    line[64] = '\0';
    if (!text)
        line[0] = line[1] = line[2] = '.';
}
