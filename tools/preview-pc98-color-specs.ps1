param([string]$CaseDirectory='build/p1-private-qa/143385443_p0',[string]$OutputDirectory='build/092-private-qa')
$ErrorActionPreference='Stop';Add-Type -AssemblyName System.Drawing
Add-Type 'public static class FYLMColorSpecBytes { public static void Swap(byte[] a) { for(int i=0;i<a.Length;i+=4){byte t=a[i];a[i]=a[i+2];a[i+2]=t;} } }'
$root=Split-Path $PSScriptRoot -Parent;$case=Join-Path $root $CaseDirectory;$out=Join-Path $root $OutputDirectory;New-Item -ItemType Directory -Path $out -Force | Out-Null
$input=[Drawing.Image]::FromFile((Join-Path $case 'input.png'));$w=$input.Width;$h=$input.Height;$tw=300;$th=[int][Math]::Round($h*$tw/$w)+30
$sheet=[Drawing.Bitmap]::new($tw*4,$th*2);$g=[Drawing.Graphics]::FromImage($sheet);$g.Clear([Drawing.Color]::FromArgb(40,40,48));$g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::NearestNeighbor;$font=[Drawing.Font]::new('Arial',13)
$g.DrawImage($input,0,30,$tw,$th-30);$g.DrawString('Input',$font,[Drawing.Brushes]::White,5,5);$input.Dispose();$n=1;$timings=@()
foreach($entry in @(@('art',8,'Art / 2-16'),@('digital8',8,'RGB3 / 8'),@('analog16',8,'Index4 / RGB444'),@('indexed256',6,'Index8 / DAC6'),@('indexed256',8,'Index8 / DAC8'),@('rgb565',8,'RGB565 / 16'),@('rgb888',8,'RGB888 / 24'))){
 $id=$entry[0]+'-'+$entry[1];$raw=Join-Path $out ($id+'.rgba');$clock=[Diagnostics.Stopwatch]::StartNew()
 & (Join-Path $root 'build/Release/pixel_art_cli.exe') --pc98-spec $entry[0] (Join-Path $case 'input.rgba') $raw $w $h 4 $entry[1] 256 0
 if($LASTEXITCODE){throw "PC98 specification render failed: $id"};$clock.Stop();$timings+=@{spec=$entry[0];dac_bits=$entry[1];width=$w;height=$h;block=4;cli_wall_ms=$clock.ElapsedMilliseconds}
 $bytes=[IO.File]::ReadAllBytes($raw);[FYLMColorSpecBytes]::Swap($bytes);$bitmap=[Drawing.Bitmap]::new($w,$h,[Drawing.Imaging.PixelFormat]::Format32bppArgb);$bits=$bitmap.LockBits([Drawing.Rectangle]::new(0,0,$w,$h),[Drawing.Imaging.ImageLockMode]::WriteOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
 try{[Runtime.InteropServices.Marshal]::Copy($bytes,0,$bits.Scan0,$bytes.Length)}finally{$bitmap.UnlockBits($bits)}
 $bitmap.Save((Join-Path $out ($id+'.png')));$x=($n%4)*$tw;$y=[Math]::Floor($n/4)*$th;$g.DrawImage($bitmap,$x,$y+30,$tw,$th-30);$g.DrawString($entry[2],$font,[Drawing.Brushes]::White,$x+5,$y+5);$bitmap.Dispose();$n++
}
$g.Dispose();$font.Dispose();$sheet.Save((Join-Path $out 'contact-sheet.jpg'),[Drawing.Imaging.ImageFormat]::Jpeg);$sheet.Dispose();$timings | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'cli-timings.json') -Encoding utf8
Write-Output 'Private QA only: no user artwork may be distributed with packages.'
