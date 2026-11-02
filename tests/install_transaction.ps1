param([string]$Source,[string]$Work)
$ErrorActionPreference='Stop'
$sandbox=Join-Path ([IO.Path]::GetFullPath($Work)) ([guid]::NewGuid().ToString('N'))
$package=Join-Path $sandbox 'package'
New-Item -ItemType Directory -Path $package -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $Source 'tools/install-fylm.ps1') -Destination (Join-Path $package 'install.ps1')
$files=@('FYLM_PixelArt.aex','FYLM_CRTVHS.aex','FYLM_ShaderGlass.aex','FYLM-sgtool.exe','FYLM-sgprobe.exe','ScriptUI Panels/FYLM Plugins.jsx','ScriptUI Panels/FYLM-i18n.jsxinc','ScriptUI Panels/FYLM_DitheringStudio.jsxinc','ScriptUI Panels/FYLM_CRTVHS.jsxinc','ScriptUI Panels/FYLM_ShaderGlass.jsxinc','Assets/ShaderGlass/example.fylmsg')
$files+=('Assets/'+[char]0x65E5+[char]0x5FD7+'/example.txt')
foreach($name in $files){$file=Join-Path $package $name;New-Item -ItemType Directory -Path (Split-Path $file -Parent) -Force | Out-Null;Set-Content -LiteralPath $file -Value ('new '+$name)}
$manifest=foreach($name in $files){[pscustomobject]@{path=$name;sha256=(Get-FileHash -LiteralPath (Join-Path $package $name)).Hash}}
[IO.File]::WriteAllText((Join-Path $package 'SHA256SUMS.json'),($manifest | ConvertTo-Json),[Text.UTF8Encoding]::new($false))
$prior=$env:LOCALAPPDATA;$env:LOCALAPPDATA=Join-Path $sandbox 'local'
# Isolated fixture, deliberately independent of whether the user's real AE is running.
function Get-Process {param($Name,$ErrorAction)}
function Copy-Item {
 [CmdletBinding()]param([string]$LiteralPath,[string]$Destination,[switch]$Force,[switch]$Recurse)
 if($global:fylmTestFault -and $LiteralPath.EndsWith('.stage') -and $Destination.StartsWith($global:fylmTestSupport,[StringComparison]::OrdinalIgnoreCase)){
  $global:fylmTestWrites++;if($global:fylmTestWrites -eq 7){$global:fylmTestFault=$false;throw 'Injected commit failure after first panel write'}
 }
 Microsoft.PowerShell.Management\Copy-Item @PSBoundParameters
}
function Inventory([string]$root){$map=@{};foreach($file in (Get-ChildItem -LiteralPath $root -File -Recurse)){$map[$file.FullName.Substring($root.Length)]=(Get-FileHash -LiteralPath $file.FullName).Hash};return $map}
try{
 foreach($case in @('fresh-fail','upgrade-fail','upgrade-success')){
  $support=Join-Path $sandbox $case;New-Item -ItemType Directory -Path $support -Force | Out-Null
  if($case -ne 'fresh-fail'){
   foreach($name in @('Plug-ins/FYLM Plugins/FYLM_DitheringStudio.aex','Plug-ins/Other/FYLM_DitheringStudio.aex','Plug-ins/FYLM Plugins/Assets/old.txt','Scripts/ScriptUI Panels/FYLM Plugins.jsx')){$file=Join-Path $support $name;New-Item -ItemType Directory -Path (Split-Path $file -Parent) -Force | Out-Null;Set-Content -LiteralPath $file -Value ('old '+$name)}
  }
  $before=Inventory $support;$global:fylmTestSupport=$support;$global:fylmTestWrites=0;$global:fylmTestFault=$case -ne 'upgrade-success';$failed=$false
  try{& (Join-Path $package 'install.ps1') -AfterEffects $support}catch{$failed=$true;if($case -eq 'upgrade-success'){throw}}
  $after=Inventory $support
  if($case -ne 'upgrade-success'){
   if(!$failed -or $before.Count -ne $after.Count){throw "Failed transaction changed file set: $case"}
   foreach($key in $before.Keys){if($before[$key] -ne $after[$key]){throw "Failed transaction changed $key"}}
  }else{if($failed -or !(Test-Path -LiteralPath (Join-Path $support 'Plug-ins/FYLM Plugins/FYLM_PixelArt.aex'))){throw 'Missing new binary'};if(@(Get-ChildItem -LiteralPath (Join-Path $support 'Plug-ins') -Filter FYLM_DitheringStudio.aex -File -Recurse).Count){throw 'Duplicate legacy binary'}}
  Write-Output "PASS transaction $case"
 }
}finally{$env:LOCALAPPDATA=$prior;Remove-Variable fylmTestFault,fylmTestSupport,fylmTestWrites -Scope Global -ErrorAction SilentlyContinue}
