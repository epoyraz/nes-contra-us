# Contra (NES) — native C port

A port of the NES game _Contra_ (US) from 6502 assembly to C, and from C to
WebAssembly. It is built on [vermiceli](https://github.com/vermiceli/nes-contra-us)'s
annotated disassembly (itself started from Trax's work); the original assembly
and docs still live in `src/` and `docs/`.

**Play in the browser:** https://epoyraz.github.io/nes-contra-us/ — you supply
your own `Contra (US)` ROM (it is never uploaded or shipped). Arrows move, `A`
fires, `S`/`Space` jumps, `Enter` starts. There is an online 2‑player mode
(lockstep over WebRTC) and a speaker button to mute sound.

The port is checked against the real game frame by frame: a Mesen 2 recording is
replayed through the C core and every frame's RAM, nametables, palette, OAM,
CHR, PPU registers, sound-engine state and final pixels are compared
(`port/tools/parity_telemetry.c`). Stages 1–4 are identical over the whole
40k-frame recording, including sound. The sound engine is a line-by-line
translation of bank 1 feeding a 2A03 APU model.

**Build:** put your ROM at `baserom.nes` (MD5 `7BDAD8B4A7A56A634C9649D20BD3011B`),
then `cmake -B build && cmake --build build` for the SDL desktop build
and tests (`ctest` in `build/`), or `web/build.sh` with Emscripten for the web
version. `./build.sh` still reassembles the original ROM byte-for-byte with cc65.

No ROM data is included in this repository. _Contra_ is © Konami.
