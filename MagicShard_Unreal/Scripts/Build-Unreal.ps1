param(
    [string]$UnrealEditorPath = "",
    [switch]$OpenEditor
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot "MagicShard_Unreal.uproject"
$editor = & (Join-Path $PSScriptRoot "Find-UnrealEditor.ps1") -PreferredPath $UnrealEditorPath
$engineRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $editor))
$buildBat = Join-Path $engineRoot "Build\BatchFiles\Build.bat"

Write-Host "Using UnrealEditor: $editor"
Write-Host "Project: $projectFile"

if (!(Test-Path -LiteralPath $buildBat)) {
    throw "Build.bat was not found at $buildBat"
}

& $buildBat MagicShardEditor Win64 Development "`"$projectFile`"" -WaitMutex -NoHotReloadFromIDE

if ($OpenEditor) {
    Start-Process -FilePath $editor -ArgumentList "`"$projectFile`""
}
