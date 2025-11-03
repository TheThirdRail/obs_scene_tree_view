param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Args)
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $Args[0]
$psi.Arguments = [string]::Join(' ', $Args[1..($Args.Length-1)])
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.UseShellExecute = $false
$proc = New-Object System.Diagnostics.Process
$proc.StartInfo = $psi
$proc.Start() | Out-Null
$stdout = $proc.StandardOutput.ReadToEnd()
$stderr = $proc.StandardError.ReadToEnd()
$proc.WaitForExit()
if ($proc.ExitCode -ne 0) {
  Write-Error $stderr
  throw "External process failed: $($Args[0])"
}
Write-Output $stdout

