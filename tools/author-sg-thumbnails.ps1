$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$work=Join-Path $root 'build/sg-thumbnails'
$destination=Join-Path $root 'assets/ShaderGlass/Library/thumbnails'
New-Item -ItemType Directory -Path $work,$destination -Force | Out-Null
Add-Type -AssemblyName System.Drawing
# Only the original FYLM procedural diagnostic is used. Private artwork is excluded.
$source=[Drawing.Bitmap]::new((Join-Path $root 'assets/presets/pixel-art/thumbnails/source.png'))
$width=$source.Width;$height=$source.Height
$data=$source.LockBits([Drawing.Rectangle]::new(0,0,$width,$height),[Drawing.Imaging.ImageLockMode]::ReadOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
$bytes=[byte[]]::new($width*$height*4)
[Runtime.InteropServices.Marshal]::Copy($data.Scan0,$bytes,0,$bytes.Length)
$source.UnlockBits($data);$source.Dispose()
[IO.File]::WriteAllBytes((Join-Path $work 'input.bgra'),$bytes)
& node -e 'const fs=require("fs"),p=require("path"),d=process.argv[1],b=fs.readFileSync(p.join(d,"input.bgra"));for(let i=0;i<b.length;i+=4){let t=b[i];b[i]=b[i+2];b[i+2]=t;}fs.writeFileSync(p.join(d,"input.rgba"),b);' $work
if($LASTEXITCODE -ne 0){throw 'Source conversion failed'}
$tool=Join-Path $root 'build/modules/shaderglass/Release/FYLM-sglibrary.exe'
$probe=Join-Path $root 'build/modules/shaderglass/Release/FYLM-sgprobe.exe'
$catalog=Get-Content (Join-Path $root 'assets/ShaderGlass/Library/catalog-v1.json') -Raw -Encoding utf8|ConvertFrom-Json
foreach($entry in $catalog.entries){
 if($entry.menu -eq 3){continue}
 $preset=Join-Path $work ($entry.slug+'.fylmsg');$image=Join-Path $work ($entry.slug+'.rgba')
 & $tool --export $entry.menu $preset
 if($LASTEXITCODE -ne 0){throw 'Preset export failed'}
 & $probe $preset $width $height $entry.virtual_height 0 $image (Join-Path $work ($entry.slug+'.json')) (Join-Path $work 'input.rgba') | Out-Null
 if($LASTEXITCODE -ne 0){throw ('Thumbnail render failed: '+$entry.slug)}
 & node -e 'const fs=require("fs"),b=fs.readFileSync(process.argv[1]);for(let i=0;i<b.length;i+=4){let t=b[i];b[i]=b[i+2];b[i+2]=t;}fs.writeFileSync(process.argv[2],b);' $image (Join-Path $work 'frame.bgra')
 $bytes=[IO.File]::ReadAllBytes((Join-Path $work 'frame.bgra'))
 $bitmap=[Drawing.Bitmap]::new($width,$height)
 $data=$bitmap.LockBits([Drawing.Rectangle]::new(0,0,$width,$height),[Drawing.Imaging.ImageLockMode]::WriteOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
 [Runtime.InteropServices.Marshal]::Copy($bytes,0,$data.Scan0,$bytes.Length)
 $bitmap.UnlockBits($data)
 $thumb=[Drawing.Bitmap]::new(192,128);$graphics=[Drawing.Graphics]::FromImage($thumb)
 $graphics.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
 $graphics.DrawImage($bitmap,0,0,192,128);$graphics.Dispose();$bitmap.Dispose()
 $thumb.Save((Join-Path $destination ($entry.slug+'.png')));$thumb.Dispose()
}
[pscustomobject]@{source='FYLM original procedural four-scene diagnostic';presets=($catalog.entries.Count-1);renderer='D3D11';subjective_acceptance='pending';private_artwork_used=$false}|ConvertTo-Json|Set-Content (Join-Path $destination 'README.json') -Encoding utf8
Write-Output ('Authored '+($catalog.entries.Count-1)+' diagnostic thumbnails; no private artwork included.')
