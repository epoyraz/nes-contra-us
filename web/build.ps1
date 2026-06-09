# Build the Contra WebAssembly port with Emscripten.
#
# Requires the Emscripten SDK. By default this looks for it at $env:EMSDK, then
# at ~/emsdk. Output lands in web/dist/ (contra.js, contra.wasm, contra.data).
#
#   pwsh web/build.ps1            # release (-O2)
#   pwsh web/build.ps1 -Debug     # -O0 + assertions for troubleshooting

param(
  [switch]$Debug
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$EmsdkDir = if ($env:EMSDK) { $env:EMSDK } else { Join-Path $HOME "emsdk" }

$envScript = Join-Path $EmsdkDir "emsdk_env.ps1"
if (-not (Test-Path $envScript)) {
  throw "Emscripten SDK not found at '$EmsdkDir'. Install it (https://emscripten.org/docs/getting_started/downloads.html) or set `$env:EMSDK."
}
& $envScript *> $null

$Rom = Join-Path $RepoRoot "baserom.nes"
if (-not (Test-Path $Rom)) {
  throw "baserom.nes not found at '$Rom'. It supplies CHR/graphics and is embedded into the build."
}

$DistDir = Join-Path $PSScriptRoot "dist"
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

$Opt = if ($Debug) { @("-O0", "-sASSERTIONS=2", "-gsource-map") } else { @("-O2") }

$Args = @(
  (Join-Path $RepoRoot "port\contra_core\src\core.c"),
  (Join-Path $RepoRoot "port\platform_web\main.c"),
  "-I", (Join-Path $RepoRoot "port\contra_core\include")
) + $Opt + @(
  "-sMODULARIZE=1",
  "-sEXPORT_NAME=ContraModule",
  "-sALLOW_MEMORY_GROWTH=1",
  "-sEXIT_RUNTIME=0",
  "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,HEAPU8",
  "-sEXPORTED_FUNCTIONS=_main,_contra_web_init,_contra_web_reset,_contra_web_set_input,_contra_web_step,_contra_web_framebuffer,_contra_web_width,_contra_web_height,_contra_web_warp_level2_boss,_contra_web_warp_level4",
  "--preload-file", "$Rom@baserom.nes",
  "-o", (Join-Path $DistDir "contra.js")
)

Write-Output "emcc $($Args -join ' ')"
& emcc @Args

Write-Output "Build complete -> $DistDir (contra.js, contra.wasm, contra.data)"
Write-Output "Serve it:  python -m http.server -d `"$PSScriptRoot`" 8000   then open http://localhost:8000/"
