param()
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
$dest=Join-Path $root 'build/github-source-0.13.2'
if(Test-Path -LiteralPath $dest){throw 'Source candidate already exists. Review or move it before running again; this script never deletes a tree.'}
New-Item -ItemType Directory -Path $dest | Out-Null
foreach($name in @('CMakeLists.txt','README.md','README.zh-CN.md','README.ja.md','NOTICE','LICENSE','LICENSE.GPL-3.0.txt','.gitignore','core','i18n','modules','plugin','tools','tests','assets','references')){
 Copy-Item -LiteralPath (Join-Path $root $name) -Destination $dest -Recurse -Force
}
New-Item -ItemType Directory -Path (Join-Path $dest 'docs') | Out-Null
foreach($name in @('BUILDING.md','PUBLICATION-REVIEW.md')){Copy-Item -LiteralPath (Join-Path $root ('docs/'+$name)) -Destination (Join-Path $dest 'docs')}
New-Item -ItemType Directory -Path (Join-Path $dest 'third_party') | Out-Null
Set-Content -LiteralPath (Join-Path $dest 'third_party/README.md') -Encoding utf8 -Value 'Run ./tools/fetch-shaderglass.ps1 from the repository root as described in docs/BUILDING.md. Dependencies and the Adobe SDK are not bundled in this source candidate.'
$files=Get-ChildItem -LiteralPath $dest -Recurse -File
foreach($file in $files){
 $relative=[IO.Path]::GetRelativePath($dest,$file.FullName)
 if($relative -match '(?i)p1-private|092-private|sg-art-private|vhs-private|reference-metadata|119751990|143385443|(^|[\\/])\.env($|\.)|\.aepx?$|\.aex$|\.pdb$|\.zip$|(^|[\\/])AE_Effect\.h$'){throw ('Excluded file in candidate: '+$relative)}
 if($file.Length -ge 100MB){throw ('File exceeds source limit: '+$relative)}
}
$inventory=@($files | Sort-Object FullName | ForEach-Object { [pscustomobject]@{path=[IO.Path]::GetRelativePath($dest,$_.FullName);bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash} })
$inventory | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $root 'build/github-source-0.13.2.inventory.json') -Encoding utf8
Write-Output ('Prepared '+$inventory.Count+' files at '+$dest+'. Review PUBLICATION-REVIEW.md before uploading.')
