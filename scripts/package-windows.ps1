param(
  [string]$BuildDir = "build",
  [string]$OutDir = "dist"
)

$ErrorActionPreference = 'Stop'

# Staging layout (system-level, unzip into C:\Program Files)
$stage = Join-Path $OutDir 'obs-scene-tree-view-windows-x64'
$binDir = Join-Path $stage 'obs-studio/obs-plugins/64bit'
$locDir = Join-Path $stage 'obs-studio/data/obs-plugins/obs_scene_tree_view/locale'

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $binDir | Out-Null
New-Item -ItemType Directory -Force -Path $locDir | Out-Null

# Locate built DLL & PDB
$dll = Get-ChildItem -Path $BuildDir -Recurse -Filter 'obs_scene_tree_view.dll' | Select-Object -First 1
$pdb = Get-ChildItem -Path $BuildDir -Recurse -Filter 'obs_scene_tree_view.pdb' | Select-Object -First 1
if (-not $dll) { throw "obs_scene_tree_view.dll not found under $BuildDir" }
if (-not $pdb) { Write-Warning "obs_scene_tree_view.pdb not found under $BuildDir (continuing without PDB)" }

Copy-Item $dll.FullName $binDir -Force
if ($pdb) { Copy-Item $pdb.FullName $binDir -Force }

# Locales
Copy-Item 'data/locale/*.ini' $locDir -Force

# INSTALL instructions
$installTxt = @"
Installation Instructions for Windows (System-Level)
===================================================

1) Close OBS Studio completely (ensure obs64.exe is not running).
2) Extract this entire archive into C:\Program Files
   - You should end up with:
     C:\Program Files\obs-studio\obs-plugins\64bit\obs_scene_tree_view.dll
     C:\Program Files\obs-studio\obs-plugins\64bit\obs_scene_tree_view.pdb (optional)
     C:\Program Files\obs-studio\data\obs-plugins\obs_scene_tree_view\locale\*.ini
3) Start OBS Studio.
4) Enable the dock via: View -> Docks -> Scene Tree View (Reset UI if needed).

Notes:
- This is a system-level install and may require Administrator privileges.
- OBS 32.x is required.
"@
$installPath = Join-Path $stage 'INSTALL.txt'
Set-Content -Path $installPath -Value $installTxt -Encoding UTF8

# Create ZIP
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$zipPath = Join-Path $OutDir 'obs-scene-tree-view-windows-x64.zip'
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zipPath -Force

Write-Host "Packaged: $zipPath"
