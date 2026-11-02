param()
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$third=Join-Path $root 'third_party'
$target=Join-Path $third 'ShaderGlass-master'
$commit='12599475101dc4e64eef11469f33efd653586ba2'
if(Test-Path -LiteralPath $target){throw 'ShaderGlass-master already exists. Keep it or move it aside before fetching; this script never replaces an existing dependency.'}
New-Item -ItemType Directory -Path $third -Force | Out-Null
$temp=Join-Path $third ('fetch-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
$archive=Join-Path $temp 'source.zip'
Invoke-WebRequest -Uri ('https://github.com/mausimus/ShaderGlass/archive/'+$commit+'.zip') -OutFile $archive
Expand-Archive -LiteralPath $archive -DestinationPath $temp
$source=Join-Path $temp ('ShaderGlass-'+$commit)
foreach($relative in @('ShaderGC/ShaderGC.cpp','ShaderGC/include/json.hpp','lib/glslang.lib')){
 if(!(Test-Path -LiteralPath (Join-Path $source $relative))){throw ('Incomplete dependency: '+$relative)}
}
Move-Item -LiteralPath $source -Destination $target
Write-Output ('Fetched ShaderGlass commit '+$commit+'. Download retained at '+$archive)
