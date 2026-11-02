param([string]$InputDirectory='C:\Users\FYLM\Downloads\test img',[string]$OutputDirectory='build/p1-private-qa')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$root=Split-Path $PSScriptRoot -Parent
$out=[IO.Path]::GetFullPath((Join-Path $root $OutputDirectory))
New-Item -ItemType Directory -Path $out -Force | Out-Null
function Write-Rgba($bitmap,$path){
 $rect=[Drawing.Rectangle]::new(0,0,$bitmap.Width,$bitmap.Height)
 $bits=$bitmap.LockBits($rect,[Drawing.Imaging.ImageLockMode]::ReadOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
 try{$bytes=[byte[]]::new($bitmap.Width*$bitmap.Height*4);[Runtime.InteropServices.Marshal]::Copy($bits.Scan0,$bytes,0,$bytes.Length)
 for($i=0;$i -lt $bytes.Length;$i+=4){$v=$bytes[$i];$bytes[$i]=$bytes[$i+2];$bytes[$i+2]=$v};[IO.File]::WriteAllBytes($path,$bytes)}finally{$bitmap.UnlockBits($bits)}
}
function Read-Rgba($path,$w,$h){
 $bytes=[IO.File]::ReadAllBytes($path);for($i=0;$i -lt $bytes.Length;$i+=4){$v=$bytes[$i];$bytes[$i]=$bytes[$i+2];$bytes[$i+2]=$v}
 $bitmap=[Drawing.Bitmap]::new($w,$h,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
 $bits=$bitmap.LockBits([Drawing.Rectangle]::new(0,0,$w,$h),[Drawing.Imaging.ImageLockMode]::WriteOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
 try{[Runtime.InteropServices.Marshal]::Copy($bytes,0,$bits.Scan0,$bytes.Length)}finally{$bitmap.UnlockBits($bits)};return $bitmap
}
$manifest=@();$catalog=Get-Content -Raw -LiteralPath (Join-Path $root 'assets/presets/pixel-art/catalog-v1.json') | ConvertFrom-Json
$font=[Drawing.Font]::new('Arial',14)
foreach($name in @('博丽灵梦（红魔乡立绘）.png','143385443_p0.png','119751990_p0.jpg')){
 $path=Join-Path $InputDirectory $name;if(!(Test-Path -LiteralPath $path)){continue}
 $source=[Drawing.Image]::FromFile($path);$w=[Math]::Min(1280,$source.Width);$h=[int][Math]::Round($source.Height*$w/$source.Width)
 $tag=[IO.Path]::GetFileNameWithoutExtension($name);$case=Join-Path $out $tag;New-Item -ItemType Directory -Path $case -Force | Out-Null
 $input=[Drawing.Bitmap]::new($w,$h,[Drawing.Imaging.PixelFormat]::Format32bppArgb);$g=[Drawing.Graphics]::FromImage($input)
 $g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic;$g.CompositingMode=[Drawing.Drawing2D.CompositingMode]::SourceCopy;$g.DrawImage($source,0,0,$w,$h);$g.Dispose();$source.Dispose()
 $input.Save((Join-Path $case 'input.png'));$raw=Join-Path $case 'input.rgba';Write-Rgba $input $raw
 $tileW=280;$tileH=[int][Math]::Round($h*280/$w)+30
 $sheet=[Drawing.Bitmap]::new($tileW*4,$tileH*4);$g=[Drawing.Graphics]::FromImage($sheet);$g.Clear([Drawing.Color]::FromArgb(40,40,48));$g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
 $n=0;$timings=@()
 foreach($preset in $catalog.presets){foreach($variant in @('A','B')){
  $target=Join-Path $case ($preset.id+'-'+$variant+'.rgba');$clock=[Diagnostics.Stopwatch]::StartNew()
  & (Join-Path $root 'build/Release/pixel_art_cli.exe') --preset $preset.id $variant $raw $target $w $h
  if($LASTEXITCODE -ne 0){throw "Preset render failed: $($preset.id) $variant"};$clock.Stop()
  $image=Read-Rgba $target $w $h;$image.Save((Join-Path $case ($preset.id+'-'+$variant+'.png')))
  $x=($n%4)*$tileW;$y=[Math]::Floor($n/4)*$tileH;$g.DrawImage($image,$x,$y+30,$tileW,$tileH-30)
  $g.DrawString(($preset.id+' '+$variant),$font,[Drawing.Brushes]::White,$x+6,$y+4);$image.Dispose();$n++
  $timings+=@{id=$preset.id;variant=$variant;cli_wall_ms=$clock.ElapsedMilliseconds}
 }}
 $g.Dispose();$sheet.Save((Join-Path $case 'contact-sheet.jpg'),[Drawing.Imaging.ImageFormat]::Jpeg);$sheet.Dispose();$input.Dispose()
 $manifest+=@{input=$path;input_sha256=(Get-FileHash -LiteralPath $path).Hash;width=$w;height=$h;derivation='High quality bicubic downscale, no crop; private QA only';timings=$timings}
}
$font.Dispose();$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $out 'manifest.json') -Encoding utf8
Write-Output "Private previews saved: $out"
