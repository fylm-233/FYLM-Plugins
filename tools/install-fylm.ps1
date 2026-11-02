param([string]$AfterEffects='C:\Program Files\Adobe\Adobe After Effects 2026\Support Files')
$ErrorActionPreference='Stop'
if(Get-Process AfterFX*,aerender* -ErrorAction SilentlyContinue){throw 'Close After Effects and aerender before installing.'}
$package=[IO.Path]::GetFullPath($PSScriptRoot)
$support=[IO.Path]::GetFullPath($AfterEffects)
$manifest=Join-Path $package 'SHA256SUMS.json'
if(!(Test-Path -LiteralPath $manifest)){throw 'Extract the complete FYLM Plugins package before installing.'}
foreach($entry in (Get-Content -LiteralPath $manifest -Raw -Encoding UTF8 | ConvertFrom-Json)){
 $path=[IO.Path]::GetFullPath((Join-Path $package $entry.path))
 if(!$path.StartsWith($package+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Invalid manifest path'}
 if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.sha256){throw "Package hash mismatch: $($entry.path)"}
}
$backup=Join-Path $env:LOCALAPPDATA ('FYLM\backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$pluginsRoot=Join-Path $support 'Plug-ins'
$target=Join-Path $pluginsRoot 'FYLM Plugins'
$panel=Join-Path $support 'Scripts\ScriptUI Panels'
$ops=[Collections.Generic.List[object]]::new()
function Add-Operation([string]$source,[string]$destination){
 $destination=[IO.Path]::GetFullPath($destination)
 if(!$destination.StartsWith($support+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Target outside AE support directory'}
 if(Test-Path -LiteralPath $destination -PathType Container){throw "File target is a directory: $destination"}
 $parent=Split-Path $destination -Parent
 while($parent.Length -ge $support.Length){if(Test-Path -LiteralPath $parent -PathType Leaf){throw "Parent path is a file: $parent"};$parent=Split-Path $parent -Parent;if(!$parent){break}}
 $ops.Add([pscustomobject]@{source=$source;destination=$destination;existed=(Test-Path -LiteralPath $destination -PathType Leaf);saved='';stage='';changed=$false})
}
foreach($name in @('FYLM_PixelArt.aex','FYLM_CRTVHS.aex','FYLM_ShaderGlass.aex','FYLM-sgtool.exe','FYLM-sgprobe.exe')){Add-Operation (Join-Path $package $name) (Join-Path $target $name)}
foreach($name in @('FYLM Plugins.jsx','FYLM-i18n.jsxinc','FYLM_DitheringStudio.jsxinc','FYLM_CRTVHS.jsxinc','FYLM_ShaderGlass.jsxinc')){Add-Operation (Join-Path $package ('ScriptUI Panels\'+$name)) (Join-Path $panel $name)}
$assetRoot=Join-Path $package 'Assets'
foreach($file in (Get-ChildItem -LiteralPath $assetRoot -File -Recurse)){Add-Operation $file.FullName (Join-Path (Join-Path $target 'Assets') ($file.FullName.Substring($assetRoot.Length).TrimStart('\')))}
foreach($file in @(Get-ChildItem -LiteralPath $pluginsRoot -Recurse -File -Filter 'FYLM_DitheringStudio.aex' -ErrorAction SilentlyContinue)){Add-Operation '' $file.FullName}
# Prepare every file and back up every existing destination before changing AE files.
for($i=0;$i -lt $ops.Count;$i++){
 $op=$ops[$i];$op.saved=Join-Path $backup ($i.ToString()+'.previous');$op.stage=Join-Path $backup ($i.ToString()+'.stage')
 if($op.existed){Copy-Item -LiteralPath $op.destination -Destination $op.saved -Force}
 if($op.source){Copy-Item -LiteralPath $op.source -Destination $op.stage -Force;if((Get-FileHash -LiteralPath $op.source).Hash -ne (Get-FileHash -LiteralPath $op.stage).Hash){throw 'Staging hash mismatch'}}
}
$ops | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'journal.json') -Encoding utf8
$created=[Collections.Generic.List[string]]::new()
try{
 foreach($op in $ops){
  if($op.source){
   $parent=Split-Path $op.destination -Parent;$missing=[Collections.Generic.List[string]]::new()
   while(!(Test-Path -LiteralPath $parent)){if(!$parent.StartsWith($support,[StringComparison]::OrdinalIgnoreCase)){throw 'Bad parent'};$missing.Add($parent);$parent=Split-Path $parent -Parent}
   for($j=$missing.Count-1;$j -ge 0;$j--){New-Item -ItemType Directory -Path $missing[$j] | Out-Null;$created.Add($missing[$j])}
   $op.changed=$true;Copy-Item -LiteralPath $op.stage -Destination $op.destination -Force
   if((Get-FileHash -LiteralPath $op.stage).Hash -ne (Get-FileHash -LiteralPath $op.destination).Hash){throw "Installed hash mismatch: $($op.destination)"}
  }else{$op.changed=$true;Remove-Item -LiteralPath $op.destination -Force}
 }
}catch{
 $reason=$_.Exception.Message;$errors=[Collections.Generic.List[string]]::new()
 for($i=$ops.Count-1;$i -ge 0;$i--){$op=$ops[$i];if(!$op.changed){continue};try{
  if($op.existed){Copy-Item -LiteralPath $op.saved -Destination $op.destination -Force}
  elseif(Test-Path -LiteralPath $op.destination){Remove-Item -LiteralPath $op.destination -Force}
 }catch{$errors.Add($_.Exception.Message)}}
 for($i=$created.Count-1;$i -ge 0;$i--){$dir=$created[$i];if($dir.StartsWith($support+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -and (Test-Path -LiteralPath $dir) -and @(Get-ChildItem -LiteralPath $dir -Force).Count -eq 0){Remove-Item -LiteralPath $dir -Force}}
 if($errors.Count){throw "Install failed: $reason. Recovery incomplete; backup=$backup; $($errors -join '; ')"}
 throw "Install failed; all changed files restored. $reason. Backup=$backup"
}
Write-Output "Installed FYLM Pixel Art, FYLM CRT VHS and FYLM ShaderGlass. Backup: $backup. Restart AE."
