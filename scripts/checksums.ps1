<#
.SYNOPSIS
  Generate a SHA256 checksum file for a given artifact (Windows).

.DESCRIPTION
  Computes a SHA256 hash for the specified file using Get-FileHash (preferred)
  or CertUtil (fallback). Writes a companion .sha256 file next to the input file
  in the standard format used by most distributions:

    <lowercase-hash>  <filename>

.PARAMETER Path
  Path to the file to hash (e.g., the release ZIP). Can be relative or absolute.

.EXAMPLE
  PS> .\scripts\checksums.ps1 -Path .\scene-tree-view-windows-x64.zip
  Wrote C:\repo\scene-tree-view-windows-x64.zip.sha256
  1f9b...  scene-tree-view-windows-x64.zip

.NOTES
  - Intended for Windows release packaging for Scene Tree View.
  - Keep archives at the repo root or pass an explicit path.
#>

[CmdletBinding()]
param(
  [Parameter(Mandatory = $true, Position = 0)]
  [string]$Path
)

$ErrorActionPreference = 'Stop'

try {
  $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
} catch {
  Write-Error "File not found: $Path"
  exit 1
}

$file = Get-Item -LiteralPath $resolved
if (-not $file -or -not $file.Exists) {
  Write-Error "File not found: $Path"
  exit 1
}

# Compute SHA256 (prefer Get-FileHash)
$hash = $null
try {
  $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLower()
} catch {
  # Fallback to CertUtil if Get-FileHash is unavailable
  $certOut = & certutil -hashfile $file.FullName SHA256 2>$null
  if ($LASTEXITCODE -ne 0 -or -not $certOut) {
    Write-Error "Failed to compute SHA256 via Get-FileHash and CertUtil."
    exit 1
  }
  $lines = $certOut -split "`r?`n" | Where-Object { $_ -match '\\S' }
  if ($lines.Count -lt 2) {
    Write-Error "Unexpected CertUtil output."
    exit 1
  }
  $hash = $lines[1].Trim().ToLower()
}

$dir = Split-Path -Parent $file.FullName
$baseName = $file.Name
$outPath = Join-Path $dir ($baseName + '.sha256')
$content = "$hash  $baseName"

# Write checksum file using ASCII to avoid BOM differences.
Set-Content -LiteralPath $outPath -Value $content -NoNewline:$false -Encoding Ascii

Write-Host "Wrote $outPath"
Write-Host $content

