#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <filesystem>
#include <map>
#include "zmod_common.cpp"

typedef uint32_t(__stdcall fn_48F700_t)(uint32_t);
fn_48F700_t flinch_check_hook;
fn_48F700_t *flinch_check_orig = nullptr;

uint32_t __stdcall flinch_check_hook(uint32_t a1)
{
    __asm {
        push eax;
        mov eax, flinch_check_orig;
        call eax;
        test eax, eax;
        jnz skip_eax_write;
        mov eax, 1;
    skip_eax_write:
    }
}

void insert_relative_address(void *dst, void *next_instruction, void *target)
{
    auto relative_address = (intptr_t)target - (intptr_t)next_instruction;
    memcpy(dst, &relative_address, sizeof(relative_address));
}

void insert_absolute_address(void *dst, void *next_instruction, void *relative)
{
    auto absolute_address = (intptr_t)next_instruction + (intptr_t)relative;
    memcpy(dst, &absolute_address, sizeof(absolute_address));
}

void module_main(HINSTANCE instance)
{
    auto module_path = zmod::get_module_path(instance);
    auto ini_path = module_path.replace_filename(L"zmod_dw8xlce_affinity.ini");

    zmod::ini ini = {
        {{L"tetsuo9999", L"force_neutral_affinity"}, L"0"},
        {{L"tetsuo9999", L"officers_with_affinity_advantage_can_flinch"}, L"0"},
        {{L"fumasparku", L"player_cannot_flinch"}, L"0"},
    };

    if (zmod::file_exists(ini_path))
    {
        zmod::read_ini_file(ini_path, ini);
    }
    else
    {
        zmod::write_ini_file(ini_path, ini);
    }

    if (ini[{L"tetsuo9999", L"force_neutral_affinity"}] != L"0")
    {
        auto addr = zmod::find_pattern("8B 44 94 04 33 CC");
        auto patch = zmod::parse_hex("31 C0 40 90");
        zmod::write_memory((void *)addr, patch.data(), patch.size());
    }

    if (ini[{L"tetsuo9999", L"officers_with_affinity_advantage_can_flinch"}] != L"0")
    {
        auto addr = zmod::find_pattern("0F 84 B2 00 00 00 80 BE E5 02 00 00 00");
        auto patch = zmod::parse_hex("0F 84 61 FF FF FF");
        zmod::write_memory((void *)addr, patch.data(), patch.size());
    }

    if (ini[{L"fumasparku", L"player_cannot_flinch"}] != L"0")
    {
        auto addr = zmod::find_pattern("8B 76 58 83 C8 FF 50") + 8;
        insert_absolute_address(&flinch_check_orig, (void *)(addr + 4), *(void **)addr);

        intptr_t relative;
        insert_relative_address(&relative, (void *)(addr + 4), flinch_check_hook);
        zmod::write_memory((void *)addr, &relative, sizeof(relative));
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDLL)))
            ;
        module_main(hinstDLL);
        break;
    }
    return TRUE;
}