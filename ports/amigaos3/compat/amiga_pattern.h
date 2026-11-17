/* Allegro file dialog patterns to AmigaDOS patterns.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0 with the game (see LICENSE.md).
 */
#ifndef AMIGA_PATTERN_H
#define AMIGA_PATTERN_H
#include <stddef.h>
/* "*.c" -> "#?.c", "*.c;*.h" -> "(#?.c|#?.h)", empty or NULL -> "#?" */
void to_amiga_pattern(char *out, size_t size, const char *patterns);
#endif
