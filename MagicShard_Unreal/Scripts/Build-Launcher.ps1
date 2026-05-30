$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$SourceFile = Join-Path $ProjectDir "Launcher\map1_launcher.cpp"
$OutputFile = Join-Path $ProjectDir "main.exe"

if (-not (Test-Path -LiteralPath $SourceFile)) {
    throw "Launcher source was not found: $SourceFile"
}

$Gpp = (Get-Command g++.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source)
if (-not $Gpp) {
    throw "g++.exe was not found on PATH."
}

Write-Host "Compiler: $Gpp"
Write-Host "Source: $SourceFile"
Write-Host "Output: $OutputFile"

& $Gpp -std=c++17 -O2 -municode -mwindows "$SourceFile" -o "$OutputFile" -lshell32
if ($LASTEXITCODE -ne 0) {
    throw "Launcher build failed with exit code $LASTEXITCODE."
}

Write-Host "Built launcher: $OutputFile"
