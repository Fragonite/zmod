#include <Windows.h>
#include <map>
#include <filesystem>
#include "detours.h"
#include "zmod_common.cpp"

typedef void(fn_7024D0_t)(LARGE_INTEGER *lpPerformanceCount, uint64_t vsync_30);
fn_7024D0_t wait_loop;

struct
{
    double dt_target;
    double dt_sleep;
    double frequency;
} globals;

void wait_loop(LARGE_INTEGER *lpPerformanceCount, uint64_t vsync_30)
{
    auto dt_target = globals.dt_target;
    auto dt_sleep = globals.dt_sleep;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    auto dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;
    if (dt < dt_target)
    {
        while (dt < dt_sleep)
        {
            Sleep(1);
            QueryPerformanceCounter(&now);
            dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;
        }
        while (dt < dt_target)
        {
            QueryPerformanceCounter(&now);
            dt = (double)(now.QuadPart - lpPerformanceCount->QuadPart) / globals.frequency;
        }
        lpPerformanceCount->QuadPart = lpPerformanceCount->QuadPart + (LONGLONG)(dt_target * globals.frequency); // Prevent drift
    }
    else if (dt < dt_target * 2.0)
    {
        lpPerformanceCount->QuadPart = lpPerformanceCount->QuadPart + (LONGLONG)(dt_target * globals.frequency); // Catch up if less than 2 frames behind
    }
    else
    {
        lpPerformanceCount->QuadPart = now.QuadPart; // Else prevent the game from speeding up
    }
}

void set_timer_resolution()
{
    timeBeginPeriod(1);

    typedef NTSTATUS(CALLBACK * NtQueryTimerResolution_t)(
        OUT PULONG MaximumResolution,
        OUT PULONG MinimumResolution,
        OUT PULONG CurrentResolution);

    typedef NTSTATUS(CALLBACK * NtSetTimerResolution_t)(
        IN ULONG DesiredResolution,
        IN BOOLEAN SetResolution,
        OUT PULONG CurrentResolution);

    auto hModule = LoadLibraryA("ntdll.dll");
    NtQueryTimerResolution_t NtQueryTimerResolution = (NtQueryTimerResolution_t)GetProcAddress(hModule, "NtQueryTimerResolution");
    NtSetTimerResolution_t NtSetTimerResolution = (NtSetTimerResolution_t)GetProcAddress(hModule, "NtSetTimerResolution");

    ULONG MaximumResolution, MinimumResolution, CurrentResolution;
    NtQueryTimerResolution(&MaximumResolution, &MinimumResolution, &CurrentResolution);
    NtSetTimerResolution(MinimumResolution, TRUE, &CurrentResolution);
}

void module_main(HINSTANCE hinstDLL)
{
    zmod::ini ini = {
        {{L"zmod_wo4u_performance", L"clock_speed"}, L"1.0"},
    };

    auto ini_path = zmod::get_module_path(hinstDLL).replace_filename(L"zmod_wo4u_performance.ini");
    if (zmod::file_exists(ini_path))
    {
        zmod::read_ini_file(ini_path, ini);
    }
    else
    {
        zmod::write_ini_file(ini_path, ini);
    }

    auto dt_target = 1.0 / 60.0 * std::wcstod(ini[{L"zmod_wo4u_performance", L"clock_speed"}].c_str(), nullptr);
    globals.dt_target = dt_target;
    globals.dt_sleep = dt_target - 0.0016;

    zmod::set_timer_resolution();
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    globals.frequency = (double)frequency.QuadPart;
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDll)))
            ;
        if (zmod::get_module_path(NULL).filename().wstring().find(L"WO4.exe") == std::wstring::npos)
        {
            return FALSE;
        }
        module_main(hinstDll);
        break;
    }
    return TRUE;
}