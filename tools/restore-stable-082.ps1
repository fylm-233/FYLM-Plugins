param([string]$AfterEffects='C:\Program Files\Adobe\Adobe After Effects 2026\Support Files')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$log=Join-Path $root 'docs/restore-0.8.2-after-0.9.txt'
try{
 & (Join-Path $root 'dist/0.8.2/install.ps1') -AfterEffects $AfterEffects | Out-File -LiteralPath $log -Encoding utf8
 $verification=foreach($name in @('FYLM_PixelArt.aex','FYLM_CRTVHS.aex','FYLM_ShaderGlass.aex')){
  $source=Join-Path $root ('dist/0.8.2/'+$name);$installed=Join-Path $AfterEffects ('Plug-ins/FYLM Plugins/'+$name)
  $expected=(Get-FileHash -LiteralPath $source).Hash;$actual=(Get-FileHash -LiteralPath $installed).Hash
  if($expected -ne $actual){throw "Wrong installed build: $name"}
  [pscustomobject]@{name=$name;sha256=$actual;verified=$true}
 }
 $verification | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'docs/restored-0.8.2-after-0.9.json') -Encoding utf8
 Add-Content -LiteralPath $log -Value 'FYLM 0.8.2 installed and hashes verified.'
}catch{Add-Content -LiteralPath $log -Value ('ERROR '+$_.Exception.Message);exit 1}

