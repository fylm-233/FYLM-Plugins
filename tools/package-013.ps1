param([string]$PluginDirectory='build/plugin/ae/Release')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$version='0.13.0'
$dest=Join-Path $root ('dist/'+$version)
New-Item -ItemType Directory -Path $dest -Force | Out-Null
foreach($name in @('FYLM_PixelArt.aex','FYLM_CRTVHS.aex','FYLM_ShaderGlass.aex')){
 Copy-Item -LiteralPath (Join-Path (Join-Path $root $PluginDirectory) $name) -Destination $dest -Force
}
Copy-Item -LiteralPath (Join-Path $root 'build/modules/shaderglass/Release/FYLM-sgtool.exe') -Destination $dest -Force
Copy-Item -LiteralPath (Join-Path $root 'build/modules/shaderglass/Release/FYLM-sgprobe.exe') -Destination $dest -Force
$panels=Join-Path $dest 'ScriptUI Panels'
New-Item -ItemType Directory -Path $panels -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'plugin/ae/panel/FYLM Plugins.jsx') -Destination $panels -Force
foreach($name in @('FYLM-i18n.jsxinc','FYLM_DitheringStudio.jsxinc','FYLM_CRTVHS.jsxinc','FYLM_ShaderGlass.jsxinc')){
 Copy-Item -LiteralPath (Join-Path $root ('build/panel/'+$name)) -Destination $panels -Force
}
$assets=Join-Path $dest 'Assets'
New-Item -ItemType Directory -Path $assets -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'assets/ShaderGlass') -Destination $assets -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'assets/palettes') -Destination $assets -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'assets/presets') -Destination $assets -Recurse -Force
Get-ChildItem -LiteralPath (Join-Path $assets 'ShaderGlass') -Filter '*.test.rgba' | Remove-Item
foreach($name in @('README.md','NOTICE','LICENSE','LICENSE.GPL-3.0.txt')){Copy-Item -LiteralPath (Join-Path $root $name) -Destination $dest -Force}
Copy-Item -LiteralPath (Join-Path $root 'modules/retro_gpu/third_party/LICENSE.crt-vhs.txt') -Destination $dest -Force
Copy-Item -LiteralPath (Join-Path $root 'tools/install-fylm.ps1') -Destination (Join-Path $dest 'install.ps1') -Force
New-Item -ItemType Directory -Path (Join-Path $dest 'docs') -Force | Out-Null
foreach($name in @('FYLM-0.5-REPORT.md','FYLM-0.6-REPORT.md','FYLM-0.7-REPORT.md','FYLM-0.8-REPORT.md','FYLM-0.8.1-REPORT.md','FYLM-0.8.2-REPORT.md','FYLM-0.9-REPORT.md','FYLM-0.9.1-REPORT.md','FYLM-0.9.2-REPORT.md','FYLM-0.10-REPORT.md','FYLM-0.10.1-REPORT.md','FYLM-0.11-REPORT.md','FYLM-0.12-REPORT.md','FYLM-0.12.1-REPORT.md','FYLM-0.12.2-REPORT.md','FYLM-0.13-REPORT.md','PROJECT-STATUS-2026-11-02.md','FYLM-STYLE-PRESETS-PLAN-2026-11-01.md','FYLM-DEVELOPMENT-PLAN.md','REVIEW-0.6-2026-11-01.md')){if(Test-Path -LiteralPath (Join-Path $root ('docs/'+$name))){Copy-Item -LiteralPath (Join-Path $root ('docs/'+$name)) -Destination (Join-Path $dest 'docs') -Force}}
foreach($name in @('0.13-host','0.12.2-host','0.12.1-host','vhs','0.12-host','0.11-host','0.10.1-host','shaderglass','0.10-host','presets','pixel-art','0.9.2-host','0.9.1-host','0.9-host','0.8-host','0.8.1-host','0.8.2-host')){Copy-Item -LiteralPath (Join-Path $root ('docs/'+$name)) -Destination (Join-Path $dest 'docs') -Recurse -Force}
$manifest=Get-ChildItem -LiteralPath $dest -File -Recurse | Where-Object Name -ne 'SHA256SUMS.json' | ForEach-Object {
 [pscustomobject]@{path=[IO.Path]::GetRelativePath($dest,$_.FullName);sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $dest 'SHA256SUMS.json') -Encoding utf8
Compress-Archive -Path (Join-Path $dest '*') -DestinationPath (Join-Path $root ('FYLM-Plugins-'+$version+'-win-x64.zip')) -Force
Write-Output ('Packaged FYLM '+$version)
$source=Join-Path $root ('source-release/'+$version)
New-Item -ItemType Directory -Path $source -Force | Out-Null
foreach($name in @('CMakeLists.txt','README.md','NOTICE','LICENSE','LICENSE.GPL-3.0.txt','AGENTS.md','core','i18n','modules','plugin','tools','tests','assets','references')){
 Copy-Item -LiteralPath (Join-Path $root $name) -Destination $source -Recurse -Force
}
New-Item -ItemType Directory -Path (Join-Path $source 'docs') -Force | Out-Null
foreach($name in @('FYLM-0.5-REPORT.md','FYLM-0.6-REPORT.md','FYLM-0.7-REPORT.md','FYLM-0.8-REPORT.md','FYLM-0.8.1-REPORT.md','FYLM-0.8.2-REPORT.md','FYLM-0.9-REPORT.md','FYLM-0.9.1-REPORT.md','FYLM-0.9.2-REPORT.md','FYLM-0.10-REPORT.md','FYLM-0.10.1-REPORT.md','FYLM-0.11-REPORT.md','FYLM-0.12-REPORT.md','FYLM-0.12.1-REPORT.md','FYLM-0.12.2-REPORT.md','FYLM-0.13-REPORT.md','PROJECT-STATUS-2026-11-02.md','FYLM-STYLE-PRESETS-PLAN-2026-11-01.md','FYLM-DEVELOPMENT-PLAN.md','REVIEW-0.6-2026-11-01.md')){if(Test-Path -LiteralPath (Join-Path $root ('docs/'+$name))){Copy-Item -LiteralPath (Join-Path $root ('docs/'+$name)) -Destination (Join-Path $source 'docs') -Force}}
Copy-Item -LiteralPath (Join-Path $root 'docs/presets') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/pixel-art') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.9.2-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.9.1-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.9-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.8-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.8.1-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.8.2-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/references') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.12.1-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.12-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/vhs') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.11-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.10.1-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.10-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/shaderglass') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.12.2-host') -Destination (Join-Path $source 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'docs/0.13-host') -Destination (Join-Path $source 'docs') -Recurse -Force
$third=Join-Path $source 'third_party'
New-Item -ItemType Directory -Path $third -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'third_party/ShaderGlass-master.zip') -Destination $third -Force
Copy-Item -LiteralPath (Join-Path $root 'third_party/slang-shaders-a4f3aeec04fcb2624ec6df5dd17e38f9b575eab9.zip') -Destination $third -Force
Set-Content -LiteralPath (Join-Path $third 'README.txt') -Value 'Extract ShaderGlass-master.zip here before configuring. Archive commit: 12599475101dc4e64eef11469f33efd653586ba2. Compiler dependency sources: https://github.com/KhronosGroup/glslang and https://github.com/KhronosGroup/SPIRV-Cross . Adobe SDK is obtained separately.'
Compress-Archive -Path (Join-Path $source '*') -DestinationPath (Join-Path $root ('FYLM-Plugins-'+$version+'-source.zip')) -Force
Write-Output 'Packaged corresponding project and upstream source'





