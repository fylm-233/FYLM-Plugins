param([string]$Folder,[string]$Reference='docs/pixel-art/0.7-samples/pc98.png',[string]$ReportName='pixel-verification.json')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$root=[IO.Path]::GetFullPath($Folder)
$files=@(Get-ChildItem -LiteralPath $root -File -Recurse | Where-Object {$_.Name -match '\.(tif|tiff|png)$' -and $_.Name -notlike 'AEtemp-*'})
if(!$files.Count){throw "No completed images in $root"}
$referenceImage=[Drawing.Bitmap]::new([IO.Path]::GetFullPath($Reference))
try{
 $results=foreach($file in $files){
  $image=[Drawing.Bitmap]::new($file.FullName)
  try{
   if($image.Width -ne $referenceImage.Width -or $image.Height -ne $referenceImage.Height){throw "Wrong size $($file.Name)"}
   $colors=[Collections.Generic.HashSet[int]]::new();$pixels=[byte[]]::new($image.Width*$image.Height*4);$different=0;$max=0
   for($y=0;$y -lt $image.Height;$y++){for($x=0;$x -lt $image.Width;$x++){
    $p=$image.GetPixel($x,$y);$q=$referenceImage.GetPixel($x,$y);if($p.A -eq 255){[void]$colors.Add($p.ToArgb())}
    $i=($y*$image.Width+$x)*4;$pixels[$i]=$p.R;$pixels[$i+1]=$p.G;$pixels[$i+2]=$p.B;$pixels[$i+3]=$p.A
    $d=[Math]::Max([Math]::Abs([int]$p.R-$q.R),[Math]::Max([Math]::Abs([int]$p.G-$q.G),[Math]::Max([Math]::Abs([int]$p.B-$q.B),[Math]::Abs([int]$p.A-$q.A))))
    if($d){$different++};$max=[Math]::Max($max,$d)
   }}
   $sha=[Security.Cryptography.SHA256]::Create();try{$pixelHash=[BitConverter]::ToString($sha.ComputeHash($pixels)).Replace('-','')}finally{$sha.Dispose()}
   [pscustomobject]@{file=$file.FullName;width=$image.Width;height=$image.Height;opaque_colors=$colors.Count;decoded_rgba_sha256=$pixelHash;different_pixels_vs_cli=$different;maximum_channel_difference_vs_cli=$max;file_sha256=(Get-FileHash -LiteralPath $file.FullName).Hash}
  }finally{$image.Dispose()}
 }
 $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $root $ReportName) -Encoding utf8
 $results | Select-Object file,opaque_colors,different_pixels_vs_cli,maximum_channel_difference_vs_cli | Format-List
}finally{$referenceImage.Dispose()}
