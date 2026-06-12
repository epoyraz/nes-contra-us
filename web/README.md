# Contra in the browser (WebAssembly)

This builds the faithful `contra_core` native port (the same C library the SDL
and headless hosts use) to WebAssembly with [Emscripten](https://emscripten.org),
and plays it in the browser. The on-screen gamepad is the
[CodePen NES controller by epzilla](https://codepen.io/epzilla/pen/AqweZk),
wired up so its D-pad / A / B / Start / Select actually drive the game (mouse &
touch), alongside keyboard input.

Inspired by [pokeemerald-wasm](https://github.com/tripplyons/pokeemerald-wasm/).

## How it fits together

```
port/contra_core/      pure-C game core (init / set_input / step_frame / framebuffer)
port/platform_web/main.c   thin Emscripten host: exports a small C ABI to JS
web/index.html         page shell: the "TV" canvas + the NES controller markup
web/styles.css         layout + the CodePen controller (SCSS compiled to plain CSS)
web/controller.js      boots the wasm module, 60 Hz loop, keyboard + on-screen input
web/multiplayer-server.mjs  optional dependency-free WebSocket lobby/relay
web/build.ps1 / .sh    one-command Emscripten build
web/dist/              build output (git-ignored): contra.js / .wasm / .data
```

The core renders into a 256×240 framebuffer in memory; `main.c` repacks it to
RGBA each frame and the JS loop copies it straight from wasm memory onto the
canvas with `putImageData`. Input flows the other way: JS computes the NES
button bitmask and calls the exported `contra_web_set_input` /
`contra_web_set_inputs`.

`baserom.nes` (CHR / graphics data the core reads at startup) is **embedded into
the build** via Emscripten's `--preload-file`; it is not committed (it stays
git-ignored). You need it at the repo root to build.

## Build

Prerequisite: the Emscripten SDK installed and either on `PATH` or discoverable
via `$EMSDK` (default lookup is `~/emsdk`).

```powershell
# Windows / PowerShell
pwsh web/build.ps1
```

```bash
# Linux / macOS / Git Bash
./web/build.sh
```

Add `-Debug` / `--debug` for an `-O0` build with assertions.

## Run

Serve `web/` over HTTP (the `.wasm`/`.data` won't load from `file://`):

```bash
python -m http.server -d web 8000
```

Then open <http://localhost:8000/>.

### Controls

| Action | Keyboard | On-screen |
| ------ | -------- | --------- |
| Move   | Arrow keys | D-pad |
| Jump   | `A` / `Space` | A |
| Fire   | `S` | B |
| Start  | `Enter` | START |
| Select | `Shift` | SELECT |

## Deploy to GitHub Pages

`baserom.nes` is embedded into the build and is git-ignored, so GitHub Pages
can't build this in CI. Instead, build locally and publish the artifacts to the
`gh-pages` branch (served at the branch root):

```powershell
pwsh web/deploy-pages.ps1               # build + publish to the `fork` remote
pwsh web/deploy-pages.ps1 -SkipBuild    # reuse the existing web/dist
pwsh web/deploy-pages.ps1 -Remote origin
```

It builds (`web/build.ps1`), then commits `index.html` / `styles.css` /
`controller.js` / `dist/*` / `.nojekyll` to `gh-pages` via a throwaway worktree
and pushes. One-time setup: enable Pages with source = `gh-pages`, path = `/`.

Debug warps: open `index.html#level2boss` or `index.html#level4`.
And yes — the controller still hides the original pen's Konami-code easter egg. 🙂

## Online co-op

The browser build supports two-browser co-op through deterministic input
lockstep. Both browsers run the same WASM core locally; the server only relays
per-frame input bytes and periodic state hashes.

For local testing, run the included WebSocket relay:

```bash
node web/multiplayer-server.mjs
```

Then open <http://localhost:8787/> in two browser windows:

1. Player 1 clicks **Create Room** and shares the room code.
2. Player 2 enters the code and clicks **Join Room**.
3. Both players click **Ready**.
4. Player 1 selects 2-player mode with **SELECT** / <kbd>Shift</kbd>, then starts.

GitHub Pages can host the static client, but it cannot run the WebSocket relay.
For a deployed relay, host `web/multiplayer-server.mjs` on a Node-capable
service and open the Pages build with `?ws=wss://your-relay.example/ws`.
