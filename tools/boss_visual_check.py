#!/usr/bin/env python3
"""Compare the settled Level 2 boss arena against the original ROM."""
import os
from pathlib import Path
import subprocess
import sys

from indoor_visual_check import run


def main():
    if len(sys.argv) != 5:
        raise SystemExit("usage: boss_visual_check.py MESEN NATIVE ROM OUTPUT_DIR")
    mesen, native, rom, directory = sys.argv[1:]
    output = Path(directory).resolve()
    output.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve().parent
    # Remove prior captures so an incomplete run cannot reuse stale evidence.
    for name in ("mesen.bin", "native.bin"):
        (output / name).unlink(missing_ok=True)
    env = os.environ.copy()
    env.update({
        "CONTRA_MESEN_TRACE_JSONL": str(output / "mesen.jsonl"),
        "CONTRA_MESEN_BOSS_FRAMEBUFFER_DUMP_PATH": str(output / "mesen.bin"),
    })
    run([mesen, "--testRunner", "--doNotSaveSettings", "--timeout=900",
         "--debug.scriptWindow.allowIoOsAccess=true", rom,
         str(source / "mesen_checkpoint_trace.lua")], env)
    run([native, str(output / "native.bin")], os.environ.copy())
    # Player/HUD/projectile timing differs between the seeded routes. The
    # 3,000-pixel budget catches the original 21,498-pixel placement error.
    return subprocess.run([
        sys.executable, str(source / "render_diff.py"),
        str(output / "mesen.bin"), str(output / "native.bin"),
        str(output / "boss"), "3000",
    ]).returncode


if __name__ == "__main__":
    sys.exit(main())
