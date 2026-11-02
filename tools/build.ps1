param([string]$AeSdkRoot=$env:AE_SDK_ROOT,[switch]$CoreOnly)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$cmake=(Get-Command cmake -ErrorAction SilentlyContinue).Source
if(!$cmake){
 $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
 $vs=& $vswhere -latest -property installationPath
 $cmake=Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}
if(!(Test-Path -LiteralPath $cmake)){throw 'CMake 3.21+ is required.'}
$build=Join-Path $root 'build'
$options=@('-S',$root,'-B',$build)
if($CoreOnly){$options+='-DBUILD_AE_PLUGIN=OFF'}else{
 if(!$AeSdkRoot){throw 'Pass -AeSdkRoot or set AE_SDK_ROOT; use -CoreOnly without the Adobe SDK.'}
 $options+=@('-DBUILD_AE_PLUGIN=ON',"-DAE_SDK_ROOT=$AeSdkRoot")
}
& $cmake @options
if($LASTEXITCODE){throw 'Configure failed'}
foreach($configuration in @('Debug','Release')){
 & $cmake --build $build --config $configuration
 if($LASTEXITCODE){throw "Build failed: $configuration"}
 & (Join-Path (Split-Path $cmake) 'ctest.exe') --test-dir $build -C $configuration --output-on-failure
 if($LASTEXITCODE){throw "Tests failed: $configuration"}
}
