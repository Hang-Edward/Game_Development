param(
    [string]$PreferredPath = ""
)

$ErrorActionPreference = "Stop"

if ($PreferredPath -and (Test-Path -LiteralPath $PreferredPath)) {
    Resolve-Path -LiteralPath $PreferredPath
    exit 0
}

$candidates = @(
    "D:\Unreal Engine 5.7\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe",
    "C:\Program Files\Epic Games\UE_5.4\Engine\Binaries\Win64\UnrealEditor.exe",
    "C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor.exe",
    "C:\Program Files\Epic Games\UE_5.2\Engine\Binaries\Win64\UnrealEditor.exe",
    "D:\Epic Games\UE_5.4\Engine\Binaries\Win64\UnrealEditor.exe",
    "D:\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor.exe",
    "D:\Unreal\UE_5.4\Engine\Binaries\Win64\UnrealEditor.exe",
    "D:\Unreal\UE_5.3\Engine\Binaries\Win64\UnrealEditor.exe"
)

foreach ($candidate in $candidates) {
    if (Test-Path -LiteralPath $candidate) {
        Resolve-Path -LiteralPath $candidate
        exit 0
    }
}

$cmd = Get-Command UnrealEditor.exe -ErrorAction SilentlyContinue
if ($cmd) {
    $cmd.Source
    exit 0
}

throw "UnrealEditor.exe was not found. Install Unreal Engine 5.3+ or pass -UnrealEditorPath to Build-Unreal.ps1."
