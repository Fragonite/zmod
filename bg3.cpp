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

    void setup_far_reach_mod(float reach_bonus)
    {
        // bg3_dx11.exe+E3DBBA - 73 26                 - jae bg3_dx11.exe+E3DBE2
        // bg3_dx11.exe+E3DBBC - 41 0F2F F3            - comiss xmm6,xmm11
        // bg3_dx11.exe+E3DBC0 - 72 19                 - jb bg3_dx11.exe+E3DBDB
        // bg3_dx11.exe+E3DBC2 - 83 7B 54 00           - cmp dword ptr [rbx+54],00 { 0 }
        // bg3_dx11.exe+E3DBC6 - 75 13                 - jne bg3_dx11.exe+E3DBDB
        // bg3_dx11.exe+E3DBC8 - 41 0F2E F3            - ucomiss xmm6,xmm11
        // bg3_dx11.exe+E3DBCC - 7A 07                 - jp bg3_dx11.exe+E3DBD5
        // bg3_dx11.exe+E3DBCE - 75 05                 - jne bg3_dx11.exe+E3DBD5
        // bg3_dx11.exe+E3DBD0 - 0F57 F6               - xorps xmm6,xmm6
        // bg3_dx11.exe+E3DBD3 - EB 0D                 - jmp bg3_dx11.exe+E3DBE2
        // bg3_dx11.exe+E3DBD5 - F3 0F58 F1            - addss xmm6,xmm1
        // bg3_dx11.exe+E3DBD9 - EB 07                 - jmp bg3_dx11.exe+E3DBE2
        // bg3_dx11.exe+E3DBDB - 0F28 F2               - movaps xmm6,xmm2
        // bg3_dx11.exe+E3DBDE - F3 0F5C F1            - subss xmm6,xmm1
        // bg3_dx11.exe+E3DBE2 - F2 44 0F10 43 7C      - movsd xmm8,[rbx+7C]

        static const uint8_t code[64] = {};
        zmod::unprotect(code, sizeof(code));

        auto jp_jne = zmod::find_pattern("7A 07 75 05 0F 57 F6 EB 0D F3 0F 58 F1");

        auto patch = zmod::parse_hex("7A 06 75 04 0F 57 F6 C3 F3 0F 58 35 .... C3 00 00 00 00 00 00 00 ....", zmod::rel((uint8_t *)code + 16, (uint8_t *)code + 24), reach_bonus);
        zmod::write_memory_unsafe(code, patch.data(), patch.size());

        auto call = zmod::parse_hex("48 B8 ........ FF D0 90", (intptr_t)&code[0]);
        zmod::write_memory(jp_jne, call.data(), call.size());
    }

    void module_main(HINSTANCE hinstDLL)
    {
        auto ini = zmod::ini(zmod::get_module_path(hinstDLL).replace_filename(L"zmod_bg3.ini"));
        ini.set_many({
            {{L"zmod_bg3", L"enable_speedhack_mod"}, L"true"},
            {{L"zmod_bg3", L"speedhack_multiplier"}, L"2.0"},
            {{L"zmod_bg3", L"speedhack_virtual_key_code"}, L"0x87"},

            {{L"zmod_bg3", L"enable_far_reach_mod"}, L"true"},
            {{L"zmod_bg3", L"far_reach_bonus"}, L"1000.0"},

            {{L"zmod_bg3", L"process_priority_class"}, L"0x00000080"},
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

        auto process_priority_class = ini.get_uint({L"zmod_bg3", L"process_priority_class"}, 0);
        if (process_priority_class != 0)
        {
            if (!(SetPriorityClass(GetCurrentProcess(), process_priority_class)))
                ;
        }

        if (ini.get_bool({L"zmod_bg3", L"enable_speedhack_mod"}) == true)
        {
            auto speedhack_multiplier = ini.get_float({L"zmod_bg3", L"speedhack_multiplier"});
            auto speedhack_virtual_key_code = ini.get_int({L"zmod_bg3", L"speedhack_virtual_key_code"}, 0);

            std::thread t(speedhack_toggle_listener, speedhack_multiplier, speedhack_virtual_key_code);
            t.detach();
        }

        if (ini.get_bool({L"zmod_bg3", L"enable_far_reach_mod"}) == true)
        {
            auto reach_bonus = ini.get_float({L"zmod_bg3", L"far_reach_bonus"});
            setup_far_reach_mod(reach_bonus);
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