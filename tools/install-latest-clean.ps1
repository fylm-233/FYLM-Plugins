param([ValidateSet('0.8.2','0.9.0','0.9.1','0.9.2','0.10.0','0.10.1','0.11.0','0.12.0','0.12.1','0.12.2','0.13.0')][string]$Version='0.8.2',[string]$AfterEffects='C:\Program Files\Adobe\Adobe After Effects 2026\Support Files')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$log=Join-Path $root ('docs/install-'+$Version+'.txt')
try{
 & (Join-Path $root ('dist/'+$Version+'/install.ps1')) -AfterEffects $AfterEffects | Out-File -LiteralPath $log -Encoding utf8
 $verification=foreach($name in @('FYLM_PixelArt.aex','FYLM_CRTVHS.aex','FYLM_ShaderGlass.aex')){
  $source=Join-Path $root ('dist/'+$Version+'/'+$name);$installed=Join-Path $AfterEffects ('Plug-ins/FYLM Plugins/'+$name)
  $expected=(Get-FileHash -LiteralPath $source).Hash;$actual=(Get-FileHash -LiteralPath $installed).Hash
  if($expected -ne $actual){throw "Wrong installed build: $name"}
  [pscustomobject]@{name=$name;sha256=$actual;verified=$true}
 }
 $verification | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root ('docs/installed-'+$Version+'-verification.json')) -Encoding utf8
 Add-Content -LiteralPath $log -Value ('FYLM '+$Version+' installed and hashes verified.')
}catch{Add-Content -LiteralPath $log -Value ('ERROR '+$_.Exception.Message);exit 1}



