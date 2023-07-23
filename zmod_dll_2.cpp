#include <Windows.h>
#include <map>
#include <filesystem>
#include "detours.h"
#include "zmod_common.cpp"

typedef HWND(WINAPI CreateWindowExA_t)(
    DWORD dwExStyle,
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam);

typedef HWND(WINAPI CreateWindowExW_t)(
    DWORD dwExStyle,
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam);

CreateWindowExA_t CreateWindowExA_hook;
CreateWindowExW_t CreateWindowExW_hook;

CreateWindowExA_t *CreateWindowExA_orig = CreateWindowExA;
CreateWindowExW_t *CreateWindowExW_orig = CreateWindowExW;
std::filesystem::path module_path;

HWND WINAPI CreateWindowExA_hook(
    DWORD dwExStyle,
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam)
{
    auto ret = CreateWindowExA_orig(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID &)CreateWindowExA_orig, CreateWindowExA_hook);
    DetourDetach(&(PVOID &)CreateWindowExW_orig, CreateWindowExW_hook);
    DetourTransactionCommit();
    load_dlls();
    return ret;
}

HWND WINAPI CreateWindowExW_hook(
    DWORD dwExStyle,
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam)
{
    auto ret = CreateWindowExW_orig(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID &)CreateWindowExA_orig, CreateWindowExA_hook);
    DetourDetach(&(PVOID &)CreateWindowExW_orig, CreateWindowExW_hook);
    DetourTransactionCommit();
    load_dlls();
    return ret;
}

std::wstring to_lower(std::wstring str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

void load_dlls()
{
    std::vector<std::filesystem::path> matching_dlls;
    auto parent_path = module_path.parent_path();
    for (const auto &entry : std::filesystem::directory_iterator(parent_path))
    {
        if (!(entry.is_regular_file()))
        {
            continue;
        }
        if (to_lower(entry.path().extension().wstring()) != L".dll")
        {
            continue;
        }
        auto filename = to_lower(entry.path().filename().wstring());
        if (filename == L"zmod_32.dll")
        {
            continue;
        }
        if (filename == L"zmod_64.dll")
        {
            continue;
        }
        if (filename.find(L"zmod") != 0)
        {
            continue;
        }
        matching_dlls.push_back(entry.path());
    }
    std::sort(matching_dlls.begin(), matching_dlls.end());
    for (const auto &dll : matching_dlls)
    {
        LoadLibraryW(dll.wstring().c_str());
    }
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
        module_path = zmod::get_module_path(hinstDLL);
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID &)CreateWindowExA_orig, CreateWindowExA_hook);
        DetourAttach(&(PVOID &)CreateWindowExW_orig, CreateWindowExW_hook);
        DetourTransactionCommit();
        break;
    }
    return TRUE;
}