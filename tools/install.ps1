param([string]$AfterEffects='C:\Program Files\Adobe\Adobe After Effects 2026\Support Files')
$ErrorActionPreference='Stop'
$source=Join-Path $PSScriptRoot 'DitheringStudio.aex'
if(!(Test-Path -LiteralPath $source)){throw 'DitheringStudio.aex must be beside this installer.'}
if(Get-Process AfterFX*,aerender* -ErrorAction SilentlyContinue){throw 'Please close After Effects and aerender before installing.'}
$target=Join-Path $AfterEffects 'Plug-ins\DitheringStudio'
New-Item -ItemType Directory -Force -Path $target | Out-Null
Copy-Item -LiteralPath $source -Destination (Join-Path $target 'DitheringStudio.aex')
$panels=Join-Path $PSScriptRoot 'ScriptUI Panels'
if(Test-Path -LiteralPath $panels){
 $panelTarget=Join-Path $AfterEffects 'Scripts\ScriptUI Panels'
 New-Item -ItemType Directory -Force -Path $panelTarget | Out-Null
 foreach($name in @('Dithering Studio.jsx','DitheringStudio-i18n.jsxinc')){
  Copy-Item -LiteralPath (Join-Path $panels $name) -Destination (Join-Path $panelTarget $name)
 }
}
Write-Host "Installed: $target\DitheringStudio.aex"
