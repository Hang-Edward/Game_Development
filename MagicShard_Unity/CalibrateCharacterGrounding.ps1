param(
    [string]$UnityPath = ""
)

$ErrorActionPreference = "Stop"
$ProjectPath = Split-Path -Parent $MyInvocation.MyCommand.Path

function Find-Unity {
    param([string]$ExplicitPath)

    if ($ExplicitPath -and (Test-Path -LiteralPath $ExplicitPath)) {
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }

    $candidateRoots = @(
        "C:\Program Files\Unity\Hub\Editor",
        "C:\Program Files\Unity",
        "D:\Program Files\Unity\Hub\Editor",
        "D:\Unity\Hub\Editor"
    )

    foreach ($root in $candidateRoots) {
        if (-not (Test-Path -LiteralPath $root)) {
            continue
        }

        $match = Get-ChildItem -LiteralPath $root -Recurse -Filter Unity.exe -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1

        if ($match) {
            return $match.FullName
        }
    }

    $cmd = Get-Command Unity.exe -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    return ""
}

$unity = Find-Unity -ExplicitPath $UnityPath
if (-not $unity) {
    throw "Unity.exe was not found. Run this script with -UnityPath 'C:\Path\To\Unity.exe'."
}

& $unity `
    -batchmode `
    -quit `
    -projectPath $ProjectPath `
    -executeMethod CharacterGroundingCalibrator.CalibrateCharacterGroundingBatch `
    -logFile "$ProjectPath\Logs\character-grounding-calibration.log"

if ($LASTEXITCODE -ne 0) {
    throw "Unity calibration failed. See Logs\character-grounding-calibration.log"
}

Write-Host "Character grounding calibration complete."
