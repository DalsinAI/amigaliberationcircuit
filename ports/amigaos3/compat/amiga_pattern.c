/* Allegro file dialog patterns to AmigaDOS patterns.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0 with the game (see LICENSE.md).
 */
#include <string.h>
#include "amiga_pattern.h"

void to_amiga_pattern(char *out, size_t size, const char *patterns)
{
    size_t n = 0;
    int parts = 0, in_part = 0;
    const char *p;
    if (!out || size < 3) { if (out && size) out[0] = 0; return; }
    /* count the patterns between ';' and ' ' separators */
    for (p = patterns ? patterns : ""; *p; p++) {
        if (*p == ';' || *p == ' ') in_part = 0;
        else if (!in_part) { in_part = 1; parts++; }
    }
    if (parts == 0) { strcpy(out, "#?"); return; }
    if (parts > 1) out[n++] = '(';
    in_part = 0;
    for (p = patterns; *p && n + 3 < size; p++) {
        if (*p == ';' || *p == ' ') { in_part = 0; continue; }
        if (!in_part) { if (n > 1 || (n == 1 && parts == 1)) out[n++] = '|'; in_part = 1; }
        if (*p == '*') { out[n++] = '#'; out[n++] = '?'; }
        else out[n++] = *p;
    }
    if (parts > 1) out[n++] = ')';
    out[n] = 0;
}
