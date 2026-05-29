$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$ProjectFile = Join-Path $ProjectDir "MagicShard_Unreal.uproject"
$FindEditor = Join-Path $ScriptDir "Find-UnrealEditor.ps1"

$Editor = & $FindEditor
if (-not (Test-Path -LiteralPath $Editor)) {
    throw "UnrealEditor.exe was not found: $Editor"
}

if (-not (Test-Path -LiteralPath $ProjectFile)) {
    throw "Project file was not found: $ProjectFile"
}

Write-Host "Launching MagicShard Unreal project..."
Write-Host "Editor: $Editor"
Write-Host "Project: $ProjectFile"

Start-Process -FilePath $Editor -ArgumentList @("`"$ProjectFile`"")
