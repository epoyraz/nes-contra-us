#!/usr/bin/env bash
#
# Build the Contra WebAssembly port with Emscripten.
#
# Requires the Emscripten SDK. By default this looks for it at $EMSDK, then at
# ~/emsdk. Output lands in web/dist/ (contra.js, contra.wasm, contra.data).
#
#   ./web/build.sh           # release (-O2)
#   ./web/build.sh --debug   # -O0 + assertions for troubleshooting

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
EMSDK_DIR="${EMSDK:-$HOME/emsdk}"

if [ -f "$EMSDK_DIR/emsdk_env.sh" ]; then
  # shellcheck disable=SC1091
  source "$EMSDK_DIR/emsdk_env.sh" >/dev/null 2>&1
fi
if ! command -v emcc >/dev/null 2>&1; then
  echo "emcc not found. Install the Emscripten SDK and/or set \$EMSDK." >&2
  echo "  https://emscripten.org/docs/getting_started/downloads.html" >&2
  exit 1
fi

ROM="$REPO_ROOT/baserom.nes"
if [ ! -f "$ROM" ]; then
  echo "baserom.nes not found at $ROM (supplies CHR/graphics; embedded into the build)." >&2
  exit 1
fi

DIST="$SCRIPT_DIR/dist"
mkdir -p "$DIST"

OPT=(-O2)
if [ "${1:-}" = "--debug" ]; then
  OPT=(-O0 -sASSERTIONS=2 -gsource-map)
fi

emcc \
  "$REPO_ROOT/port/contra_core/src/core.c" \
  "$REPO_ROOT/port/platform_web/main.c" \
  -I "$REPO_ROOT/port/contra_core/include" \
  "${OPT[@]}" \
  -sMODULARIZE=1 \
  -sEXPORT_NAME=ContraModule \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXIT_RUNTIME=0 \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,HEAPU8 \
  -sEXPORTED_FUNCTIONS=_main,_contra_web_init,_contra_web_reset,_contra_web_set_input,_contra_web_set_inputs,_contra_web_state_hash,_contra_web_step,_contra_web_framebuffer,_contra_web_width,_contra_web_height,_contra_web_warp_level2_boss,_contra_web_warp_level4 \
  --preload-file "$ROM@baserom.nes" \
  -o "$DIST/contra.js"

echo "Build complete -> $DIST (contra.js, contra.wasm, contra.data)"
echo "Serve it:  python -m http.server -d \"$SCRIPT_DIR\" 8000   then open http://localhost:8000/"
