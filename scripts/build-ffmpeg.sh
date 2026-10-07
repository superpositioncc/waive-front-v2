#!/usr/bin/env bash

# WAIVE-FRONT
# Copyright (C) 2024  Bram Bogaerts, Superposition
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Builds the small, static FFmpeg that WAIVE-FRONT links against: decoders and
# demuxers for common video files, the filters the palette extraction uses, and
# nothing else. No external libraries, so the result runs on any machine.
#
# Usage: build-ffmpeg.sh <ffmpeg source dir> <install prefix> [arch] [extra configure args...]
#
# arch is arm64 or x86_64 on macOS (default: the machine's own). On Windows, run
# it from an MSYS2 shell inside a Visual Studio developer environment and pass
# --toolchain=msvc as an extra argument.

set -euo pipefail

SRC="$1"
PREFIX="$2"
ARCH="${3:-}"
shift $(( $# < 3 ? $# : 3 ))

ARGS=(
    --prefix="$PREFIX"
    --enable-static --disable-shared --enable-pic
    --disable-programs --disable-doc --disable-network --disable-autodetect
    --disable-everything --disable-avdevice --disable-swresample
    --enable-protocol=file
    --enable-demuxer=mov,matroska,avi,mpegts,mpegps,m4v,h264,hevc,mjpeg
    --enable-decoder=h264,hevc,mpeg4,mpeg2video,mpeg1video,h263,prores,vp8,vp9,mjpeg,dnxhd,png
    --enable-parser=h264,hevc,mpeg4video,mpegvideo,h263,vp8,vp9,mjpeg,dnxhd,png
    --enable-filter=buffer,buffersink,palettegen,format,scale,null
)

if [[ "$(uname -s)" == "Darwin" ]]; then
    ARCH="${ARCH:-$(uname -m)}"
    MIN="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
    ARGS+=(--arch="$ARCH" --cc="clang -arch $ARCH"
           --extra-cflags="-mmacosx-version-min=$MIN" --extra-ldflags="-mmacosx-version-min=$MIN")
    if [[ "$ARCH" != "$(uname -m)" ]]; then
        ARGS+=(--enable-cross-compile --target-os=darwin)
    fi
fi

# x86 builds want nasm for the fast decoder paths. Without it they still work, only slower.
if [[ "$ARCH" == "x86_64" || ( -z "$ARCH" && "$(uname -m)" == "x86_64" ) ]] && ! command -v nasm >/dev/null; then
    echo "build-ffmpeg: nasm not found, building without x86 assembly (slower decoding)" >&2
    ARGS+=(--disable-x86asm)
fi

BUILD="$PREFIX.build"
rm -rf "$BUILD"
mkdir -p "$BUILD"
cd "$BUILD"

"$SRC/configure" "${ARGS[@]}" "$@"
make -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
make install
