param(
    [int]$SecondsToRun = 10
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$ProjectFile = Join-Path $ProjectDir "MagicShard_Unreal.uproject"
$MapName = "/Game/Maps/Map01_Forest"
$LogFile = Join-Path $ProjectDir "Saved\Logs\MagicShard_Unreal.log"
$SaveFile = Join-Path $ProjectDir "Saved\MagicShard\PlayerRecords.dat"

$FindEditor = Join-Path $ScriptDir "Find-UnrealEditor.ps1"
$Editor = & $FindEditor
if (-not (Test-Path -LiteralPath $Editor)) {
    throw "UnrealEditor.exe was not found: $Editor"
}

Write-Host "Starting runtime smoke test with UnrealEditor: $Editor"
Write-Host "Project: $ProjectFile"
Write-Host "Map: $MapName"

$Arguments = @(
    "`"$ProjectFile`"",
    $MapName,
    "-game",
    "-windowed",
    "-ResX=800",
    "-ResY=450",
    "-NoSound",
    "-nop4"
)

$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -PassThru -WindowStyle Hidden
Start-Sleep -Seconds $SecondsToRun

if (-not $Process.HasExited) {
    Stop-Process -Id $Process.Id
}

if (-not (Test-Path -LiteralPath $LogFile)) {
    throw "Runtime smoke log was not found: $LogFile"
}

$RequiredPatterns = @(
    "\[MagicShardSmoke\] SaveSubsystem initialized",
    "\[MagicShardSmoke\] GameMode BeginPlay",
    "\[MagicShardSmoke\] HUD BeginPlay",
    "\[MagicShardSmoke\] PlayerCharacter BeginPlay"
)

$LogText = Get-Content -LiteralPath $LogFile -Raw
foreach ($Pattern in $RequiredPatterns) {
    if ($LogText -notmatch $Pattern) {
        throw "Runtime smoke pattern missing: $Pattern"
    }
}

if (-not (Test-Path -LiteralPath $SaveFile)) {
    throw "Random-access save file was not created: $SaveFile"
}

$SaveInfo = Get-Item -LiteralPath $SaveFile
if ($SaveInfo.Length -le 0) {
    throw "Random-access save file is empty: $SaveFile"
}

Write-Host "Runtime smoke test passed."
Write-Host "Save file: $($SaveInfo.FullName) ($($SaveInfo.Length) bytes)"
