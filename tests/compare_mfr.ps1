param([string]$Root=(Split-Path $PSScriptRoot -Parent),[string]$Folder='')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
function ImageHash([string]$file){
 $bitmap=[Drawing.Bitmap]::new($file)
 try{
  $rect=[Drawing.Rectangle]::new(0,0,$bitmap.Width,$bitmap.Height)
  $data=$bitmap.LockBits($rect,[Drawing.Imaging.ImageLockMode]::ReadOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
  try{
   $row=$bitmap.Width*4;$pixels=[byte[]]::new($row*$bitmap.Height)
   for($y=0;$y -lt $bitmap.Height;$y++){[Runtime.InteropServices.Marshal]::Copy([IntPtr]::Add($data.Scan0,$y*$data.Stride),$pixels,$y*$row,$row)}
   return @{width=$bitmap.Width;height=$bitmap.Height;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($pixels))}
  }finally{$bitmap.UnlockBits($data)}
 }finally{$bitmap.Dispose()}
}
if(!$Folder){$Folder=Join-Path $Root 'docs\0.3-mfr'};$images=@()
foreach($name in @('bayer8','fs16')){
 $serial=@(Get-ChildItem -LiteralPath (Join-Path $folder "$name-serial") -Filter '*.tif' | Sort-Object Name)
 $parallel=@(Get-ChildItem -LiteralPath (Join-Path $folder "$name-mfr") -Filter '*.tif' | Sort-Object Name)
 if($serial.Count -ne 100 -or $parallel.Count -ne 100){throw "Frame count mismatch: $name"}
 for($i=0;$i -lt 100;$i++){
  $a=ImageHash $serial[$i].FullName;$b=ImageHash $parallel[$i].FullName
  if($a.width -ne $b.width -or $a.height -ne $b.height -or $a.sha256 -ne $b.sha256){throw "Pixel mismatch: $name/$i"}
  $images+=@{case=$name;frame=$i;width=$a.width;height=$a.height;sha256=$a.sha256}
 }
 Write-Output "$name : 100/100 decoded RGBA8 images identical (TIFF metadata excluded)."
}
$lines=Get-Content -LiteralPath (Join-Path $folder 'render-audit.csv')
$begins=@();$ends=@{}
foreach($line in $lines){$p=$line.Split(',');if($p.Length -ne 6){throw 'Invalid audit row'};$key="$($p[2]):$($p[3])";$record=@{tick=[long]$p[1];thread=[int]$p[2];token=$p[3];value=[long]$p[4];scale=[long]$p[5];key=$key};if($p[0] -eq 'begin'){$begins+=$record}else{$ends[$key]=$record}}
$begins=@($begins | Sort-Object {$_.tick})
if($begins.Count -ne 400 -or $ends.Count -ne 400){throw "Unexpected native callback count: $($begins.Count) / $($ends.Count). Cannot attribute four phases safely."}
$phases=@();$names=@('bayer8-serial','bayer8-mfr','fs16-serial','fs16-mfr')
for($phase=0;$phase -lt 4;$phase++){
 $events=@();$threads=@{};$frames=@{}
 for($i=0;$i -lt 100;$i++){$start=$begins[$phase*100+$i];$end=$ends[$start.key];if(!$end){throw 'Unbalanced native render'};$events+=@{tick=$start.tick;delta=1};$events+=@{tick=$end.tick;delta=-1};$threads[$start.thread]=1;$frames["$($start.value)/$($start.scale)"]=1}
 $active=0;$maximum=0;foreach($e in ($events | Sort-Object @{Expression={$_.tick}},@{Expression={$_.delta}})){$active+=$e.delta;$maximum=[Math]::Max($maximum,$active)}
 if($active -ne 0 -or $frames.Count -ne 100){throw 'Invalid phase balance or duplicate frame time'}
 $phases+=@{name=$names[$phase];callbacks=100;threads=$threads.Count;max_overlapping_callbacks=$maximum;unique_times=$frames.Count}
}
if($phases[1].max_overlapping_callbacks -le 1 -or $phases[3].max_overlapping_callbacks -le 1){throw 'AE did not demonstrate concurrent MFR callbacks'}
@{pixel_comparisons=$images;native_phases=$phases;decoded_format='RGBA8 via GDI+; TIFF metadata excluded'}|ConvertTo-Json -Depth 6|Set-Content (Join-Path $folder 'comparison.json') -Encoding utf8
$phases|ForEach-Object {Write-Output "$($_.name): callbacks=$($_.callbacks), threads=$($_.threads), overlap=$($_.max_overlapping_callbacks)"}
