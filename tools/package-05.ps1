$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$dest=Join-Path $root 'dist/0.5.0'
New-Item -ItemType Directory -Path $dest -Force | Out-Null
foreach($name in @('FYLM_DitheringStudio.aex','FYLM_CRTVHS.aex','FYLM_ShaderGlass.aex')){
 Copy-Item -LiteralPath (Join-Path $root ('build/plugin/ae/Release/'+$name)) -Destination $dest -Force
}
Copy-Item -LiteralPath (Join-Path $root 'build/modules/shaderglass/Release/FYLM-sgtool.exe') -Destination $dest -Force
$panels=Join-Path $dest 'ScriptUI Panels'
New-Item -ItemType Directory -Path $panels -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'plugin/ae/panel/FYLM Plugins.jsx') -Destination $panels -Force
foreach($name in @('FYLM-i18n.jsxinc','FYLM_DitheringStudio.jsxinc','FYLM_CRTVHS.jsxinc','FYLM_ShaderGlass.jsxinc')){
 Copy-Item -LiteralPath (Join-Path $root ('build/panel/'+$name)) -Destination $panels -Force
}
$assets=Join-Path $dest 'Assets'
New-Item -ItemType Directory -Path $assets -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'assets/ShaderGlass') -Destination $assets -Recurse -Force
Get-ChildItem -LiteralPath (Join-Path $assets 'ShaderGlass') -Filter '*.test.rgba' | Remove-Item
foreach($name in @('README.md','NOTICE','LICENSE','LICENSE.GPL-3.0.txt')){Copy-Item -LiteralPath (Join-Path $root $name) -Destination $dest -Force}
Copy-Item -LiteralPath (Join-Path $root 'modules/retro_gpu/third_party/LICENSE.crt-vhs.txt') -Destination $dest -Force
Copy-Item -LiteralPath (Join-Path $root 'tools/install-fylm.ps1') -Destination (Join-Path $dest 'install.ps1') -Force
New-Item -ItemType Directory -Path (Join-Path $dest 'docs') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'docs/FYLM-0.5-REPORT.md') -Destination (Join-Path $dest 'docs') -Force
$manifest=Get-ChildItem -LiteralPath $dest -File -Recurse | Where-Object Name -ne 'SHA256SUMS.json' | ForEach-Object {
 [pscustomobject]@{path=[IO.Path]::GetRelativePath($dest,$_.FullName);sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $dest 'SHA256SUMS.json') -Encoding utf8
Compress-Archive -Path (Join-Path $dest '*') -DestinationPath (Join-Path $root 'FYLM-Plugins-0.5.0-win-x64.zip') -Force
Write-Output 'Packaged FYLM 0.5.0'
$source=Join-Path $root 'source-release/0.5.0'
New-Item -ItemType Directory -Path $source -Force | Out-Null
foreach($name in @('CMakeLists.txt','README.md','NOTICE','LICENSE','LICENSE.GPL-3.0.txt','AGENTS.md','core','i18n','modules','plugin','tools','tests','assets','references')){
 Copy-Item -LiteralPath (Join-Path $root $name) -Destination $source -Recurse -Force
}
New-Item -ItemType Directory -Path (Join-Path $source 'docs') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'docs/FYLM-0.5-REPORT.md') -Destination (Join-Path $source 'docs') -Force
$third=Join-Path $source 'third_party'
New-Item -ItemType Directory -Path $third -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'third_party/ShaderGlass-master.zip') -Destination $third -Force
Set-Content -LiteralPath (Join-Path $third 'README.txt') -Value 'Extract ShaderGlass-master.zip here before configuring. Archive commit: 12599475101dc4e64eef11469f33efd653586ba2. Compiler dependency sources: https://github.com/KhronosGroup/glslang and https://github.com/KhronosGroup/SPIRV-Cross . Adobe SDK is obtained separately.'
Compress-Archive -Path (Join-Path $source '*') -DestinationPath (Join-Path $root 'FYLM-Plugins-0.5.0-source.zip') -Force
Write-Output 'Packaged corresponding project and upstream source'
