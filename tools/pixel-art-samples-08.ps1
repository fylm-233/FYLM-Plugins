param([string]$Output='docs/pixel-art/0.8-samples')
$ErrorActionPreference='Stop';Add-Type -AssemblyName System.Drawing
$folder=[IO.Path]::GetFullPath((Join-Path $PWD $Output));New-Item -ItemType Directory -Path $folder -Force | Out-Null
Copy-Item -LiteralPath docs/pixel-art/0.7-samples/input.png -Destination $folder -Force
Copy-Item -LiteralPath docs/pixel-art/0.7-samples/input.rgba -Destination $folder -Force
$cli=Join-Path $PWD 'build/Release/pixel_art_cli.exe'
function Save-Rgba([string]$Name,[int]$Width,[int]$Height){
 $raw=[IO.File]::ReadAllBytes((Join-Path $folder ($Name+'.rgba')));if($raw.Length -ne $Width*$Height*4){throw 'Wrong output size'}
 $bitmap=[Drawing.Bitmap]::new($Width,$Height);$colors=[Collections.Generic.HashSet[int]]::new();$covered=0
 for($y=0;$y -lt $Height;$y++){for($x=0;$x -lt $Width;$x++){$i=($y*$Width+$x)*4;$c=[Drawing.Color]::FromArgb([int]$raw[$i+3],[int]$raw[$i],[int]$raw[$i+1],[int]$raw[$i+2]);$bitmap.SetPixel($x,$y,$c);if($c.A -eq 255){[void]$colors.Add($c.ToArgb());$covered++}}}
 $bitmap.Save((Join-Path $folder ($Name+'.png')),[Drawing.Imaging.ImageFormat]::Png);$bitmap.Dispose();return [pscustomobject]@{name=$Name;width=$Width;height=$Height;opaque_colors=$colors.Count;opaque_pixels=$covered;sha256=(Get-FileHash -LiteralPath (Join-Path $folder ($Name+'.rgba'))).Hash}
}
$results=@()
foreach($mode in @('gba','modern')){$block=if($mode -eq 'gba'){6}else{3};$timer=[Diagnostics.Stopwatch]::StartNew();& $cli (Join-Path $folder 'input.rgba') (Join-Path $folder ($mode+'.rgba')) 384 256 $block $mode;if($LASTEXITCODE -ne 0){throw "$mode CLI failed"};$timer.Stop();$r=Save-Rgba $mode 384 256;$r|Add-Member -NotePropertyName single_cli_elapsed_ms -NotePropertyValue $timer.Elapsed.TotalMilliseconds;$results+=$r}
& $cli (Join-Path $folder 'input.rgba') (Join-Path $folder 'gba-area.rgba') 384 256 6 gba 0 0 0 1 area
if($LASTEXITCODE -ne 0){throw 'GBA area baseline failed'}
$results+=Save-Rgba 'gba-area' 384 256
# Choose the closest distinct built-in colors: a controlled low-contrast cleanup
# fixture plus a long thin stroke and a high-contrast one-cell highlight.
$colors=(Get-Content assets/palettes/fylm-modern.json -Raw|ConvertFrom-Json).colors|ForEach-Object {[Drawing.ColorTranslator]::FromHtml($_)}
$best=[double]::MaxValue;$a=$null;$b=$null
for($i=0;$i -lt $colors.Count;$i++){for($j=$i+1;$j -lt $colors.Count;$j++){$x=$colors[$i];$y=$colors[$j];$d=[Math]::Pow([int]$x.R-$y.R,2)+[Math]::Pow([int]$x.G-$y.G,2)+[Math]::Pow([int]$x.B-$y.B,2);if($d -lt $best){$best=$d;$a=$x;$b=$y}}}
$w=81;$h=81;$raw=[byte[]]::new($w*$h*4)
for($y=0;$y -lt $h;$y++){for($x=0;$x -lt $w;$x++){$gx=[int][Math]::Floor($x/3);$gy=[int][Math]::Floor($y/3);$c=$a;if(($gx -eq 13 -and $gy -eq 13)-or($gx -eq 5 -and $gy -ge 5 -and $gy -le 20)-or($gx -eq 6 -and $gy -eq 12)){$c=$b};if($gx -eq 20 -and $gy -eq 12){$c=$colors[-1]};$i=($y*$w+$x)*4;$raw[$i]=$c.R;$raw[$i+1]=$c.G;$raw[$i+2]=$c.B;$raw[$i+3]=255}}
[IO.File]::WriteAllBytes((Join-Path $folder 'structure-input.rgba'),$raw);$results+=Save-Rgba 'structure-input' $w $h
foreach($view in @('final','baseline','mask')){$name='structure-'+$view;& $cli (Join-Path $folder 'structure-input.rgba') (Join-Path $folder ($name+'.rgba')) $w $h 3 modern 0 0 0 1 feature $view;if($LASTEXITCODE -ne 0){throw "$view CLI failed"};$results+=Save-Rgba $name $w $h}
$results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $folder 'summary.json') -Encoding utf8
Write-Output ($results|ConvertTo-Json -Depth 5)
