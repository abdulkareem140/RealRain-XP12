# Downloads the X-Plane plugin SDK into third_party/XPSDK.
# Check https://developer.x-plane.com/sdk/plugin-sdk-downloads/ for the current
# zip and pass it with -Url if the default below is out of date.
param(
    [string]$Url = "https://developer.x-plane.com/wp-content/plugins/code-sample-generation/sdk_zip_files/XPSDK411.zip"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $root "third_party\XPSDK"
$tmp  = Join-Path ([IO.Path]::GetTempPath()) ("xpsdk_" + [guid]::NewGuid().ToString("N"))

New-Item -ItemType Directory -Force $tmp | Out-Null
$zip = Join-Path $tmp "sdk.zip"
Write-Host "Downloading $Url"
Invoke-WebRequest -Uri $Url -OutFile $zip -UseBasicParsing
Expand-Archive -Path $zip -DestinationPath (Join-Path $tmp "x") -Force

# The zip normally contains a top-level SDK/ folder; find whatever holds CHeaders.
$headers = Get-ChildItem (Join-Path $tmp "x") -Recurse -Directory -Filter CHeaders | Select-Object -First 1
if (-not $headers) { throw "CHeaders folder not found in the downloaded zip." }

if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
New-Item -ItemType Directory -Force (Split-Path $dest) | Out-Null
Move-Item $headers.Parent.FullName $dest
Remove-Item -Recurse -Force $tmp
Write-Host "SDK installed to $dest"
