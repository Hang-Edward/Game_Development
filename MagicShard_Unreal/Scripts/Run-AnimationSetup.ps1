param(
    [string]$UnrealEditorPath = ""
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot "MagicShard_Unreal.uproject"
$scriptFile = Join-Path $projectRoot "Scripts\SetupCharacterAnimations.py"
$logFile = Join-Path $projectRoot "Intermediate\AnimSetup.log"

$editor = & (Join-Path $PSScriptRoot "Find-UnrealEditor.ps1") -PreferredPath $UnrealEditorPath

Write-Host "Editor: $editor"
Write-Host "Project: $projectFile"
Write-Host "Script: $scriptFile"

if (!(Test-Path -LiteralPath $scriptFile)) {
    throw "Python script not found: $scriptFile"
}

# Build C++ first if needed
$dllPath = Join-Path $projectRoot "Binaries\Win64\UnrealEditor-MagicShard.dll"
if (!(Test-Path -LiteralPath $dllPath)) {
    Write-Host "Building C++ project first..." -ForegroundColor Yellow
    & (Join-Path $PSScriptRoot "Build-Unreal.ps1") -UnrealEditorPath $UnrealEditorPath
}

Write-Host "Launching UE Editor (headless) to run animation setup..." -ForegroundColor Cyan

$proc = Start-Process -FilePath $editor -ArgumentList @(
    "`"$projectFile`"",
    "-RunPythonScript=`"$scriptFile`"",
    "-NullRHI",
    "-NoOutput",
    "-Log=`"$logFile`"",
    "-Unattended",
    "-RespawnAfterCrash=0"
) -NoNewWindow -Wait -PassThru

if ($proc.ExitCode -ne 0) {
    Write-Host "Editor exited with code $($proc.ExitCode)" -ForegroundColor Yellow
    Write-Host "This may be due to GPU/RHI compatibility."
    Write-Host ""
    Write-Host "Please open the Editor manually and run in Python Console:"
    Write-Host "  exec(open('Scripts/SetupCharacterAnimations.py').read())"
    exit 1
}

if (Test-Path -LiteralPath $logFile) {
    Write-Host "=== Last 30 log lines ===" -ForegroundColor Cyan
    Get-Content -LiteralPath $logFile -Tail 30
    Write-Host "=========================" -ForegroundColor Cyan
}

Write-Host "Animation setup complete!" -ForegroundColor Green
