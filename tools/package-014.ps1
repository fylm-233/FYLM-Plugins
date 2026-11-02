param()
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$dest=Join-Path $root 'dist/0.14.0'
if(Test-Path -LiteralPath $dest){throw '0.14.0 staging already exists; preserve/review it before packaging.'}
New-Item -ItemType Directory -Path $dest | Out-Null
foreach($name in @('FYLM_PixelArt.aex','FYLM_CRTVHS.aex','FYLM_ShaderGlass.aex')){Copy-Item -LiteralPath (Join-Path $root ('build/plugin/ae/Release/'+$name)) -Destination $dest}
foreach($name in @('FYLM-sgtool.exe','FYLM-sgprobe.exe')){Copy-Item -LiteralPath (Join-Path $root ('build/modules/shaderglass/Release/'+$name)) -Destination $dest}
$panels=Join-Path $dest 'ScriptUI Panels';New-Item -ItemType Directory -Path $panels | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'plugin/ae/panel/FYLM Plugins.jsx') -Destination $panels
foreach($name in @('FYLM-i18n.jsxinc','FYLM_DitheringStudio.jsxinc','FYLM_CRTVHS.jsxinc','FYLM_ShaderGlass.jsxinc')){Copy-Item -LiteralPath (Join-Path $root ('build/panel/'+$name)) -Destination $panels}
$assets=Join-Path $dest 'Assets';New-Item -ItemType Directory -Path $assets | Out-Null
foreach($name in @('ShaderGlass','palettes','presets')){Copy-Item -LiteralPath (Join-Path $root ('assets/'+$name)) -Destination (Join-Path $assets $name) -Recurse}
foreach($name in @('README.md','README.zh-CN.md','README.ja.md','NOTICE','LICENSE','LICENSE.GPL-3.0.txt')){Copy-Item -LiteralPath (Join-Path $root $name) -Destination $dest}
Copy-Item -LiteralPath (Join-Path $root 'modules/retro_gpu/third_party/LICENSE.crt-vhs.txt') -Destination $dest
Copy-Item -LiteralPath (Join-Path $root 'tools/install-fylm.ps1') -Destination (Join-Path $dest 'install.ps1')
Copy-Item -LiteralPath (Join-Path $root 'tools/install-wrapper.ps1') -Destination $dest
New-Item -ItemType Directory -Path (Join-Path $dest 'docs') | Out-Null
foreach($name in @('BUILDING.md','PUBLICATION-REVIEW.md','FYLM-0.14-REPORT.md')){Copy-Item -LiteralPath (Join-Path $root ('docs/'+$name)) -Destination (Join-Path $dest 'docs')}
$manifest=Get-ChildItem -LiteralPath $dest -File -Recurse | ForEach-Object {[pscustomobject]@{path=[IO.Path]::GetRelativePath($dest,$_.FullName);sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $dest 'SHA256SUMS.json') -Encoding utf8
$zip=Join-Path $root 'FYLM-Plugins-0.14.0-win-x64.zip'
Compress-Archive -Path (Join-Path $dest '*') -DestinationPath $zip
$cs=(Get-Content -LiteralPath (Join-Path $root 'tools/FYLMSetup.cs') -Raw).Replace('PAYLOAD_HASH',(Get-FileHash -LiteralPath $zip).Hash)
$generated=Join-Path $root 'build/FYLMSetup.generated.cs';Set-Content -LiteralPath $generated -Value $cs -Encoding utf8
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$exe=Join-Path $root 'FYLM-Plugins-0.14.0-Setup.exe'
& $compiler /nologo /target:winexe /platform:anycpu /optimize+ "/out:$exe" /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll "/resource:$zip,payload.zip" $generated
if($LASTEXITCODE){throw 'Setup compilation failed'}
Write-Output ('Packaged '+$zip+' and '+$exe)
