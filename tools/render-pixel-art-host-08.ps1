param([Parameter(Mandatory=$true)][string]$Projects,[string]$Output='docs/0.8-host/verified', [string]$AfterEffects='C:\Program Files\Adobe\Adobe After Effects 2026\Support Files')
$ErrorActionPreference='Stop'
$projectsPath=[IO.Path]::GetFullPath($Projects)
$outputPath=[IO.Path]::GetFullPath($Output)
$names=@('gba-16','gba-8','gba-32','modern-16','modern-8','modern-32','pc98-8','gba-area-8','structure-final','structure-baseline','structure-mask')
foreach($name in $names){
 $out=Join-Path $outputPath $name
 New-Item -ItemType Directory -Path $out -Force | Out-Null
 # Never accept an earlier image as evidence of a new render.
 if(@(Get-ChildItem -LiteralPath $out -Filter 'frame_*.tif').Count){throw "Use a fresh output directory: $out"}
 & (Join-Path $AfterEffects 'aerender.exe') -project (Join-Path $projectsPath "$name.aep") -comp 'FYLM Pixel Art 0.8' -s 0 -e 0 -OMtemplate '带有 Alpha 的 TIFF 序列 ' -output (Join-Path $out 'frame_[#####].tif') -v ERRORS_AND_PROGRESS -log (Join-Path $out 'aerender.log') *> (Join-Path $out 'console.log')
 $exitCode=$LASTEXITCODE
 $images=@(Get-ChildItem -LiteralPath $out -Filter 'frame_*.tif')
 if($exitCode -ne 0 -or !$images.Count){throw "Render failed or produced no image: $name (exit $exitCode)"}
 $reference=switch -Wildcard ($name){
  'pc98*' {'docs/pixel-art/0.7-samples/pc98.png';break}
  'gba-area*' {'docs/pixel-art/0.8-samples/gba-area.png';break}
  'gba*' {'docs/pixel-art/0.8-samples/gba.png';break}
  'modern*' {'docs/pixel-art/0.8-samples/modern.png';break}
  default {"docs/pixel-art/0.8-samples/$name.png"}
 }
 & (Join-Path $PSScriptRoot 'verify-pc98-host.ps1') -Folder $out -Reference $reference
 Write-Output "RENDERED_AND_COMPARED $name"
}
