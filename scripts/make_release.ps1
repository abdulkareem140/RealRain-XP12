# Builds the plugin and refreshes the SkunkCrafts Updater depot in release/RealRain.
#
# The depot is what users' updaters download from:
#   https://raw.githubusercontent.com/abdulkareem140/RealRain-XP12/main/release/RealRain
#
# To publish an update:
#   1. Bump the version in CMakeLists.txt  (project(RealRain VERSION x.y.z ...))
#   2. Run this script
#   3. Commit and push release/ along with the source
#
# Files listed in skunkcrafts_updater_blacklist.txt (the user's RealRain.ini) are never shipped.
param(
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$root  = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root "build"
$dist  = Join-Path $build "dist\RealRain"
$depot = Join-Path $root "release\RealRain"

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) {
    $cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
}
& $cmake -S $root -B $build -G "Visual Studio 17 2022" -A x64 | Out-Null
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
& $cmake --build $build --config $Config
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

# Standard CRC-32 (IEEE), printed as an unsigned decimal like SkunkCrafts expects.
if (-not ("RealRainCrc32" -as [type])) {
    Add-Type -TypeDefinition @'
public static class RealRainCrc32 {
    static readonly uint[] T = Make();
    static uint[] Make() {
        var t = new uint[256];
        for (uint i = 0; i < 256; i++) {
            uint c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
        return t;
    }
    public static uint Compute(byte[] data) {
        uint c = 0xFFFFFFFFu;
        foreach (byte b in data) c = T[(c ^ b) & 0xFF] ^ (c >> 8);
        return c ^ 0xFFFFFFFFu;
    }
}
'@
}
if ([RealRainCrc32]::Compute([Text.Encoding]::ASCII.GetBytes("123456789")) -ne 3421780262) {
    throw "CRC32 self-test failed."
}

# Files shipped to users, relative to the plugin folder (forward slashes).
$shipped = @("win_x64/RealRain.xpl")
$blacklist = @("RealRain.ini")

if (Test-Path $depot) { Remove-Item -Recurse -Force $depot }
New-Item -ItemType Directory -Force $depot | Out-Null

$white = New-Object System.Collections.Generic.List[string]
$sizes = New-Object System.Collections.Generic.List[string]
foreach ($rel in $shipped) {
    $src = Join-Path $dist ($rel -replace '/', '\')
    if (-not (Test-Path $src)) { throw "Missing build output: $src" }
    $dst = Join-Path $depot ($rel -replace '/', '\')
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    Copy-Item $src $dst -Force
    $bytes = [IO.File]::ReadAllBytes($dst)
    $white.Add("$rel|$([RealRainCrc32]::Compute($bytes))")
    $sizes.Add("$rel|$($bytes.Length)")
}

$utf8 = New-Object System.Text.UTF8Encoding($false)
function Write-Lf([string]$path, [string[]]$lines) {
    [IO.File]::WriteAllText($path, (($lines -join "`n") + "`n"), $utf8)
}
Copy-Item (Join-Path $dist "skunkcrafts_updater.cfg") (Join-Path $depot "skunkcrafts_updater.cfg") -Force
Write-Lf (Join-Path $depot "skunkcrafts_updater_whitelist.txt") $white
Write-Lf (Join-Path $depot "skunkcrafts_updater_sizeslist.txt") $sizes
Write-Lf (Join-Path $depot "skunkcrafts_updater_blacklist.txt") $blacklist

Write-Host "Depot updated: $depot"
Get-Content (Join-Path $depot "skunkcrafts_updater.cfg")
Get-Content (Join-Path $depot "skunkcrafts_updater_whitelist.txt")
