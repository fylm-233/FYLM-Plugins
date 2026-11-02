param([string]$Output='docs/pixel-art/0.7-samples')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$out=[IO.Path]::GetFullPath((Join-Path $PWD $Output));New-Item -ItemType Directory -Path $out -Force | Out-Null
$w=384;$h=256;$bitmap=[Drawing.Bitmap]::new($w,$h)
for($y=0;$y -lt $h;$y++){for($x=0;$x -lt $w;$x++){$r=[int](255*$x/($w-1));$g=[int](200*$y/($h-1));$b=[int](150+80*[Math]::Sin($x/80));$bitmap.SetPixel($x,$y,[Drawing.Color]::FromArgb(255,$r,$g,$b))}}
$g=[Drawing.Graphics]::FromImage($bitmap);$g.FillEllipse([Drawing.Brushes]::MistyRose,110,45,118,160);$g.FillEllipse([Drawing.Brushes]::MidnightBlue,98,30,146,76);$g.FillRectangle([Drawing.Brushes]::MidnightBlue,108,62,18,110);$g.FillRectangle([Drawing.Brushes]::MidnightBlue,220,62,18,110);$g.DrawLine([Drawing.Pens]::MidnightBlue,140,117,158,117);$g.DrawLine([Drawing.Pens]::MidnightBlue,184,117,202,117);$g.DrawLine([Drawing.Pens]::IndianRed,160,160,183,160);for($i=0;$i -lt 4;$i++){$g.DrawLine([Drawing.Pens]::White,8,215+$i*5,90,230+$i*5)};$g.Dispose()
$bitmap.Save((Join-Path $out 'input.png'),[Drawing.Imaging.ImageFormat]::Png);$raw=[byte[]]::new($w*$h*4)
for($y=0;$y -lt $h;$y++){for($x=0;$x -lt $w;$x++){$p=$bitmap.GetPixel($x,$y);$i=($y*$w+$x)*4;$raw[$i]=$p.R;$raw[$i+1]=$p.G;$raw[$i+2]=$p.B;$raw[$i+3]=$p.A}};$bitmap.Dispose();[IO.File]::WriteAllBytes((Join-Path $out 'input.rgba'),$raw)
& (Join-Path $PWD 'build/Release/pixel_art_cli.exe') (Join-Path $out 'input.rgba') (Join-Path $out 'pc98.rgba') $w $h 3 pc98
if($LASTEXITCODE -ne 0){throw 'PC98 CLI failed'}
$raw=[IO.File]::ReadAllBytes((Join-Path $out 'pc98.rgba'));$image=[Drawing.Bitmap]::new($w,$h);$colors=[Collections.Generic.HashSet[int]]::new()
for($y=0;$y -lt $h;$y++){for($x=0;$x -lt $w;$x++){$i=($y*$w+$x)*4;$c=[Drawing.Color]::FromArgb([int]$raw[$i+3],[int]$raw[$i],[int]$raw[$i+1],[int]$raw[$i+2]);$image.SetPixel($x,$y,$c);if($c.A -eq 255){[void]$colors.Add($c.ToArgb())}}}
$image.Save((Join-Path $out 'pc98.png'),[Drawing.Imaging.ImageFormat]::Png);$image.Dispose();[pscustomobject]@{width=$w;height=$h;opaque_colors=$colors.Count;source='Generated diagnostic image, not human art or visual acceptance'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'summary.json')
Write-Output "Saved input/PC98 samples; colors=$($colors.Count)"
