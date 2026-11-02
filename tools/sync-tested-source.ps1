param([string]$Repository='D:\Reimu Player\FYLM-Plugins')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(!(Test-Path -LiteralPath (Join-Path $Repository '.git'))){throw 'Target must be an existing Git working tree.'}
$receipt=Join-Path $root 'build/014-tests-final.log'
if(!(Select-String -LiteralPath $receipt -Pattern '100% tests passed, 0 tests failed' -Quiet)){throw 'Current automated test receipt has not passed.'}
$dirty=@(& git -C $Repository status --porcelain)
if($LASTEXITCODE){throw 'Cannot inspect Git working tree'}
if($dirty.Count){throw 'Target has changes. Review them before synchronizing; this tool never discards edits.'}
foreach($name in @('CMakeLists.txt','.gitignore','README.md','README.zh-CN.md','README.ja.md','LICENSE','LICENSE.GPL-3.0.txt','NOTICE','core','i18n','modules','plugin','tools','tests','assets','references')){
 Copy-Item -LiteralPath (Join-Path $root $name) -Destination $Repository -Recurse -Force
}
foreach($name in @('BUILDING.md','PUBLICATION-REVIEW.md','FYLM-0.14-REPORT.md')){Copy-Item -LiteralPath (Join-Path $root ('docs/'+$name)) -Destination (Join-Path $Repository 'docs') -Force}
$verified=0
foreach($directory in @('core','i18n','modules','plugin','tools','tests','assets','references')){
 foreach($file in (Get-ChildItem -LiteralPath (Join-Path $root $directory) -Recurse -File)){
  $relative=[IO.Path]::GetRelativePath($root,$file.FullName)
  if((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath (Join-Path $Repository $relative)).Hash){throw ('Git mirror mismatch: '+$relative)}
  $verified++
 }
}
Write-Output ('Verified '+$verified+' source/assets files in '+$Repository+'. Changes are in the working tree; no commit or push was performed.')
& git -C $Repository diff --stat
