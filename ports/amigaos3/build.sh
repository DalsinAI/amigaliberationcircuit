#!/bin/sh
# Copyright (c) 2026 AmiCradle Team. GPL-3.0 with the game (see LICENSE.md).
# Called by build-amiga.sh. See ports/amigaos3/README.md.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
PORT="$ROOT/ports/amigaos3"
STOVE=${STOVE:-"$HOME/AmiCradle/kitchen/stoves/os32-gcc16"}
OUT=${OUT:-"$ROOT/build-amiga"}
A5_PREFIX=${A5_PREFIX:-"$OUT/allegro-prefix"}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
P="$STOVE/prefix"
SYSROOT="$P/m68k-amigaos"
CC="$P/bin/m68k-amigaos-gcc"
STRIP="$P/bin/m68k-amigaos-strip"
PIN=$(cat "$PORT/allegro.pin")
mkdir -p "$OUT"

for f in "$CC" "$SYSROOT/lib/libSDL2.a" "$SYSROOT/lib/libGL.a" "$SYSROOT/port-deps/lib/libpng.a" "$SYSROOT/port-deps/lib/libz.a"; do
    [ -e "$f" ] || { echo "missing in the stove: $f" >&2; exit 3; }
done

# ---- amigaallegro, once per pin -----------------------------------------------------
if [ "$(cat "$A5_PREFIX/.pin" 2>/dev/null || true)" != "$PIN" ]; then
    SRC="$OUT/amigaallegro-src"
    if [ ! -d "$SRC/.git" ]; then
        git clone --filter=blob:none --no-checkout https://github.com/DalsinAI/amigaallegro.git "$SRC"
    fi
    git -C "$SRC" cat-file -e "$PIN^{commit}" 2>/dev/null || git -C "$SRC" fetch --depth 1 origin "$PIN"
    git -C "$SRC" -c advice.detachedHead=false checkout -q --detach "$PIN"
    rm -rf "$OUT/amigaallegro-build" "$A5_PREFIX"
    cmake -S "$SRC" -B "$OUT/amigaallegro-build" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_TOOLCHAIN_FILE="$SRC/cmake/Toolchain-amigaos3.cmake" \
        -DAMIGA_STOVE="$STOVE" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$A5_PREFIX" \
        -DALLEGRO_SDL=ON \
        -DALLEGRO_AMIGA_BITMAP_BATCH_LIMIT=0 \
        -DSHARED=OFF -DWANT_MONOLITH=ON -DWANT_ALLOW_SSE=OFF \
        -DWANT_DOCS=OFF -DWANT_TESTS=OFF -DWANT_EXAMPLES=OFF -DWANT_DEMO=OFF \
        -DWANT_OPENAL=OFF -DWANT_VIDEO=OFF -DWANT_NATIVE_DIALOG=OFF \
        -DWANT_TTF=OFF -DWANT_PHYSFS=OFF -DWANT_COLOR=ON -DWANT_MEMFILE=ON \
        -DWANT_IMAGE_FREEIMAGE=OFF -DWANT_IMAGE_WEBP=OFF -DWANT_IMAGE_JPG=OFF -DWANT_IMAGE_PNG=ON \
        -DWANT_FLAC=OFF -DWANT_VORBIS=OFF -DWANT_OPUS=OFF -DWANT_MODAUDIO=OFF -DWANT_OPENSL=OFF \
        -DWANT_MP3=OFF -DWANT_ACODEC_DYNAMIC_LOAD=OFF \
        -DSDL2_INCLUDE_DIR="$SYSROOT/include/SDL2" \
        -DSDL2_LIBRARY_TEMP="$SYSROOT/lib/libSDL2.a" \
        -DOPENGL_gl_LIBRARY="$SYSROOT/lib/libGL.a" \
        -DOPENGL_INCLUDE_DIR="$SYSROOT/include" \
        -DPNG_PNG_INCLUDE_DIR="$SYSROOT/port-deps/include" \
        -DPNG_LIBRARY="$SYSROOT/port-deps/lib/libpng.a" \
        -DZLIB_INCLUDE_DIR="$SYSROOT/port-deps/include" \
        -DZLIB_LIBRARY="$SYSROOT/port-deps/lib/libz.a" \
        -DALLEGRO_HAVE_FSEEKO=0 -DALLEGRO_HAVE_FTELLO=0 \
        -DALLEGRO_HAVE_STRERROR_R=0 -DALLEGRO_HAVE_STRERROR_S=0
    cmake --build "$OUT/amigaallegro-build" --parallel "$JOBS"
    cmake --install "$OUT/amigaallegro-build"
    echo "$PIN" > "$A5_PREFIX/.pin"
fi
ALLEGRO_LIB="$A5_PREFIX/lib/liballegro_monolith-static.a"
[ -f "$ALLEGRO_LIB" ] || { echo "amigaallegro did not install $ALLEGRO_LIB" >&2; exit 3; }

# ---- the game ---------------------------------------------------------------------------
# Upstream's own flags, plus the Amiga target (as amigaallegro is built)
CFLAGS="-mcrt=nix20 -m68040 -m68881 -pthread -O2 -fomit-frame-pointer -fno-delete-null-pointer-checks -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-misleading-indentation -DALLEGRO_STATICLINK=1 -D__AMIGAOS3__=1"
OBJ="$OUT/obj"
mkdir -p "$OBJ"
OBJS=""
pids=""
n=0
for c in "$ROOT"/src/*.c "$PORT/compat/native_dialog.c" "$PORT/compat/amiga_pattern.c"; do
    o="$OBJ/$(basename "$c" .c).o"
    OBJS="$OBJS $o"
    if [ ! -f "$o" ] || [ "$c" -nt "$o" ] || [ "$PORT/compat/amiga_pattern.h" -nt "$o" ] || [ "$ALLEGRO_LIB" -nt "$o" ]; then
        # shellcheck disable=SC2086
        "$CC" $CFLAGS -I"$PORT/compat" -I"$A5_PREFIX/include" -I"$SYSROOT/include" -c -o "$o" "$c" &
        pids="$pids $!"
        n=$((n + 1))
        if [ "$n" -ge "$JOBS" ]; then for p in $pids; do wait "$p"; done; pids=""; n=0; fi
    fi
done
for p in $pids; do wait "$p"; done
DRAWER="$OUT/LiberationCircuit"
mkdir -p "$DRAWER"
# shellcheck disable=SC2086
"$CC" -mcrt=nix20 -m68040 -m68881 -pthread -o "$OUT/libcirc.debug" $OBJS \
    "$ALLEGRO_LIB" "$SYSROOT/lib/libSDL2.a" "$SYSROOT/lib/libGL.a" \
    "$SYSROOT/port-deps/lib/libpng.a" "$SYSROOT/port-deps/lib/libz.a" -lpthread -lm
"$STRIP" -o "$DRAWER/LiberationCircuit" "$OUT/libcirc.debug"
# the data, beside the program (the game finds it from its own drawer)
rm -rf "$DRAWER/data" "$DRAWER/proc" "$DRAWER/story"
cp -R "$ROOT/bin/data" "$ROOT/bin/proc" "$ROOT/bin/story" "$DRAWER/"
cp "$ROOT/bin/init.txt" "$ROOT/bin/licence.txt" "$ROOT/bin/readme.txt" "$ROOT/bin/Manual.html" "$DRAWER/"
cp "$PORT/README.md" "$DRAWER/AMIGA-README.md"
echo "$DRAWER/LiberationCircuit ($(wc -c < "$DRAWER/LiberationCircuit") bytes)"
sha256sum "$DRAWER/LiberationCircuit"
