param(
    [string]$UnrealEditorPath = ""
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$ProjectFile = Join-Path $ProjectDir "MagicShard_Unreal.uproject"
$PythonScript = Join-Path $ScriptDir "ReimportMapAndRebuildWorld.py"
$FindEditor = Join-Path $ScriptDir "Find-UnrealEditor.ps1"

# Fix: ensure UTF-8 encoding for this script
$PSDefaultParameterValues['*:Encoding'] = 'utf8'

$Editor = & $FindEditor -PreferredPath $UnrealEditorPath
if (-not (Test-Path -LiteralPath $Editor)) {
    throw "Cannot find UnrealEditor.exe: $Editor"
}

# Build the project first
$EngineRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $Editor))
$BuildBat = Join-Path $EngineRoot "Build\BatchFiles\Build.bat"

Write-Host "============================================"
Write-Host "  Map Reimport + World Rebuild"
Write-Host "============================================"
Write-Host "`nStep 1/2: Building project..."
& $BuildBat MagicShardEditor Win64 Development "`"$ProjectFile`"" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code: $LASTEXITCODE" }
Write-Host "Build complete.`n"

Write-Host "Step 2/2: Launching UE Editor to run Python script..."
Write-Host "  - Reimport map (with auto_generate_collision = True)"
Write-Host "  - Rebuild scene Map01_Forest"
Write-Host "`nUE Editor will open now. It will run the script and save automatically."
Write-Host "Press Enter to continue..."
$null = Read-Host

Start-Process -FilePath $Editor -ArgumentList @(
    "`"$ProjectFile`"",
    "-ExecutePythonScript=`"$PythonScript`"",
    "-stdout",
    "-FullStdOutLogOutput",
    "-NoSound",
    "-nop4"
) -Wait

Write-Host "`nUE Editor has been closed."
Write-Host "The map has been reimported with collision geometry."
Write-Host "Run .\map1.exe to test if the character no longer falls through the ground."

if ($LASTEXITCODE -ne 0) {
    Write-Host "NOTE: UE Editor exited with code $LASTEXITCODE. If there were errors, please share the output."
}
