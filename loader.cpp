#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <winternl.h>
#include <map>
#include <filesystem>
#include <stack>
#include <ostream>
#include "detours.h"
#include "zmod_common.cpp"

// 1    0 000010F0 GetFileVersionInfoA
//           2    1 00002370 GetFileVersionInfoByHandle
//           3    2 00001E90 GetFileVersionInfoExA
//           4    3 00001070 GetFileVersionInfoExW
//           5    4 00001010 GetFileVersionInfoSizeA
//           6    5 00001EB0 GetFileVersionInfoSizeExA
//           7    6 00001090 GetFileVersionInfoSizeExW
//           8    7 000010B0 GetFileVersionInfoSizeW
//           9    8 000010D0 GetFileVersionInfoW
//          10    9 00001ED0 VerFindFileA
//          11    A 00002560 VerFindFileW
//          12    B 00001EF0 VerInstallFileA
//          13    C 00003320 VerInstallFileW
//          14    D          VerLanguageNameA (forwarded to KERNEL32.VerLanguageNameA)
//          15    E          VerLanguageNameW (forwarded to KERNEL32.VerLanguageNameW)
//          16    F 00001030 VerQueryValueA
//          17   10 00001050 VerQueryValueW

#pragma comment(linker, "/export:GetFileVersionInfoA=dummy_export,@1")
#pragma comment(linker, "/export:GetFileVersionInfoByHandle=dummy_export,@2")
#pragma comment(linker, "/export:GetFileVersionInfoExA=dummy_export,@3")
#pragma comment(linker, "/export:GetFileVersionInfoExW=dummy_export,@4")
#pragma comment(linker, "/export:GetFileVersionInfoSizeA=dummy_export,@5")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExA=dummy_export,@6")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExW=dummy_export,@7")
#pragma comment(linker, "/export:GetFileVersionInfoSizeW=dummy_export,@8")
#pragma comment(linker, "/export:GetFileVersionInfoW=dummy_export,@9")
#pragma comment(linker, "/export:VerFindFileA=dummy_export,@10")
#pragma comment(linker, "/export:VerFindFileW=dummy_export,@11")
#pragma comment(linker, "/export:VerInstallFileA=dummy_export,@12")
#pragma comment(linker, "/export:VerInstallFileW=dummy_export,@13")
#pragma comment(linker, "/export:VerLanguageNameA=dummy_export,@14")
#pragma comment(linker, "/export:VerLanguageNameW=dummy_export,@15")
#pragma comment(linker, "/export:VerQueryValueA=dummy_export,@16")
#pragma comment(linker, "/export:VerQueryValueW=dummy_export,@17")

#define OK 0

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

CreateWindowExW_t CreateWindowExW_hook;
CreateWindowExA_t CreateWindowExA_hook;
void load_dlls();

CreateWindowExW_t *CreateWindowExW_orig = CreateWindowExW;
CreateWindowExA_t *CreateWindowExA_orig = CreateWindowExA;
HINSTANCE instance;

int module_main(HINSTANCE hinstDLL);

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
    DetourDetach(&(PVOID &)CreateWindowExW_orig, CreateWindowExW_hook);
    DetourDetach(&(PVOID &)CreateWindowExA_orig, CreateWindowExA_hook);
    DetourTransactionCommit();
    // load_dlls();
    // module_main(instance);
    // MessageBoxA(NULL, "Hello from loader!", "CreateWindowExW", MB_OK);
    return ret;
}

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
    DetourDetach(&(PVOID &)CreateWindowExW_orig, CreateWindowExW_hook);
    DetourDetach(&(PVOID &)CreateWindowExA_orig, CreateWindowExA_hook);
    DetourTransactionCommit();
    // load_dlls();
    // module_main(instance);
    return ret;
}

extern "C"
{
    void dummy_export()
    {
    }
}

class memory
{
    struct protection_info
    {
        void *addr;
        size_t size;
        DWORD old_protect;
    };
    std::stack<protection_info> protections;

public:
    bool unprotect(void *addr, size_t size)
    {
        DWORD old_protect;
        if (VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &old_protect))
        {
            protections.push({addr, size, old_protect});
            return true;
        }
        return false;
    }
    bool write(void *dst, void *src, size_t size)
    {
        if (unprotect(dst, size))
        {
            memcpy(dst, src, size);
            return true;
        }
        return false;
    }
    void write_unsafe(void *dst, void *src, size_t size)
    {
        memcpy(dst, src, size);
    }
    ~memory()
    {
        while (!protections.empty())
        {
            auto &info = protections.top();
            VirtualProtect(info.addr, info.size, info.old_protect, &info.old_protect);
            protections.pop();
        }
    }
};

