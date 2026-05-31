#include <windows.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
// 这个启动器的目标很单纯：像旧版 map1.exe 一样，一键打开当前 Unreal 项目。
// 默认使用 D3D11，避免本机 UE 5.7 + D3D12 驱动组合在启动时弹 Crash Reporter。
constexpr const wchar_t* kKnownEditorPath = L"D:\\Unreal Engine 5.7\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor.exe";
constexpr const wchar_t* kProjectFileName = L"MagicShard_Unreal.uproject";

std::filesystem::path GetExecutableDirectory()
{
    std::vector<wchar_t> Buffer(MAX_PATH);
    DWORD Length = GetModuleFileNameW(nullptr, Buffer.data(), static_cast<DWORD>(Buffer.size()));
    while (Length == Buffer.size())
    {
        Buffer.resize(Buffer.size() * 2);
        Length = GetModuleFileNameW(nullptr, Buffer.data(), static_cast<DWORD>(Buffer.size()));
    }

    if (Length == 0)
    {
        return std::filesystem::current_path();
    }

    return std::filesystem::path(std::wstring(Buffer.data(), Length)).parent_path();
}

std::filesystem::path FindProjectFile(const std::filesystem::path& LauncherDir)
{
    const std::filesystem::path SameDir = LauncherDir / kProjectFileName;
    if (std::filesystem::exists(SameDir))
    {
        return SameDir;
    }

    const std::filesystem::path ParentDir = LauncherDir.parent_path() / kProjectFileName;
    if (std::filesystem::exists(ParentDir))
    {
        return ParentDir;
    }

    return SameDir;
}

std::filesystem::path FindEditor()
{
    if (std::filesystem::exists(kKnownEditorPath))
    {
        return kKnownEditorPath;
    }

    const wchar_t* EnvironmentPath = _wgetenv(L"UNREAL_EDITOR");
    if (EnvironmentPath != nullptr && std::filesystem::exists(EnvironmentPath))
    {
        return EnvironmentPath;
    }

    return kKnownEditorPath;
}

std::wstring Quote(const std::filesystem::path& Path)
{
    return L"\"" + Path.wstring() + L"\"";
}
}

int wmain()
{
    const std::filesystem::path LauncherDir = GetExecutableDirectory();
    const std::filesystem::path ProjectFile = FindProjectFile(LauncherDir);
    const std::filesystem::path Editor = FindEditor();

    if (!std::filesystem::exists(ProjectFile))
    {
        std::wcerr << L"Project file not found: " << ProjectFile.wstring() << L"\n";
        std::wcerr << L"Please keep main.exe in the MagicShard_Unreal folder.\n";
        return 1;
    }

    if (!std::filesystem::exists(Editor))
    {
        std::wcerr << L"UnrealEditor.exe not found: " << Editor.wstring() << L"\n";
        std::wcerr << L"You can set UNREAL_EDITOR to the full UnrealEditor.exe path.\n";
        return 1;
    }

    const std::wstring Parameters = Quote(ProjectFile) + L" -d3d11 -NoSound -nop4 -NoLiveCoding";

    SHELLEXECUTEINFOW ExecuteInfo{};
    ExecuteInfo.cbSize = sizeof(ExecuteInfo);
    ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ExecuteInfo.lpVerb = L"open";
    ExecuteInfo.lpFile = Editor.c_str();
    ExecuteInfo.lpParameters = Parameters.c_str();
    ExecuteInfo.lpDirectory = LauncherDir.c_str();
    ExecuteInfo.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&ExecuteInfo))
    {
        const DWORD Error = GetLastError();
        std::wcerr << L"Failed to launch Unreal Editor. Win32 error: " << Error << L"\n";
        return static_cast<int>(Error);
    }

    std::wcout << L"Launching MagicShard Unreal project...\n";
    std::wcout << L"Editor: " << Editor.wstring() << L"\n";
    std::wcout << L"Project: " << ProjectFile.wstring() << L"\n";
    std::wcout << L"RHI: D3D11\n";
    return 0;
}
