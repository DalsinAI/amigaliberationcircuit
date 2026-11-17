#!/bin/sh
# Host unit tests for the Amiga port's own code (no Amiga needed).
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CC=${CC:-cc}
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
$CC -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -I"$HERE/../compat" \
    -o "$T/test_amiga_pattern" "$HERE/test_amiga_pattern.c" "$HERE/../compat/amiga_pattern.c"
"$T/test_amiga_pattern"
