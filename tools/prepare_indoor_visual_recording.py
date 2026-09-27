#!/usr/bin/env python3
"""Expand the recorded controller/RNG bytes for the indoor visual checkpoint.

The fixture contains 9,326 frames from the Mesen play recording used for
frame 9,321. Each frame stores six bytes: raw P1/P2 input, latched P1/P2 input,
RNG, and frame counter. No ROM graphics or framebuffer pixels are stored.
"""

import base64
import hashlib
import json
from pathlib import Path
import sys
import zlib

FRAME_COUNT = 9326
ROW_SIZE = 6
SHA256 = "0e7c54f740cb28c77cb732e20b9a4c6c71cf20152e854ce249911a4d35480c39"
FIXTURE = Path(__file__).resolve().parent / "fixtures" / "indoor_9321_inputs.b64"


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} OUTPUT.jsonl")
    raw = zlib.decompress(base64.b64decode(FIXTURE.read_text()))
    if len(raw) != FRAME_COUNT * ROW_SIZE or hashlib.sha256(raw).hexdigest() != SHA256:
        raise SystemExit("indoor visual input fixture failed integrity check")

    output = Path(sys.argv[1])
    with output.open("w") as handle:
        for frame in range(1, FRAME_COUNT + 1):
            p1_raw, p2_raw, controller, p2_controller, rng, frame_counter = (
                raw[(frame - 1) * ROW_SIZE:frame * ROW_SIZE]
            )
            handle.write(json.dumps({
                "frame": frame,
                "p1_raw": p1_raw,
                "p2_raw": p2_raw,
                "controller": controller,
                "p2_controller": p2_controller,
                "rng": rng,
                "frame_counter": frame_counter,
            }, separators=(",", ":")) + "\n")


if __name__ == "__main__":
    main()
