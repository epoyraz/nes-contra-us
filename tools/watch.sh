#!/bin/sh
# Log every ROM write to RAM addresses between two frames of a recording.
#
#   tools/watch.sh ADDRS FROM TO [RECORDING.jsonl] [EXEC_PCS]
#   tools/watch.sh 8C-8D,83 940 945
#
# Prints "frame F sl S dot D W $addr=val pc=$pc a= x= y= bank=" lines
# (see tools/mesen_watch.lua).
set -e
cd "$(dirname "$0")/.."
ROOT="$(pwd)"
MESEN="${CONTRA_MESEN_EXECUTABLE:-${CONTRA_MESEN_APP:-$HOME/Applications/Mesen.app}/Contents/MacOS/Mesen}"
[ $# -ge 3 ] || { echo "usage: tools/watch.sh ADDRS FROM TO [RECORDING.jsonl] [EXEC_PCS]" >&2; exit 2; }
REC="${4:-tmp/reference_recording_v2.jsonl}"
case "$REC" in /*) ;; *) REC="$ROOT/$REC" ;; esac
OUT="$(mktemp "${TMPDIR:-/tmp}/contra_watch.XXXXXX")"
CONTRA_WATCH_REPLAY_JSONL="$REC" CONTRA_WATCH_OUT="$OUT" CONTRA_WATCH_ADDRS="$1" \
CONTRA_WATCH_FROM="$2" CONTRA_WATCH_TO="$3" CONTRA_WATCH_EXEC="$5" \
"$MESEN" --testRunner --doNotSaveSettings --timeout=1200 "$ROOT/baserom.nes" "$ROOT/tools/mesen_watch.lua" > /dev/null 2>&1 || true
cat "$OUT"
rm -f "$OUT"
