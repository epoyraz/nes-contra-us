#!/bin/sh
# Renderer-level check: dump both sides' framebuffers plus render telemetry at
# the given frames of the reference recording, then diff framebuffers
# palette-independently.
#
#   tools/render_check.sh [RECORDING.jsonl] FRAME [FRAME...]
#
# Output: tmp/render/<frame>.png triptychs (mesen | native | diff) + a summary.
# Requires: baserom.nes, a play recording, Mesen at $CONTRA_MESEN_APP
# (default ~/Applications/Mesen.app).
set -e

cd "$(dirname "$0")/.."
ROOT="$(pwd)"
MESEN="${CONTRA_MESEN_APP:-$HOME/Applications/Mesen.app}/Contents/MacOS/Mesen"

[ $# -ge 1 ] || { echo "usage: tools/render_check.sh [RECORDING.jsonl] FRAME [FRAME...]" >&2; exit 2; }

abspath() {
    case "$1" in
        /*) printf '%s\n' "$1" ;;
        *) printf '%s\n' "$ROOT/$1" ;;
    esac
}

if [ $# -ge 2 ] && [ -f "$1" ]; then
    CONTRA_RENDER_RECORDING="$(abspath "$1")"
    shift
fi

MESEN_RECORDING="${CONTRA_RENDER_MESEN_RECORDING:-${CONTRA_RENDER_RECORDING:-$ROOT/tmp/reference_recording_v2.jsonl}}"
NATIVE_RECORDING="${CONTRA_RENDER_NATIVE_RECORDING:-${CONTRA_RENDER_RECORDING:-$ROOT/tmp/reference_mesen_v4.jsonl}}"
MESEN_RECORDING="$(abspath "$MESEN_RECORDING")"
NATIVE_RECORDING="$(abspath "$NATIVE_RECORDING")"
NATIVE_INPUT="${CONTRA_RENDER_NATIVE_INPUT:-latched}"

FRAMES=$(echo "$@" | tr ' ' ',')
MAXF=$(( $(echo "$@" | tr ' ' '\n' | sort -n | tail -1) + 5 ))
mkdir -p tmp/render

echo "== mesen headless re-trace to frame $MAXF (telemetry dumps at: $FRAMES)"
CONTRA_MESEN_PLAY_REPLAY_JSONL="$MESEN_RECORDING" \
CONTRA_MESEN_PLAY_RECORDING_JSONL="$ROOT/tmp/render/mesen_rerun.jsonl" \
CONTRA_MESEN_PLAY_DUMP_FRAMES="$FRAMES" \
CONTRA_MESEN_PLAY_RAM_DUMP_PATH="$ROOT/tmp/render/mesen_ram.bin" \
CONTRA_MESEN_PLAY_OAM_DUMP_PATH="$ROOT/tmp/render/mesen_oam.bin" \
CONTRA_MESEN_PLAY_NAMETABLE_DUMP_PATH="$ROOT/tmp/render/mesen_nametable.bin" \
CONTRA_MESEN_PLAY_PALETTE_DUMP_PATH="$ROOT/tmp/render/mesen_palette.bin" \
CONTRA_MESEN_PLAY_FRAMEBUFFER_DUMP_PATH="$ROOT/tmp/render/mesen_fb.bin" \
CONTRA_MESEN_PLAY_CHR_DUMP_PATH="$ROOT/tmp/render/mesen_chr.bin" \
CONTRA_MESEN_PLAY_PPU_DUMP_PATH="$ROOT/tmp/render/mesen_ppu.bin" \
CONTRA_MESEN_PLAY_SUPERTILE_DUMP_PATH="$ROOT/tmp/render/mesen_supertile.bin" \
CONTRA_MESEN_PLAY_MAX_FRAME="$MAXF" \
"$MESEN" --testRunner --doNotSaveSettings --timeout=900 \
    "$ROOT/baserom.nes" "$ROOT/tools/mesen_play_recorder.lua" > /dev/null 2>&1

echo "== native replay (telemetry dumps at: $FRAMES)"
CONTRA_NATIVE_PLAY_INPUT="$NATIVE_INPUT" \
CONTRA_NATIVE_PLAY_DUMP_FRAMES="$FRAMES" \
CONTRA_NATIVE_PLAY_RAM_DUMP_PATH="$ROOT/tmp/render/native_ram.bin" \
CONTRA_NATIVE_PLAY_OAM_DUMP_PATH="$ROOT/tmp/render/native_oam.bin" \
CONTRA_NATIVE_PLAY_NAMETABLE_DUMP_PATH="$ROOT/tmp/render/native_nametable.bin" \
CONTRA_NATIVE_PLAY_PALETTE_DUMP_PATH="$ROOT/tmp/render/native_palette.bin" \
CONTRA_NATIVE_PLAY_FRAMEBUFFER_DUMP_PATH="$ROOT/tmp/render/native_fb.bin" \
CONTRA_NATIVE_PLAY_CHR_DUMP_PATH="$ROOT/tmp/render/native_chr.bin" \
CONTRA_NATIVE_PLAY_PPU_DUMP_PATH="$ROOT/tmp/render/native_ppu.bin" \
CONTRA_NATIVE_PLAY_SUPERTILE_DUMP_PATH="$ROOT/tmp/render/native_supertile.bin" \
CONTRA_NATIVE_PLAY_MAX_FRAME="$MAXF" \
./build/port/contra_play_replay_trace "$NATIVE_RECORDING" > /dev/null

STATUS=0
for f in "$@"; do
    echo "== frame $f"
    if python3 tools/render_diff.py \
        "tmp/render/mesen_fb.bin.$f" "tmp/render/native_fb.bin.$f" \
        "tmp/render/$f"; then
        :
    else
        STATUS=1
    fi
done
exit $STATUS
