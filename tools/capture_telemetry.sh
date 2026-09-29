#!/bin/sh
# Capture exhaustive ROM ground truth for a play recording (headless Mesen).
#
#   tools/capture_telemetry.sh RECORDING.jsonl OUT.ctel [MAX_FRAME]
#
# RECORDING is any recorder JSONL with per-frame p1_raw/p2_raw (the raw
# recording, e.g. tmp/reference_recording_v2.jsonl). The Lua capture streams
# one record per frame through a FIFO into `tools/ctel.py sink`, which
# compresses it into OUT.ctel (~10 KB/frame). Requires baserom.nes and Mesen at
# $CONTRA_MESEN_APP (default ~/Applications/Mesen.app).
set -e

cd "$(dirname "$0")/.."
ROOT="$(pwd)"
MESEN="${CONTRA_MESEN_EXECUTABLE:-${CONTRA_MESEN_APP:-$HOME/Applications/Mesen.app}/Contents/MacOS/Mesen}"
ROM="${CONTRA_ROM:-$ROOT/baserom.nes}"

[ $# -ge 2 ] || { echo "usage: tools/capture_telemetry.sh RECORDING.jsonl OUT.ctel [MAX_FRAME]" >&2; exit 2; }

abspath() {
    case "$1" in
        /*) printf '%s\n' "$1" ;;
        *) printf '%s\n' "$ROOT/$1" ;;
    esac
}

RECORDING="$(abspath "$1")"
OUT="$(abspath "$2")"
MAX_FRAME="${3:-0}"
FIFO="$(mktemp -u "${TMPDIR:-/tmp}/contra_tel.XXXXXX")"

mkfifo "$FIFO"
trap 'rm -f "$FIFO"' EXIT

python3 "$ROOT/tools/ctel.py" sink "$FIFO" "$OUT" &
SINK=$!

CONTRA_TEL_REPLAY_JSONL="$RECORDING" \
CONTRA_TEL_OUT="$FIFO" \
CONTRA_TEL_MAX_FRAME="$MAX_FRAME" \
"$MESEN" --testRunner --doNotSaveSettings --timeout="${CONTRA_TEL_TIMEOUT:-3600}" \
    "$ROM" "$ROOT/tools/mesen_telemetry.lua" > /dev/null 2>&1 || {
    echo "FAIL: Mesen exited with $?" >&2
    kill $SINK 2>/dev/null
    exit 1
}

wait $SINK
python3 "$ROOT/tools/ctel.py" info "$OUT"
