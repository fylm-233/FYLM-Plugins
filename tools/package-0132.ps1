param()
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Join-Path $root 'build/github-source-0.13.2'
$dest=Join-Path $root 'dist/0.13.2'
$zip=Join-Path $root 'FYLM-Plugins-0.13.2-win-x64.zip'
if(!(Test-Path -LiteralPath $source)){throw 'Prepare build/github-source-0.13.2 first.'}
if((Test-Path -LiteralPath $dest) -or (Test-Path -LiteralPath $zip)){throw '0.13.2 output already exists; inspect it before packaging again.'}
New-Item -ItemType Directory -Path $dest | Out-Null
$binaries=Join-Path $root 'build/plugin/ae/Release'
foreach($name in @('FYLM_PixelArt.aex','FYLM_CRTVHS.aex','FYLM_ShaderGlass.aex')){Copy-Item -LiteralPath (Join-Path $binaries $name) -Destination $dest}
$tools=Join-Path $root 'build/modules/shaderglass/Release'
foreach($name in @('FYLM-sgtool.exe','FYLM-sgprobe.exe')){Copy-Item -LiteralPath (Join-Path $tools $name) -Destination $dest}
$panels=Join-Path $dest 'ScriptUI Panels';New-Item -ItemType Directory -Path $panels | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'plugin/ae/panel/FYLM Plugins.jsx') -Destination $panels
foreach($name in @('FYLM-i18n.jsxinc','FYLM_DitheringStudio.jsxinc','FYLM_CRTVHS.jsxinc','FYLM_ShaderGlass.jsxinc')){Copy-Item -LiteralPath (Join-Path $root ('build/panel/'+$name)) -Destination $panels}
$assets=Join-Path $dest 'Assets';New-Item -ItemType Directory -Path $assets | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'assets/ShaderGlass') -Destination (Join-Path $assets 'ShaderGlass') -Recurse
Copy-Item -LiteralPath (Join-Path $source 'assets/palettes') -Destination $assets -Recurse
Copy-Item -LiteralPath (Join-Path $source 'assets/presets') -Destination $assets -Recurse
Get-ChildItem -LiteralPath (Join-Path $dest 'Assets/ShaderGlass') -Filter '*.test.rgba' | Remove-Item
foreach($name in @('README.md','README.zh-CN.md','README.ja.md','NOTICE','LICENSE','LICENSE.GPL-3.0.txt')){Copy-Item -LiteralPath (Join-Path $source $name) -Destination $dest}
Copy-Item -LiteralPath (Join-Path $root 'modules/retro_gpu/third_party/LICENSE.crt-vhs.txt') -Destination $dest
Copy-Item -LiteralPath (Join-Path $root 'tools/install-fylm.ps1') -Destination (Join-Path $dest 'install.ps1')
New-Item -ItemType Directory -Path (Join-Path $dest 'docs') | Out-Null
foreach($name in @('PUBLICATION-REVIEW.md','BUILDING.md')){Copy-Item -LiteralPath (Join-Path $source ('docs/'+$name)) -Destination (Join-Path $dest 'docs')}
$manifest=Get-ChildItem -LiteralPath $dest -File -Recurse | Where-Object Name -ne 'SHA256SUMS.json' | ForEach-Object {[pscustomobject]@{path=[IO.Path]::GetRelativePath($dest,$_.FullName);sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}}
$manifest | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $dest 'SHA256SUMS.json') -Encoding utf8
Compress-Archive -Path (Join-Path $dest '*') -DestinationPath $zip
Write-Output ('Created '+$zip+' with '+@($manifest).Count+' manifest entries.')
