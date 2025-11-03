[CmdletBinding()]
param(
  [ValidateSet('x64')]
  [string] $Target = 'x64',
  [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
  [string] $Configuration = 'RelWithDebInfo'
)
$ErrorActionPreference = 'Stop'
if ( $DebugPreference -eq 'Continue' ) { $VerbosePreference = 'Continue'; $InformationPreference = 'Continue' }
if ( $env:CI -eq $null ) { throw "Build-Windows.ps1 requires CI environment" }
if ( ! ([System.Environment]::Is64BitOperatingSystem) ) { throw "A 64-bit system is required to build the project." }
if ( $PSVersionTable.PSVersion -lt '7.2.0' ) { Write-Warning 'The obs-studio PowerShell build script requires PowerShell Core 7.'; exit 2 }
function Build {
  trap { Pop-Location -Stack BuildTemp -ErrorAction 'SilentlyContinue'; Write-Error $_; exit 2 }
  $ScriptHome = $PSScriptRoot
  $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
  $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse
  foreach($Utility in $UtilityFunctions) { . $Utility.FullName }

  Push-Location -Stack BuildTemp
  Set-Location $ProjectRoot

  $CmakeArgs = @('--preset', "windows-ci-${Target}")
  $CmakeBuildArgs = @('--build', '--preset', "windows-${Target}", '--config', $Configuration, '--parallel', '--', '/consoleLoggerParameters:Summary', '/noLogo')
  $CmakeInstallArgs = @('--install', "build_${Target}", '--prefix', "${ProjectRoot}/release/${Configuration}", '--config', $Configuration)

  Log-Group "Configuring ${ProductName}..."
  Invoke-External cmake @CmakeArgs
  Log-Group "Building ${ProductName}..."
  Invoke-External cmake @CmakeBuildArgs
  Log-Group "Installing ${ProductName}..."
  Invoke-External cmake @CmakeInstallArgs

  Pop-Location -Stack BuildTemp
}
Build

