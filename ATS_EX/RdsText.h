#pragma once

#include <stdint.h>

// The OLED has 16 text columns. Library buffers are borrowed and never modified.
inline void formatRdsLine(char* line, const char* text)
{
    bool missing = !text || !*text;
    uint8_t i = 0;
    if (text)
        while (i < 16 && *text)
            line[i++] = *text++;
    while (i < 16)
        line[i++] = ' ';
    if (missing)
        line[0] = line[1] = line[2] = '.';
    line[16] = '\0';
}