IMAGE_EXPORT_DIRECTORY *get_export_directory(HMODULE module)
{
    auto base_addr = (uint8_t *)module;

    auto dos_header = (IMAGE_DOS_HEADER *)base_addr;
    auto nt_headers = (IMAGE_NT_HEADERS *)(base_addr + dos_header->e_lfanew);
    auto export_entry = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    auto export_directory = (IMAGE_EXPORT_DIRECTORY *)(base_addr + export_entry.VirtualAddress);
    return export_directory;
}

IMAGE_IMPORT_DESCRIPTOR *get_import_descriptor(HMODULE module)
{
    auto base_addr = (uint8_t *)module;
    auto dos_header = (IMAGE_DOS_HEADER *)base_addr;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
    {
        // Handle error: Invalid DOS Signature
        return nullptr;
    }
    auto nt_headers = (IMAGE_NT_HEADERS *)(base_addr + dos_header->e_lfanew);
    auto import_entry = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto import_descriptor = (IMAGE_IMPORT_DESCRIPTOR *)(base_addr + import_entry.VirtualAddress);
    return import_descriptor;
}

std::string to_lower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

void clone_eat(HMODULE proxy, HMODULE original)
{
    auto prox_export = get_export_directory(proxy);
    auto orig_export = get_export_directory(original);

    auto prox_base = (uint8_t *)proxy;
    auto orig_base = (uint8_t *)original;

    memory mem;

    // Save RVAs
    auto names = prox_export->AddressOfNames;
    auto ordinals = prox_export->AddressOfNameOrdinals;
    auto functions = prox_export->AddressOfFunctions;

    // Clone IMAGE_EXPORT_DIRECTORY
    mem.write(prox_export, orig_export, sizeof(IMAGE_EXPORT_DIRECTORY));

    // Clone name, ordinal, and function arrays
    mem.write((prox_base + names), (orig_base + orig_export->AddressOfNames), orig_export->NumberOfNames * sizeof(uint32_t));
    mem.write((prox_base + ordinals), (orig_base + orig_export->AddressOfNameOrdinals), orig_export->NumberOfFunctions * sizeof(uint16_t));
    mem.write((prox_base + functions), (orig_base + orig_export->AddressOfFunctions), orig_export->NumberOfFunctions * sizeof(uint32_t));

    // Restore RVAs
    prox_export->AddressOfNames = names;
    prox_export->AddressOfNameOrdinals = ordinals;
    prox_export->AddressOfFunctions = functions;

    // Correct RVAs
    intptr_t offset = orig_base - prox_base;
    for (auto i = 0; i < orig_export->NumberOfFunctions; ++i)
    {
        auto pnames = (uint32_t *)(prox_base + names);
        auto pordinals = (uint16_t *)(prox_base + ordinals);
        auto pfunctions = (uint32_t *)(prox_base + functions);

        auto onames = (uint32_t *)(orig_base + orig_export->AddressOfNames);
        auto oordinals = (uint16_t *)(orig_base + orig_export->AddressOfNameOrdinals);
        auto ofunctions = (uint32_t *)(orig_base + orig_export->AddressOfFunctions);

        if (i < orig_export->NumberOfNames)
        {
            pnames[i] = onames[i] + offset;
        }
        pordinals[i] = oordinals[i];
        pfunctions[i] = ofunctions[i] + offset;
    }
}

bool is_current_process_64bit()
{
    BOOL wow64;
    IsWow64Process(GetCurrentProcess(), &wow64);
    return !wow64;
}

std::vector<HMODULE> get_process_modules()
{
    std::vector<HMODULE> modules;
    modules.resize(1024);
    DWORD needed;
    if (EnumProcessModules(GetCurrentProcess(), modules.data(), sizeof(HMODULE) * modules.capacity(), &needed))
    {
        modules.resize(std::min(int(needed / sizeof(HMODULE)), 1024));
    }

    return modules;
}

std::vector<HMODULE> get_loaded_modules()
{
    std::vector<HMODULE> modules;

    PPEB peb = NtCurrentTeb()->ProcessEnvironmentBlock;
    PPEB_LDR_DATA ldr = peb->Ldr;

    PLIST_ENTRY first = ldr->InMemoryOrderModuleList.Flink;
    PLIST_ENTRY current = first;
    do
    {
        PLDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        modules.push_back((HMODULE)entry->DllBase);

        MessageBoxW(NULL, L"FullDllName", entry->FullDllName.Buffer, MB_OK);

        if (!(entry->DllBase))
        {
            MessageBoxW(NULL, L"DllBase is null", &entry->FullDllName.Buffer[0], MB_OK);
        }

        current = current->Flink;
    } while (current != first);

    return modules;
}

