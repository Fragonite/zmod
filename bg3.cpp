#define NOMINMAX
#define WINMM
#include <Windows.h>
#include <map>
#include <filesystem>
#include <thread>
#include "detours.h"
#include "zmod_common.cpp"

namespace bg3
{
    void speedhack_toggle_listener(float speedhack_multiplier, int speedhack_virtual_key_code)
    {
        // Wait 5 seconds for the game to initialise.
        Sleep(5000);

        // bg3_dx11.exe+3F3FF9C - FF 15 36052D01        - call qword ptr [bg3_dx11.exe+52104D8]
        // bg3_dx11.exe+3F3FFA2 - 48 8D 8F D0000000     - lea rcx,[rdi+000000D0]
        // bg3_dx11.exe+3F3FFA9 - E8 029C1A00           - call bg3_dx11.exe+40E9BB0
        // bg3_dx11.exe+3F3FFAE - 48 8B 87 10010000     - mov rax,[rdi+00000110]
        // bg3_dx11.exe+3F3FFB5 - 48 2B 87 18010000     - sub rax,[rdi+00000118]
        // bg3_dx11.exe+3F3FFBC - 0F57 C9               - xorps xmm1,xmm1
        // bg3_dx11.exe+3F3FFBF - F2 48 0F2A C8         - cvtsi2sd xmm1,rax
        // bg3_dx11.exe+3F3FFC4 - 0F57 C0               - xorps xmm0,xmm0
        // bg3_dx11.exe+3F3FFC7 - F2 48 0F2A 87 00010000  - cvtsi2sd xmm0,[rdi+00000100]
        // bg3_dx11.exe+3F3FFD0 - F2 0F5E C8            - divsd xmm1,xmm0
        // bg3_dx11.exe+3F3FFD4 - 48 8B 05 9DDD0102     - mov rax,[bg3_dx11.exe+5F5DD78]
        // bg3_dx11.exe+3F3FFDB - 66 0F5A C9            - cvtpd2ps xmm1,xmm1
        // bg3_dx11.exe+3F3FFDF - 80 78 40 00           - cmp byte ptr [rax+40],00
        // bg3_dx11.exe+3F3FFE3 - 74 06                 - je bg3_dx11.exe+3F3FFEB
        // bg3_dx11.exe+3F3FFE5 - F3 0F5D CF            - minss xmm1,xmm7
        // bg3_dx11.exe+3F3FFE9 - EB 05                 - jmp bg3_dx11.exe+3F3FFF0
        // bg3_dx11.exe+3F3FFEB - F3 41 0F5D C8         - minss xmm1,xmm8
        // bg3_dx11.exe+3F3FFF0 - 80 BF E4010000 00     - cmp byte ptr [rdi+000001E4],00
        // bg3_dx11.exe+3F3FFF7 - 74 08                 - je bg3_dx11.exe+3F40001
        // bg3_dx11.exe+3F3FFF9 - 0F28 D6               - movaps xmm2,xmm6
        // bg3_dx11.exe+3F3FFFC - 0F28 CE               - movaps xmm1,xmm6
        // bg3_dx11.exe+3F3FFFF - EB 0F                 - jmp bg3_dx11.exe+3F40010
        // bg3_dx11.exe+3F40001 - 0F28 D1               - movaps xmm2,xmm1
        // bg3_dx11.exe+3F40004 - 48 8B 05 6DA90C02     - mov rax,[bg3_dx11.exe+600A978]
        // bg3_dx11.exe+3F4000B - F3 0F59 50 40         - mulss xmm2,[rax+40]
        auto reference = zmod::find_pattern("EB 0F 0F 28 D1 48 8B 05 ?? ?? ?? ?? F3 0F 59 50 40");
        auto mov_offset = *(int32_t *)(reference + 8);
        auto next_instruction = reference + 12;
        auto dereference = *(uint8_t **)(mov_offset + next_instruction);
        auto game_speed = (float *)(dereference + 0x40);

        *game_speed = speedhack_multiplier;
        auto is_speedhack_enabled = true;

        while (true)
        {
            if (GetAsyncKeyState(speedhack_virtual_key_code) & 0x8000)
            {
                is_speedhack_enabled = !is_speedhack_enabled;
                *game_speed = (is_speedhack_enabled) ? speedhack_multiplier : 1.0;
                do
                {
                    Sleep(10);
                } while (GetAsyncKeyState(speedhack_virtual_key_code) & 0x8000);
            }
            Sleep(10);
        }
    }

