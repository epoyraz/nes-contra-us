#!/bin/sh
# Renderer-level check: dump both sides' framebuffers at the given frames of
# the reference recording and diff them palette-independently.
#
#   tools/render_check.sh FRAME [FRAME...]
#
# Output: tmp/render/<frame>.png triptychs (mesen | native | diff) + a summary.
# Requires: baserom.nes, tmp/reference_recording_v2.jsonl (raw inputs for the
# Mesen re-trace), tmp/reference_mesen_v4.jsonl (replay input), Mesen at
# $CONTRA_MESEN_APP (default ~/Applications/Mesen.app).
set -e

cd "$(dirname "$0")/.."
ROOT="$(pwd)"
MESEN="${CONTRA_MESEN_APP:-$HOME/Applications/Mesen.app}/Contents/MacOS/Mesen"

[ $# -ge 1 ] || { echo "usage: tools/render_check.sh FRAME [FRAME...]" >&2; exit 2; }

FRAMES=$(echo "$@" | tr ' ' ',')
MAXF=$(( $(echo "$@" | tr ' ' '\n' | sort -n | tail -1) + 5 ))
mkdir -p tmp/render

echo "== mesen headless re-trace to frame $MAXF (framebuffer dumps at: $FRAMES)"
CONTRA_MESEN_PLAY_REPLAY_JSONL="$ROOT/tmp/reference_recording_v2.jsonl" \
CONTRA_MESEN_PLAY_RECORDING_JSONL="$ROOT/tmp/render/mesen_rerun.jsonl" \
CONTRA_MESEN_PLAY_DUMP_FRAMES="$FRAMES" \
CONTRA_MESEN_PLAY_FRAMEBUFFER_DUMP_PATH="$ROOT/tmp/render/mesen_fb.bin" \
CONTRA_MESEN_PLAY_MAX_FRAME="$MAXF" \
"$MESEN" --testRunner --doNotSaveSettings --timeout=900 \
    "$ROOT/baserom.nes" "$ROOT/tools/mesen_play_recorder.lua" > /dev/null 2>&1

echo "== native replay (framebuffer dumps at: $FRAMES)"
CONTRA_NATIVE_PLAY_INPUT=latched \
CONTRA_NATIVE_PLAY_DUMP_FRAMES="$FRAMES" \
CONTRA_NATIVE_PLAY_FRAMEBUFFER_DUMP_PATH="$ROOT/tmp/render/native_fb.bin" \
CONTRA_NATIVE_PLAY_MAX_FRAME="$MAXF" \
./build/port/contra_play_replay_trace tmp/reference_mesen_v4.jsonl > /dev/null

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
