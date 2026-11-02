$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$work=Join-Path $root 'build/vhs-thumbnails'
$destination=Join-Path $root 'assets/presets/vhs/thumbnails'
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
$probe=Join-Path $root 'build/Release/FYLM-vhsprobe.exe'
$catalog=Get-Content (Join-Path $root 'assets/presets/vhs/catalog-v1.json') -Raw -Encoding utf8|ConvertFrom-Json
foreach($entry in $catalog.entries){
 $config=Join-Path $work ($entry.code+'-settings.json');$settings=[ordered]@{'retro.mode'=2};foreach($property in $entry.settings.PSObject.Properties){$settings[$property.Name]=$property.Value};$settings|ConvertTo-Json|Set-Content $config -Encoding utf8
 $image=Join-Path $work ($entry.code+'.rgba')
 & $probe $config $width $height .5 $image (Join-Path $work ($entry.code+'.json')) (Join-Path $work 'input.rgba') | Out-Null
 if($LASTEXITCODE -ne 0){throw ('Thumbnail render failed: '+$entry.code)}
 & node -e 'const fs=require("fs"),b=fs.readFileSync(process.argv[1]);for(let i=0;i<b.length;i+=4){let t=b[i];b[i]=b[i+2];b[i+2]=t;}fs.writeFileSync(process.argv[2],b);' $image (Join-Path $work 'frame.bgra')
 $bytes=[IO.File]::ReadAllBytes((Join-Path $work 'frame.bgra'))
 $bitmap=[Drawing.Bitmap]::new($width,$height)
 $data=$bitmap.LockBits([Drawing.Rectangle]::new(0,0,$width,$height),[Drawing.Imaging.ImageLockMode]::WriteOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
 [Runtime.InteropServices.Marshal]::Copy($bytes,0,$data.Scan0,$bytes.Length)
 $bitmap.UnlockBits($data)
 $thumb=[Drawing.Bitmap]::new(192,128);$graphics=[Drawing.Graphics]::FromImage($thumb)
 $graphics.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
 $graphics.DrawImage($bitmap,0,0,192,128);$graphics.Dispose();$bitmap.Dispose()
 $thumb.Save((Join-Path $destination ($entry.code+'.png')));$thumb.Dispose()
}
[pscustomobject]@{source='FYLM original procedural four-scene diagnostic';presets=$catalog.entries.Count;renderer='D3D11';subjective_acceptance='pending';private_artwork_used=$false}|ConvertTo-Json|Set-Content (Join-Path $destination 'README.json') -Encoding utf8
Write-Output ('Authored '+$catalog.entries.Count+' diagnostic thumbnails; no private artwork included.')
