param([Parameter(Mandatory=$true)][string]$AfterEffects)
$ErrorActionPreference='Stop'
$log=Join-Path $PSScriptRoot 'install.log'
try {
 & (Join-Path $PSScriptRoot 'install.ps1') -AfterEffects $AfterEffects | Out-File -LiteralPath $log -Encoding utf8
 Add-Content -LiteralPath $log -Value 'FYLM 0.14.0 installation completed. Restart After Effects.'
 exit 0
} catch {
 $_ | Out-String | Out-File -LiteralPath $log -Append -Encoding utf8
 exit 1
}
