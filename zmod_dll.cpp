#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <filesystem>
#include <string.h>
#include "detours.h"

HWND(WINAPI *_CreateWindowExA)
(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) = CreateWindowExA;

extern "C"
{
    void DummyExport()
    {
    }
}

// wstring to lower function
std::wstring to_lower_w(std::wstring str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::towlower);
    return str;
}

void load_dlls()
{
    // Load all dlls in the current directory in alphabetical order, excluding any dlls with the same name as the current module
    auto cwd = std::filesystem::current_path();
    std::vector<std::filesystem::path> paths;
    // Add all dlls in the current directory to the path vector
    for (auto &p : std::filesystem::directory_iterator(cwd))
    {
        if (to_lower_w(p.path().extension()) == L".dll")
        {
            paths.push_back(p.path());
        }
    }
    // Order path vector alphabetically
    std::sort(paths.begin(), paths.end());

    std::wstring moduleName;
    moduleName.reserve(MAX_PATH);
    if (!(GetModuleFileNameW(nullptr, moduleName.data(), moduleName.capacity())))
        ;
    auto module = std::filesystem::path(moduleName.c_str()).filename();

    for (auto &p : paths)
    {
        if (p.filename() == module)
        {
            continue;
        }
        LoadLibraryW(p.wstring().c_str());
    }
}

HWND WINAPI HookedCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto ret = _CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID &)_CreateWindowExA, HookedCreateWindowExA);
    DetourTransactionCommit();
    load_dlls();
    return ret;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (DetourIsHelperProcess())
    {
        return TRUE;
    }

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDLL)))
            ;

        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID &)_CreateWindowExA, HookedCreateWindowExA);
        DetourTransactionCommit();
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}