#!/bin/sh
# One-command parity cycle: build + replay + structural report.
#
#   tools/frontier.sh                # full replay of the reference recording
#   tools/frontier.sh RECORDING.jsonl OUT.jsonl
#   CONTRA_NATIVE_PLAY_RESUME=tmp/snapshots/core_20000.bin tools/frontier.sh
#
# Guards the two known foot-guns:
#  - the replay tool writes rows to STDOUT (there is NO output-path argv);
#    this script owns the redirection so a stale-trace comparison can't happen
#  - the replay input must be the full re-traced ground truth (the v4/v5
#    mesen trace), NOT a raw recording: raw recordings lack the weapon/NMI-rng
#    fields and silently fall back to the legacy digest
set -e

cd "$(dirname "$0")/.."

RECORDING="${1:-tmp/reference_mesen_v4.jsonl}"
OUT="${2:-tmp/reference_native_latest.jsonl}"

if ! grep -q '"weapon"' "$RECORDING" 2>/dev/null; then
    echo "FAIL: $RECORDING has no \"weapon\" field -- this looks like a raw recording," >&2
    echo "      not a re-traced ground-truth trace (digest would silently downgrade)." >&2
    exit 1
fi

cmake --build build --target contra_play_replay_trace -j8 2>&1 | grep -E "error" && exit 1

CONTRA_NATIVE_PLAY_INPUT=latched ./build/port/contra_play_replay_trace "$RECORDING" > "$OUT"

python3 tools/structural_check.py "$OUT" "$RECORDING"
