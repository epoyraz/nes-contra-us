#!/usr/bin/env python3
"""Replay one recorded indoor frame in Mesen and the native port, then diff it."""

import os
from pathlib import Path
import subprocess
import sys


def run(command, env):
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    if result.returncode:
        print(result.stdout, end="")
        print(result.stderr, end="", file=sys.stderr)
        raise SystemExit(result.returncode)


def main():
    if len(sys.argv) != 9:
        raise SystemExit(
            "usage: indoor_visual_check.py MESEN NATIVE ROM MESEN_RECORDING "
            "NATIVE_RECORDING FRAME MAX_MISMATCHES OUTPUT_DIR"
        )
    mesen, native, rom, mesen_recording, native_recording = sys.argv[1:6]
    frame = int(sys.argv[6])
    maximum = int(sys.argv[7])
    output = Path(sys.argv[8])
    output.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve().parent

    mesen_env = os.environ.copy()
    mesen_env.update({
        "CONTRA_MESEN_PLAY_REPLAY_JSONL": mesen_recording,
        "CONTRA_MESEN_PLAY_RECORDING_JSONL": str(output / "mesen_rerun.jsonl"),
        "CONTRA_MESEN_PLAY_DUMP_FRAMES": str(frame),
        "CONTRA_MESEN_PLAY_FRAMEBUFFER_DUMP_PATH": str(output / "mesen_fb.bin"),
        "CONTRA_MESEN_PLAY_MAX_FRAME": str(frame + 5),
    })
    run([mesen, "--testRunner", "--doNotSaveSettings", "--timeout=900",
         "--debug.scriptWindow.allowIoOsAccess=true", rom,
         str(source / "mesen_play_recorder.lua")], mesen_env)

    native_env = os.environ.copy()
    native_env.update({
        "CONTRA_NATIVE_PLAY_INPUT": "latched",
        "CONTRA_NATIVE_PLAY_DUMP_FRAMES": str(frame),
        "CONTRA_NATIVE_PLAY_FRAMEBUFFER_DUMP_PATH": str(output / "native_fb.bin"),
        "CONTRA_NATIVE_PLAY_MAX_FRAME": str(frame + 5),
    })
    run([native, native_recording], native_env)

    result = subprocess.run([
        sys.executable, str(source / "render_diff.py"),
        str(output / f"mesen_fb.bin.{frame}"),
        str(output / f"native_fb.bin.{frame}"),
        str(output / str(frame)), str(maximum),
    ], capture_output=True, text=True)
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
