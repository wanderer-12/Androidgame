param([string]$AndroidSdk = "D:\Android\SDK")

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$ndk = Get-ChildItem (Join-Path $AndroidSdk "ndk") -Directory |
    Sort-Object Name -Descending |
    Select-Object -First 1
if (-not $ndk) { throw "No Android NDK found under $AndroidSdk\ndk" }

$compiler = Join-Path $ndk.FullName "toolchains\llvm\prebuilt\windows-x86_64\bin\clang++.exe"
$outputDir = Join-Path $projectRoot "app\build\host-tests"
$output = Join-Path $outputDir "GameLogicTest.exe"
New-Item -ItemType Directory -Force $outputDir | Out-Null

$arguments = @(
    "-std=c++17",
    "-DCANYON_HEADLESS_TEST",
    "-I$projectRoot\app\src\main\cpp",
    "$projectRoot\app\src\test\cpp\GameLogicTest.cpp",
    "$projectRoot\app\src\main\cpp\Game.cpp",
    "-o",
    $output
)
& $compiler $arguments
if ($LASTEXITCODE -ne 0) { throw "GameLogicTest compilation failed" }

& $output
if ($LASTEXITCODE -ne 0) { throw "GameLogicTest failed" }
