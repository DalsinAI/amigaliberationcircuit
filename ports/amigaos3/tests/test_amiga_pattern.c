/* Host unit test for compat/amiga_pattern.c: cc -I../compat test_amiga_pattern.c ../compat/amiga_pattern.c */
#include <stdio.h>
#include <string.h>
#include "amiga_pattern.h"

static int checks, failures;
static void check(const char *in, const char *want)
{
    char out[128];
    to_amiga_pattern(out, sizeof(out), in);
    checks++;
    if (strcmp(out, want) != 0) { failures++; fprintf(stderr, "'%s' -> '%s', want '%s'\n", in ? in : "(null)", out, want); }
}

int main(void)
{
    char small[8];
    check(NULL, "#?");
    check("", "#?");
    check(" ; ", "#?");
    check("*.c", "#?.c");
    check("*.c;*.h", "(#?.c|#?.h)");
    check("*.c; *.h", "(#?.c|#?.h)");
    check("*.c;", "#?.c");
    check(";*.txt;*.c;", "(#?.txt|#?.c)");
    check("*.*", "#?.#?");
    check("save?.dat", "save?.dat");
    to_amiga_pattern(small, sizeof(small), "*.longname;*.x");
    checks++;
    if (strlen(small) >= sizeof(small)) { failures++; fprintf(stderr, "overflow\n"); }
    printf("amiga_pattern: %d checks, %d failed\n", checks, failures);
    return failures != 0;
}