int patch_iat(HMODULE module, IMAGE_IMPORT_DESCRIPTOR *import, HMODULE proxy)
{
    auto base_addr = (uint8_t *)module;
    auto original_first_thunk = (IMAGE_THUNK_DATA *)(base_addr + import->OriginalFirstThunk);
    auto first_thunk = (IMAGE_THUNK_DATA *)(base_addr + import->FirstThunk);

    memory mem;

    MessageBoxA(NULL, "Hello from loader!", "patch_iat", MB_OK);

    for (; original_first_thunk->u1.AddressOfData; ++original_first_thunk, ++first_thunk)
    {
        FARPROC function;

        if (original_first_thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
        {
            // Imported by ordinal
            auto ordinal = IMAGE_ORDINAL(original_first_thunk->u1.Ordinal);
            function = GetProcAddress(proxy, MAKEINTRESOURCEA(ordinal));
            MessageBoxA(NULL, "1!", "patch_iat", MB_OK);
            if (!(function))
            {
                return 1;
            }
        }
        else
        {
            // Imported by name
            auto import_by_name = (IMAGE_IMPORT_BY_NAME *)(base_addr + original_first_thunk->u1.AddressOfData);
            if (!(import_by_name))
            {
                return 2;
            }

            auto function_name = (char *)import_by_name->Name;
            function = GetProcAddress(proxy, function_name);
            MessageBoxA(NULL, function_name, "patch_iat", MB_OK);
            if (!(function))
            {
                MessageBoxA(NULL, "3!", "patch_iat", MB_OK);
                return 3;
            }
        }

        mem.unprotect(&(first_thunk->u1.Function), sizeof(FARPROC));
        first_thunk->u1.Function = (intptr_t)function;
        // mem.write((void *)&(first_thunk->u1.Function), &function, sizeof(FARPROC));
    }

    return OK;
}

IMAGE_IMPORT_DESCRIPTOR *find_import(HMODULE module, const std::string &dll)
{
    if (!(module))
    {
        MessageBoxA(NULL, "Module is null", "um", MB_OK);
        return nullptr;
    }
    auto base_addr = (uint8_t *)module;
    auto import_descriptor = get_import_descriptor(module);
    // MessageBoxA(NULL, zmod::get_module_path(module).filename().string().c_str(), "um", MB_OK);

    for (; import_descriptor->Name; ++import_descriptor)
    {
        auto name = to_lower(std::string((char *)(base_addr + import_descriptor->Name)));
        if (name == dll)
        {
            MessageBoxA(NULL, name.c_str(), ("ermagerdfind_import: " + zmod::get_module_path(module).string()).c_str(), MB_OK);
            return import_descriptor;
        }
    }
    return nullptr;
}

int module_main(HINSTANCE proxy)
{
    auto proxy_path = zmod::get_module_path(proxy);
    auto original = LoadLibraryW((zmod::get_system_path() / proxy_path.filename()).wstring().c_str());
    if (!original)
    {
        return 1;
    }

    clone_eat(proxy, original);

    auto modules = get_loaded_modules();
    auto proxy_name = to_lower(proxy_path.filename().string());
    // auto module_path = zmod::get_module_path(modules[0]);
    // MessageBoxA(NULL, module_path.filename().string().c_str(), "Module", MB_OK);
    // module_path = zmod::get_module_path(modules[1]);
    // MessageBoxA(NULL, module_path.filename().string().c_str(), "Module", MB_OK);
    for (auto module : modules)
    {
        // MessageBoxA(NULL, zmod::get_module_path(module).string().c_str(), "Checkies!", MB_OK);
        auto import = find_import(module, proxy_name);
        if (!(import))
        {
            // MessageBoxA(NULL, zmod::get_module_path(module).string().c_str(), "Nopies!", MB_OK);
            continue;
        }
        MessageBoxA(NULL, zmod::get_module_path(module).string().c_str(), "This module imports the proxy", MB_OK);

        // MessageBoxA(NULL, zmod::get_module_path(module).string().c_str(), "Other", MB_OK);

        if (auto result = patch_iat(module, import, original); result != OK)
        {
            std::stringstream ss;
            ss << std::hex << (result);
            MessageBoxA(NULL, ss.str().c_str(), zmod::get_module_path(module).string().c_str(), MB_OK);
            return 2;
        }

        std::stringstream ss;
        ss << std::hex << reinterpret_cast<uintptr_t>(module);
        MessageBoxA(NULL, ss.str().c_str(), (std::string("should be good") + zmod::get_module_path(module).string()).c_str(), MB_OK);
        // break;
    }

    // instance = proxy;

    // DetourTransactionBegin();
    // DetourUpdateThread(GetCurrentThread());
    // DetourAttach(&(PVOID &)CreateWindowExW_orig, CreateWindowExW_hook);
    // DetourAttach(&(PVOID &)CreateWindowExA_orig, CreateWindowExA_hook);
    // DetourTransactionCommit();

    MessageBoxA(NULL, "Hello from loader!", "Loader", MB_OK);
    return OK;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!DisableThreadLibraryCalls(hinstDLL))
            ;
        module_main(hinstDLL);
        break;
    }
    return TRUE;
}