#include <Windows.h>
#include <map>
#include <filesystem>
#include <fstream>
#include "detours.h"
#include "zmod_common.cpp"

std::string get_appid(std::filesystem::path dir)
{
    std::map<std::wstring, std::string> appids = {
        {L"steamapps\\common\\Dynasty Warriors 8", "278080"},
        {L"steamapps\\common\\WARRIORS OROCHI 4", "831560"},
    };

    auto d = dir.wstring();
    for (const auto &[installdir, appid] : appids)
    {
        if (d.find(installdir) != std::string::npos)
        {
            return appid;
        }
    }

    return "";
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    zmod::ini ini = {{{L"zmod", L"target"}, L"Launch.exe"}}; // default values

    auto app_path = zmod::get_module_path(NULL);
    auto ini_path = app_path.replace_filename(L"zmod.ini");
    auto dll_32_path = app_path.replace_filename(L"zmod_32.dll");
    auto dll_64_path = app_path.replace_filename(L"zmod_64.dll");

    if (!(zmod::file_exists(dll_32_path)))
    {
        MessageBoxW(NULL, (std::wstring(L"Missing file: ") + dll_32_path.wstring()).c_str(), L"zmod", MB_OK);
        return 1;
    }
    if (!(zmod::file_exists(dll_64_path)))
    {
        MessageBoxW(NULL, (std::wstring(L"Missing file: ") + dll_64_path.wstring()).c_str(), L"zmod", MB_OK);
        return 1;
    }

    if (zmod::file_exists(ini_path))
    {
        zmod::read_ini_file(ini_path, ini);
    }
    else
    {
        zmod::write_ini_file(ini_path, ini);
    }

    auto target = std::filesystem::current_path() / ini[{L"zmod", L"target"}];

    if (!(zmod::file_exists(target)))
    {
        MessageBoxW(NULL, (std::wstring(L"Missing file: ") + target.wstring()).c_str(), L"zmod", MB_OK);
        return 1;
    }

    auto steam_appid = target.parent_path() / L"steam_appid.txt";
    if (!(zmod::file_exists(steam_appid)))
    {
        auto appid = get_appid(target.parent_path());
        if (!(appid.empty()))
        {
            std::ofstream(steam_appid) << appid;
        }
    }

    STARTUPINFOW si{.cb = sizeof(si)};
    PROCESS_INFORMATION pi{};

    auto result = DetourCreateProcessWithDllExW(
        target.wstring().c_str(),
        NULL,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        target.parent_path().wstring().c_str(),
        &si,
        &pi,
        dll_32_path.string().c_str(),
        NULL);

    if (!(result))
    {
        MessageBoxW(NULL, L"Failed to launch target!", L"zmod", MB_OK);
        return 1;
    }

    return 0;
}