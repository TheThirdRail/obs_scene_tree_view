param([string]$Path)
if (-not (Test-Path -Path $Path)) {
  New-Item -ItemType Directory -Force -Path $Path | Out-Null
}
Set-Location -Path $Path

