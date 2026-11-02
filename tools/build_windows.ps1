param(
    [Parameter(Mandatory=$true)][string]$ObsIncludeDir,
    [Parameter(Mandatory=$true)][string]$ObsImportLibrary,
    [string]$ObsGeneratedIncludeDir = ""
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build/windows-x64"
$stageDir = Join-Path $root "staging"
$arguments = @("-S", $root, "-B", $buildDir, "-A", "x64",
    "-DOBS_INCLUDE_DIR=$ObsIncludeDir", "-DOBS_IMPORT_LIBRARY=$ObsImportLibrary")
if ($ObsGeneratedIncludeDir) { $arguments += "-DOBS_GENERATED_INCLUDE_DIR=$ObsGeneratedIncludeDir" }
& cmake @arguments
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed" }
& cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
& ctest --test-dir $buildDir -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
& cmake --install $buildDir --config Release --prefix $stageDir
if ($LASTEXITCODE -ne 0) { throw "Staging failed" }
Write-Host "Staged DLL: $stageDir/OBS-Clock/OBS-Clock.dll"
