# Deploy the WebAssembly Contra build to GitHub Pages.
#
# Builds web/dist (via web/build.ps1) then publishes the runtime files
# (index.html, styles.css, controller.js, dist/*, .nojekyll) to the `gh-pages`
# branch, which GitHub Pages serves at the branch root.
#
# GitHub Pages can't build this itself: the build embeds baserom.nes, which is
# git-ignored and never committed. So we build locally and push the artifacts.
#
#   pwsh web/deploy-pages.ps1                 # build + deploy to `fork` gh-pages
#   pwsh web/deploy-pages.ps1 -SkipBuild      # reuse the existing web/dist
#   pwsh web/deploy-pages.ps1 -Remote origin  # push to a different remote
#
# One-time setup (already done for epoyraz/nes-contra-us): enable Pages with
# source = gh-pages branch, path = /.
#   gh api -X POST repos/<owner>/<repo>/pages -f "source[branch]=gh-pages" -f "source[path]=/"

param(
  [string]$Remote = "fork",
  [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$ScriptDir = $PSScriptRoot
$RepoRoot  = Split-Path -Parent $ScriptDir
$DistDir   = Join-Path $ScriptDir "dist"

# 1. Build (unless told to reuse the existing dist).
if ($SkipBuild) {
  if (-not (Test-Path (Join-Path $DistDir "contra.data"))) {
    throw "web/dist is missing or incomplete; run without -SkipBuild to build it."
  }
  Write-Output "Skipping build; using existing web/dist."
} else {
  Write-Output "Building WebAssembly bundle..."
  & (Join-Path $ScriptDir "build.ps1")
}

# Files that make up the served site.
$RuntimeFiles = @("index.html", "styles.css", "controller.js")
$DistFiles    = @("contra.js", "contra.wasm", "contra.data")
$WorkflowTemplate = Join-Path $ScriptDir "pages-workflow.yml"

foreach ($f in $RuntimeFiles) {
  if (-not (Test-Path (Join-Path $ScriptDir $f))) { throw "Missing web/$f" }
}
foreach ($f in $DistFiles) {
  if (-not (Test-Path (Join-Path $DistDir $f))) { throw "Missing web/dist/$f (build first)" }
}
if (-not (Test-Path $WorkflowTemplate)) { throw "Missing web/pages-workflow.yml" }

# 2. Prepare an isolated worktree for the gh-pages branch so the current
#    checkout is never disturbed.
$Wt = Join-Path ([System.IO.Path]::GetTempPath()) ("contra-gh-pages-" + [System.Guid]::NewGuid().ToString("N").Substring(0, 8))

# Does gh-pages already exist (locally or on the remote)?
git -C $RepoRoot fetch $Remote gh-pages 2>$null
$haveRemote = $LASTEXITCODE -eq 0

try {
  if ($haveRemote) {
    Write-Output "Updating existing $Remote/gh-pages..."
    git -C $RepoRoot worktree add -B gh-pages $Wt "$Remote/gh-pages" | Out-Null
  } else {
    Write-Output "Creating new orphan gh-pages branch..."
    git -C $RepoRoot worktree add --orphan -b gh-pages $Wt | Out-Null
  }

  # 3. Replace the worktree contents with the fresh build.
  Get-ChildItem -Path $Wt -Force |
    Where-Object { $_.Name -ne ".git" } |
    Remove-Item -Recurse -Force

  New-Item -ItemType Directory -Force -Path (Join-Path $Wt "dist") | Out-Null
  foreach ($f in $RuntimeFiles) { Copy-Item (Join-Path $ScriptDir $f) $Wt }
  foreach ($f in $DistFiles)    { Copy-Item (Join-Path $DistDir $f) (Join-Path $Wt "dist") }
  New-Item -ItemType Directory -Force -Path (Join-Path $Wt ".github/workflows") | Out-Null
  Copy-Item $WorkflowTemplate (Join-Path $Wt ".github/workflows/pages.yml")
  # .nojekyll: serve files as-is (no Jekyll processing).
  New-Item -ItemType File -Path (Join-Path $Wt ".nojekyll") -Force | Out-Null

  # 4. Commit (only if something changed) and push.
  git -C $Wt add -A -f | Out-Null
  $pending = git -C $Wt status --porcelain
  if ([string]::IsNullOrWhiteSpace($pending)) {
    Write-Output "gh-pages is already up to date; nothing to deploy."
  } else {
    $srcSha = (git -C $RepoRoot rev-parse --short HEAD).Trim()
    git -C $Wt commit -m "Deploy WebAssembly Contra build (from $srcSha)" | Out-Null
    git -C $Wt push $Remote gh-pages | Out-Null
    Write-Output "Deployed to $Remote/gh-pages."
  }
} finally {
  # 5. Always clean up the worktree.
  git -C $RepoRoot worktree remove $Wt --force 2>$null
}

$url = git -C $RepoRoot remote get-url $Remote 2>$null
if ($url -match "github\.com[:/]([^/]+)/([^/.]+)") {
  Write-Output ("Live at: https://{0}.github.io/{1}/" -f $Matches[1], $Matches[2])
}
