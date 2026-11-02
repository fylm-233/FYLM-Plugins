param([Parameter(Mandatory=$true)][string]$Folder)
$ErrorActionPreference='Stop'
$all=@()
foreach($dir in Get-ChildItem -LiteralPath $Folder -Directory){
 $ref=switch -Wildcard ($dir.Name){
  'pc98*' {'docs/pixel-art/0.7-samples/pc98.png';break}
  'gba-area*' {'docs/pixel-art/0.8-samples/gba-area.png';break}
  'gba*' {'docs/pixel-art/0.8-samples/gba.png';break}
  'modern*' {'docs/pixel-art/0.8-samples/modern.png';break}
  default {"docs/pixel-art/0.8-samples/$($dir.Name).png"}
 }
 & (Join-Path $PSScriptRoot 'verify-pc98-host.ps1') -Folder $dir.FullName -Reference $ref | Out-Null
 $r=Get-Content -LiteralPath (Join-Path $dir.FullName 'pixel-verification.json') -Raw|ConvertFrom-Json
 $r|Add-Member -NotePropertyName case -NotePropertyValue $dir.Name;$all+=$r
}
$all|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $Folder 'summary.json') -Encoding utf8
$all|Select-Object case,opaque_colors,different_pixels_vs_cli,maximum_channel_difference_vs_cli|Format-Table -AutoSize
# The C++ test emits this reference with exactly specified 32768-scale input
# conversion, allowing input precision effects to be separated from label edits.
Add-Type -AssemblyName System.Drawing
$raw=[IO.File]::ReadAllBytes((Join-Path $PWD 'build/modern-fixture-host16.rgba'))
if($raw.Length -ne 384*256*4){throw 'Run the GBA/Modern CTest fixture first'}
$bitmap=[Drawing.Bitmap]::new(384,256)
try{
 for($y=0;$y -lt 256;$y++){for($x=0;$x -lt 384;$x++){$i=($y*384+$x)*4;$bitmap.SetPixel($x,$y,[Drawing.Color]::FromArgb([int]$raw[$i+3],[int]$raw[$i],[int]$raw[$i+1],[int]$raw[$i+2]))}}
 $reference=Join-Path ([IO.Path]::GetFullPath($Folder)) 'modern-32768-source-reference.png'
 $bitmap.Save($reference,[Drawing.Imaging.ImageFormat]::Png)
}finally{$bitmap.Dispose()}
foreach($name in @('modern-16','modern-32')){
 & (Join-Path $PSScriptRoot 'verify-pc98-host.ps1') -Folder (Join-Path $Folder $name) -Reference $reference -ReportName 'pixel-verification-32768.json'
}
