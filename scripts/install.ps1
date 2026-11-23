# Copies the built plugin into X-Plane 12's plugins folder.
# The X-Plane path is read from %LOCALAPPDATA%\x-plane_install_12.txt unless -XPlane is given.
# RealRain.ini is the user's file: the plugin writes a default one on first run and
# this script never touches it.
param(
    [string]$XPlane
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $root "build\dist\RealRain"

if (-not (Test-Path (Join-Path $dist "win_x64\RealRain.xpl"))) {
    throw "Build output not found at $dist. Build first (see README)."
}

if (-not $XPlane) {
    $installFile = Join-Path $env:LOCALAPPDATA "x-plane_install_12.txt"
    if (-not (Test-Path $installFile)) { throw "Pass -XPlane <path to X-Plane 12>." }
    $XPlane = (Get-Content $installFile | Select-Object -First 1).Trim()
}

$target = Join-Path $XPlane "Resources\plugins\RealRain"
New-Item -ItemType Directory -Force (Join-Path $target "win_x64") | Out-Null
Copy-Item (Join-Path $dist "win_x64\RealRain.xpl") (Join-Path $target "win_x64\RealRain.xpl") -Force
Copy-Item (Join-Path $dist "skunkcrafts_updater.cfg") (Join-Path $target "skunkcrafts_updater.cfg") -Force
Write-Host "Installed to $target"
