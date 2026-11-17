# Liberation Circuit for classic AmigaOS 3.x

Liberation Circuit (Linley Henzell, GPL-3.0) on AmigaOS 3.x, 68040 with an FPU.
It is built for fast machines: a PiStorm or an AC090 with OpenGPU. The game
wants a 1024x768 display.

## How it runs

The game's own Allegro 5 calls are unchanged. It is linked with amigaallegro
(DalsinAI/amigaallegro, pinned in `allegro.pin`): Allegro 5 on SDL2.module and
GL.module, drawing through OpenGPU. This is the same path OpenSurge takes.

Changes for the Amiga:
- `compat/native_dialog.c`: the game's load and save file dialogs are asl.library
  file requesters on the game's screen; message boxes are Intuition requesters.

## Building

    ./build-amiga.sh        STOVE=<os32-gcc16 stove>  OUT=<dir>

This builds amigaallegro at the pinned commit once, then the game, into
`OUT/LiberationCircuit/`: the program with `data/`, `proc/`, `story/`,
`init.txt` and the manual. Settings are in `init.txt`.

The port's own code has host unit tests: `ports/amigaos3/tests/run_tests.sh`.

## State

Builds on the GCC 16 stove: amigaallegro 823f17f static, the game 2,721,720 B
stripped, no source changes to the game. Not yet run on an Amiga (labs closed
16 Nov night); the first run checks 1024x768 on OpenGPU, the ASL requesters
and sound.
