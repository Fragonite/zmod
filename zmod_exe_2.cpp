#include <Windows.h>
#include <map>
#include <filesystem>
#include <fstream>
#include "detours.h"
#include "zmod_common.cpp"

std::string get_appid(std::filesystem::path dir)
{
    std::map<std::wstring, std::string> appids = {
        {L"steamapps/common/Dynasty Warriors 8", "278080"},
        {L"steamapps/common/WARRIORS OROCHI 4", "831560"},
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
        MessageBoxW(NULL, L"zmod_32.dll not found!", L"zmod", MB_OK);
        return 0;
    }
    if (!(zmod::file_exists(dll_64_path)))
    {
        MessageBoxW(NULL, L"zmod_64.dll not found!", L"zmod", MB_OK);
        return 0;
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
        MessageBoxW(NULL, L"Target not found!", L"zmod", MB_OK);
        return 0;
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

    MessageBoxW(NULL, L"Hello, World!", L"zmod_exe_2", MB_OK);
    return 0;
}