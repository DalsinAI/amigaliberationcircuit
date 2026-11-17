#!/bin/sh
# Liberation Circuit for classic AmigaOS 3.x (68040 + FPU), with the GCC 16
# stove: builds amigaallegro (pinned in ports/amigaos3/allegro.pin) as a
# static monolith, then the game, linked with SDL2.module and GL.module's
# link libraries, so it draws through OpenGPU.
#   build-amiga.sh     STOVE=dir (default ~/AmiCradle/kitchen/stoves/os32-gcc16)
#                      OUT=dir (default ./build-amiga)  A5_PREFIX=dir (reuse an
#                      Allegro build; default OUT/allegro-prefix)
# Out: OUT/LiberationCircuit/ (the program, its data, the readme): the drawer
# a release ships.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec sh "$HERE/ports/amigaos3/build.sh" "$@"
