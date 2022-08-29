#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <filesystem>
#include <map>
#include "io_helper.cpp"
#include "detours.h"

struct
{
    ini_map iniMap;
} static globals;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    std::wstring moduleName;
    moduleName.reserve(MAX_PATH);
    if (!(GetModuleFileNameW(nullptr, moduleName.data(), moduleName.capacity())))
        ;
    auto modulePath = std::filesystem::path(moduleName.c_str());
    auto iniPath = modulePath.replace_extension(L".ini");
    std::wstring moduleKey = modulePath.stem().c_str();

    globals.iniMap = {
        {{moduleKey, L"target"}, L"Launch.exe"},
        {{moduleKey, L"dll"}, L"zmod_32.dll"},
    };

    if (!(file_exists(iniPath)))
    {
        create_ini_file(iniPath, globals.iniMap);
    }
    read_ini_file(iniPath, globals.iniMap);

    auto cwd = std::filesystem::current_path();
    auto target = cwd / globals.iniMap[{moduleKey, L"target"}];
    if (!(file_exists(target)))
    {
        MessageBoxW(nullptr, (std::wstring(L"Failed to find target file: ") + target.wstring()).c_str(), moduleKey.c_str(), MB_OK);
        return 1;
    }

    auto dll = target.parent_path() / globals.iniMap[{moduleKey, L"dll"}];
    if (!(file_exists(dll)))
    {
        MessageBoxW(nullptr, (std::wstring(L"Failed to find DLL file: ") + dll.wstring()).c_str(), moduleKey.c_str(), MB_OK);
        return 1;
    }

    STARTUPINFOW si = {
        .cb = sizeof(si),
    };
    PROCESS_INFORMATION pi;
    if (!(DetourCreateProcessWithDllExW(target.wstring().c_str(), NULL, NULL, NULL, FALSE, 0, NULL, target.parent_path().wstring().c_str(), &si, &pi, dll.string().c_str(), NULL)))
    {
        MessageBoxW(nullptr, (std::wstring(L"Failed to create process: ") + target.wstring()).c_str(), moduleKey.c_str(), MB_OK);
        return 1;
    }
    return 0;
}