    void setup_far_reach_mod(float reach)
    {
        auto code = zmod::alloc_nearby(zmod::get_base_address(), 0x100);
        if (code == nullptr)
        {
            MessageBoxA(nullptr, "Failed to allocate memory for far reach mod.", "zmod_bg3", MB_ICONERROR);
            return;
        }

        auto reach_ptr = (float *)code;
        auto patch_1 = code + 64;
        auto patch_2 = code + 128;
        *reach_ptr = reach;

        // Set interaction radius.
        {
            auto movss = zmod::find_pattern("F3 0F 10 73 44 4C 8B E8 F3 0F 10 53 48");

            // cmp dword ptr [rbx+44],00
            // jz vanilla
            // movss xmm6,[reach_ptr]
            // ret
            // vanilla:
            // movss xmm6,[rbx+44]
            // ret
            auto patch = zmod::parse_hex("83 7B 44 00 74 09 F3 0F 10 35 .... C3 F3 0F 10 73 44 C3", zmod::rel(patch_1 + 14, reach_ptr));
            zmod::write_memory_unsafe(patch_1, patch.data(), patch.size());
            zmod::setup_call(movss, patch_1);
        }

        // Set navigation radius.
        {
            auto movss = zmod::find_pattern("F3 0F 10 49 44 F3 0F 10 51 48 0F 28 C1");

            // bg3_dx11.exe+E2FA2F - CC                    - int 3
            // bg3_dx11.exe+E2FA30 - F3 0F10 49 44         - movss xmm1,[rcx+44]
            // bg3_dx11.exe+E2FA35 - F3 0F10 51 48         - movss xmm2,[rcx+48]
            // bg3_dx11.exe+E2FA3A - 0F28 C1               - movaps xmm0,xmm1
            // bg3_dx11.exe+E2FA3D - F3 0F10 1D FBFAAA04   - movss xmm3,[bg3_dx11.exe+58DF540] { (0.01) }
            // bg3_dx11.exe+E2FA45 - F3 0F5C C2            - subss xmm0,xmm2
            // bg3_dx11.exe+E2FA49 - 0F54 05 601DAB04      - andps xmm0,[bg3_dx11.exe+58E17B0] { (2147483647) }
            // bg3_dx11.exe+E2FA50 - 0F2F D8               - comiss xmm3,xmm0
            // bg3_dx11.exe+E2FA53 - 73 19                 - jae bg3_dx11.exe+E2FA6E
            // bg3_dx11.exe+E2FA55 - 0F57 C0               - xorps xmm0,xmm0
            // bg3_dx11.exe+E2FA58 - 0F2F C8               - comiss xmm1,xmm0
            // bg3_dx11.exe+E2FA5B - 72 15                 - jb bg3_dx11.exe+E2FA72
            // bg3_dx11.exe+E2FA5D - 83 79 54 00           - cmp dword ptr [rcx+54],00 { 0 }
            // bg3_dx11.exe+E2FA61 - 75 0F                 - jne bg3_dx11.exe+E2FA72
            // bg3_dx11.exe+E2FA63 - 0F2E C8               - ucomiss xmm1,xmm0
            // bg3_dx11.exe+E2FA66 - 7A 02                 - jp bg3_dx11.exe+E2FA6A
            // bg3_dx11.exe+E2FA68 - 74 0F                 - je bg3_dx11.exe+E2FA79
            // bg3_dx11.exe+E2FA6A - F3 0F58 CB            - addss xmm1,xmm3
            // bg3_dx11.exe+E2FA6E - 0F28 C1               - movaps xmm0,xmm1
            // bg3_dx11.exe+E2FA71 - C3                    - ret
            // bg3_dx11.exe+E2FA72 - F3 0F5C D3            - subss xmm2,xmm3
            // bg3_dx11.exe+E2FA76 - 0F28 C2               - movaps xmm0,xmm2
            // bg3_dx11.exe+E2FA79 - C3                    - ret

            // movss xmm1,[reach_ptr]
            // ret
            auto patch = zmod::parse_hex("F3 0F 10 0D .... C3", zmod::rel(patch_2 + 8, reach_ptr));
            zmod::write_memory_unsafe(patch_2, patch.data(), patch.size());
            zmod::setup_call(movss, patch_2);
        }
    }

    void module_main(HINSTANCE hinstDLL)
    {
        auto ini = zmod::ini(zmod::get_module_path(hinstDLL).replace_filename(L"zmod_bg3.ini"));
        ini.set_many({
            {{L"speedhack", L"enable_speedhack_mod"}, L"true"},
            {{L"speedhack", L"speedhack_multiplier"}, L"2.0"},
            {{L"speedhack", L"speedhack_virtual_key_code"}, L"0x87"},

            {{L"far_reach", L"enable_far_reach_mod"}, L"true"},
            {{L"far_reach", L"far_reach_distance"}, L"10.0"},

            {{L"other", L"process_priority_class"}, L"0x00000080"},
        });
        if (ini.exists())
        {
            ini.load();
        }
        else
        {
            ini.save();
        }

        zmod::set_timer_resolution();

        auto process_priority_class = ini.get_uint({L"other", L"process_priority_class"}, 0);
        if (process_priority_class != 0)
        {
            if (!(SetPriorityClass(GetCurrentProcess(), process_priority_class)))
                ;
        }

        if (ini.get_bool({L"speedhack", L"enable_speedhack_mod"}) == true)
        {
            auto speedhack_multiplier = ini.get_float({L"speedhack", L"speedhack_multiplier"});
            auto speedhack_virtual_key_code = ini.get_int({L"speedhack", L"speedhack_virtual_key_code"}, 0);

            std::thread t(speedhack_toggle_listener, speedhack_multiplier, speedhack_virtual_key_code);
            t.detach();
        }

        if (ini.get_bool({L"far_reach", L"enable_far_reach_mod"}) == true)
        {
            auto reach = ini.get_float({L"far_reach", L"far_reach_distance"});
            setup_far_reach_mod(reach);
        }
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!(DisableThreadLibraryCalls(hinstDll)))
            ;
        bg3::module_main(hinstDll);
        break;
    }
    return TRUE;
}