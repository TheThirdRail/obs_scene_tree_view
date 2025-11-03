[CmdletBinding()]
param(
  [ValidateSet('x64')]
  [string] $Target = 'x64',
  [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
  [string] $Configuration = 'RelWithDebInfo'
)
$ErrorActionPreference = 'Stop'
if ( $DebugPreference -eq 'Continue' ) { $VerbosePreference = 'Continue'; $InformationPreference = 'Continue' }
if ( $env:CI -eq $null ) { throw "Package-Windows.ps1 requires CI environment" }
if ( ! ([System.Environment]::Is64BitOperatingSystem) ) { throw "Packaging script requires a 64-bit system to build and run." }
if ( $PSVersionTable.PSVersion -lt '7.2.0' ) { Write-Warning 'The packaging script requires PowerShell Core 7.'; exit 2 }
function Package {
  trap { Write-Error $_; exit 2 }
  $ScriptHome = $PSScriptRoot
  $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
  $BuildSpecFile = "${ProjectRoot}/buildspec.json"
  $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse
  foreach( $Utility in $UtilityFunctions ) { . $Utility.FullName }
  $BuildSpec = Get-Content -Path ${BuildSpecFile} -Raw | ConvertFrom-Json
  $ProductName = $BuildSpec.name
  $ProductVersion = $BuildSpec.version

  # Assemble Windows obs-studio layout from install tree
  $InstallRoot = "${ProjectRoot}/release/${Configuration}"
  $Staging = "${ProjectRoot}/release/${ProductName}-${ProductVersion}-windows-${Target}"
  Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $Staging
  New-Item -ItemType Directory -Force -Path "${Staging}/obs-studio/obs-plugins/64bit" | Out-Null
  New-Item -ItemType Directory -Force -Path "${Staging}/obs-studio/data/obs-plugins/${ProductName}/locale" | Out-Null

  $dll = Get-ChildItem -Path "${InstallRoot}/lib/obs-plugins" -Filter *.dll | Select-Object -First 1
  if ( $dll -eq $null ) { throw 'No DLL found in install tree. Build must run first.' }
  Copy-Item -Force $dll.FullName "${Staging}/obs-studio/obs-plugins/64bit/${ProductName}.dll"
  $pdb = Get-ChildItem -Path "${InstallRoot}/lib/obs-plugins" -Filter *.pdb | Select-Object -First 1
  if ( $pdb -ne $null ) { Copy-Item -Force $pdb.FullName "${Staging}/obs-studio/obs-plugins/64bit/${ProductName}.pdb" }

  $localePath = "${InstallRoot}/share/obs/obs-plugins/${ProductName}/locale"
  if ( Test-Path $localePath ) { Copy-Item -Recurse -Force "$localePath/*" "${Staging}/obs-studio/data/obs-plugins/${ProductName}/locale/" }

  # Zip
  $ZipPath = "${ProjectRoot}/release/${ProductName}-${ProductVersion}-windows-${Target}.zip"
  if ( Test-Path $ZipPath ) { Remove-Item -Force $ZipPath }
  Compress-Archive -Force -CompressionLevel Optimal -Path (Get-ChildItem -Path $Staging) -DestinationPath $ZipPath
}
Package

