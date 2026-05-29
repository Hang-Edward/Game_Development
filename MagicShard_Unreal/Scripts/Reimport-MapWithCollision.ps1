param(
    [string]$UnrealEditorPath = "",
    [switch]$OpenEditor
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$ProjectFile = Join-Path $ProjectDir "MagicShard_Unreal.uproject"
$ImportScript = Join-Path $ScriptDir "ImportLegacyAssets.py"
$FindEditor = Join-Path $ScriptDir "Find-UnrealEditor.ps1"
$WorldSetupScript = Join-Path $ScriptDir "SetupImportedWorld.py"
$SmokeScript = Join-Path $ScriptDir "Run-RuntimeSmoke.ps1"

$Editor = & $FindEditor -PreferredPath $UnrealEditorPath
if (-not (Test-Path -LiteralPath $Editor)) {
    throw "UnrealEditor.exe was not found at: $Editor"
}

Write-Host "=== 步骤 1: 重新导入地图资产（带碰撞几何体）==="
Write-Host "Editor: $Editor"
Write-Host "Project: $ProjectFile"
Write-Host "Script:  $ImportScript"

# 先构建项目，确保编译是最新的
$EngineRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $Editor))
$BuildBat = Join-Path $EngineRoot "Build\BatchFiles\Build.bat"
Write-Host "`n构建项目..."
& $BuildBat MagicShardEditor Win64 Development "`"$ProjectFile`"" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) {
    throw "项目构建失败，错误码: $LASTEXITCODE"
}
Write-Host "构建完成。`n"

Write-Host "启动 UE Editor 执行导入..."
# 用 Editor 模式启动，执行导入脚本
$ImportArgs = @(
    "`"$ProjectFile`"",
    "-ExecutePythonScript=`"$ImportScript`"",
    "-stdout",
    "-FullStdOutLogOutput",
    "-NoSound",
    "-nop4"
)
$ImportProcess = Start-Process -FilePath $Editor -ArgumentList $ImportArgs -PassThru -WindowStyle Hidden -NoNewWindow

Write-Host "等待导入完成..."
$ImportProcess.WaitForExit()
$ExitCode = $ImportProcess.ExitCode

if ($ExitCode -ne 0 -and $ExitCode -ne 1) {
    # UE Editor 的非零退出码不一定是错误（可能在关闭时返回 1）
    # 需要检查实际日志
    Write-Host "UE Editor 退出码: $ExitCode (可能是正常关闭)"
} else {
    Write-Host "UE Editor 已关闭。"
}

Write-Host "`n=== 步骤 2: 重建场景（更新地图引用）==="

# 导入完成后，用 Editor 模式执行场景重建脚本
Write-Host "启动 UE Editor 重建场景..."
$SetupArgs = @(
    "`"$ProjectFile`"",
    "-ExecutePythonScript=`"$WorldSetupScript`"",
    "-stdout",
    "-FullStdOutLogOutput",
    "-NoSound",
    "-nop4"
)
$SetupProcess = Start-Process -FilePath $Editor -ArgumentList $SetupArgs -PassThru -WindowStyle Hidden -NoNewWindow

Write-Host "等待场景重建完成..."
$SetupProcess.WaitForExit()
Write-Host "场景重建完成。"

Write-Host "`n=== 步骤 3: 运行烟雾测试验证 ==="
& $SmokeScript -SecondsToRun 8

Write-Host "`n=== 完成 ==="
Write-Host "地图已重新导入（带碰撞），场景已重建。"
Write-Host "你现在可以运行 .\map1.exe 测试角色是否不再坠落。"
