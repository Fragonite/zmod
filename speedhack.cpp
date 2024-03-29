#define NOMINMAX
#define WINMM
#include <Windows.h>
#include <map>
#include <filesystem>
#include "detours.h"
#include "zmod_common.cpp"

typedef BOOL(WINAPI QueryPerformanceCounter_t)(LARGE_INTEGER *lpPerformanceCount);
QueryPerformanceCounter_t *QueryPerformanceCounter_orig = QueryPerformanceCounter;

struct
{
    double speedhack;
} globals;

BOOL WINAPI QueryPerformanceCounter_hook(LARGE_INTEGER *lpPerformanceCount)
{
    auto ret = QueryPerformanceCounter_orig(lpPerformanceCount);
    lpPerformanceCount->QuadPart *= globals.speedhack;
    return ret;
}

void module_main(HINSTANCE hinstDLL)
{
    zmod::ini ini = {
        {{L"zmod_speedhack", L"global_speed_multiplier"}, L"1.0"},
    };

    auto ini_path = zmod::get_module_path(hinstDLL).replace_filename(L"zmod_speedhack.ini");
    if (zmod::file_exists(ini_path))
    {
        zmod::read_ini_file(ini_path, ini);
    }
    else
    {
        zmod::write_ini_file(ini_path, ini);
    }

    globals.speedhack = std::wcstod(ini[{L"zmod_speedhack", L"global_speed_multiplier"}].c_str(), nullptr);

    zmod::set_timer_resolution();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID &)QueryPerformanceCounter_orig, QueryPerformanceCounter_hook);
    DetourTransactionCommit();
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDll)))
            ;
        module_main(hinstDll);
        break;
    }
    return TRUE;
}