param([string]$Root=(Split-Path $PSScriptRoot -Parent))
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$folder=Join-Path $Root 'docs/0.3-ui-verified'
$log=Get-Content -LiteralPath (Join-Path $folder 'host.txt') -Raw
if($log -notmatch 'UI_03_COMPLETE' -or $log -match 'ERROR '){throw 'AE UI script did not complete'}
function Sample([string]$Path){
 $bmp=[Drawing.Bitmap]::new($Path)
 try{
  $bytes=[byte[]]::new($bmp.Width*$bmp.Height*4);$nonzeroAlpha=0
  for($y=0;$y -lt $bmp.Height;$y++){for($x=0;$x -lt $bmp.Width;$x++){
   $p=$bmp.GetPixel($x,$y);$i=($y*$bmp.Width+$x)*4
   $bytes[$i]=$p.R;$bytes[$i+1]=$p.G;$bytes[$i+2]=$p.B;$bytes[$i+3]=$p.A
   if($p.A -gt 0){$nonzeroAlpha++}
  }}
  return [pscustomobject]@{file=(Split-Path $Path -Leaf);width=$bmp.Width;height=$bmp.Height;nonzeroAlpha=$nonzeroAlpha;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes))}
 }finally{$bmp.Dispose()}
}
$first=Get-ChildItem -LiteralPath $folder -Filter '*-frame0.png'
if(@($first).Count -ne 1){throw 'Expected one host language sample'}
$prefix=$first.BaseName -replace '-frame0$',''
$a=Sample $first.FullName
$b=Sample (Join-Path $folder ($prefix+'-frame12.png'))
$repeat=Sample (Join-Path $folder ($prefix+'-frame0-repeat.png'))
foreach($sample in @($a,$b,$repeat)){
 if($sample.width -ne 96 -or $sample.height -ne 64 -or $sample.nonzeroAlpha -eq 0){throw 'Empty or wrong-size frame'}
}
$dynamic=$a.sha256 -ne $b.sha256
$repeatable=$a.sha256 -eq $repeat.sha256
$installed='C:/Program Files/Adobe/Adobe After Effects 2026/Support Files/Plug-ins/DitheringStudio/DitheringStudio.aex'
[pscustomobject]@{locale=$prefix;timesSeconds=@(0,0.5,0);samples=@($a,$b,$repeat);dynamic=$dynamic;repeatable=$repeatable;installedPluginSha256=(Get-FileHash -LiteralPath $installed -Algorithm SHA256).Hash;buildPluginSha256=(Get-FileHash -LiteralPath (Join-Path $Root 'build/plugin/ae/Release/DitheringStudio.aex') -Algorithm SHA256).Hash} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $folder 'comparison.json') -Encoding utf8
if(!$dynamic){throw 'VHS did not change between 0 and 0.5 seconds'}
if(!$repeatable){throw 'Replaying the same time changed the result'}
Write-Output 'AE UI: automatic language, independent switches and preset apply/read passed. Both in-range frames are nonempty, differ across time, and repeat exactly at the same time.'
