namespace zmod
{
    using ini_map = std::map<std::pair<std::wstring, std::wstring>, std::wstring>;
    using ini = ini_map;

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

    bool file_exists(const std::filesystem::path &path)
    {
        return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
    }

    void read_ini_file(const std::filesystem::path &path, ini_map &ini)
    {
        wchar_t buffer[1024];
        for (const auto &[key, value] : ini)
        {
            GetPrivateProfileStringW(key.first.c_str(), key.second.c_str(), value.c_str(), buffer, 1024, path.wstring().c_str());
            ini[{key.first, key.second}] = buffer;
        }
    }

    void write_ini_file(const std::filesystem::path &path, const ini_map &ini)
    {
        for (const auto &[key, value] : ini)
        {
            if (!(WritePrivateProfileStringW(key.first.c_str(), key.second.c_str(), value.c_str(), path.wstring().c_str())))
                ;
        }
    }

    std::filesystem::path get_module_path(HMODULE module)
    {
        wchar_t module_path[MAX_PATH];
        if (!(GetModuleFileNameW(module, module_path, MAX_PATH)))
            ;
        return std::filesystem::path(module_path);
    }

    std::filesystem::path get_system_path()
    {
        wchar_t system_path[MAX_PATH];
        if (!(GetSystemDirectoryW(system_path, MAX_PATH)))
            ;
        return std::filesystem::path(system_path);
    }

    const uint8_t *find_pattern_in_memory(const uint8_t *start, const uint8_t *end, const std::vector<uint8_t> &pattern, const std::string &mask)
    {
        if (pattern.size() != mask.size())
        {
            return nullptr;
        }

        for (const uint8_t *current = start; current <= end - pattern.size(); ++current)
        {
            auto found = true;
            for (auto i = 0; i < pattern.size(); ++i)
            {
                if (mask[i] != '?' && pattern[i] != current[i])
                {
                    found = false;
                    break;
                }
            }
            if (found)
            {
                return current;
            }
        }
        return nullptr;
    }

    const uint8_t *find_pattern(const uint8_t *start, size_t size, const std::vector<uint8_t> &pattern, const std::string &mask)
    {
        if (pattern.size() != mask.size())
        {
            return nullptr;
        }

        auto current = start;
        MEMORY_BASIC_INFORMATION mbi;
        while (VirtualQuery(current, &mbi, sizeof(mbi)) && current < (start + size))
        {
            switch (mbi.Protect)
            {
            case PAGE_EXECUTE_READ:
            case PAGE_EXECUTE_READWRITE:
            case PAGE_EXECUTE_WRITECOPY:
            case PAGE_READONLY:
            case PAGE_READWRITE:
            case PAGE_WRITECOPY:
            {
                auto end = (const uint8_t *)mbi.BaseAddress + mbi.RegionSize;
                if (end > start + size)
                {
                    end = start + size;
                }
                auto address = find_pattern_in_memory(current, end, pattern, mask);
                if (address)
                {
                    return address;
                }
                break;
            }
            }
            current = (uint8_t *)mbi.BaseAddress + mbi.RegionSize;
        }
        return nullptr;
    }

    void write_memory(void *address, const void *data, size_t size)
    {
        DWORD old_protect;
        if (!(VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &old_protect)))
            ;
        std::memcpy(address, data, size);
        if (!(VirtualProtect(address, size, old_protect, &old_protect)))
            ;
    }

    template <typename T>
    void add_replacements_helper(std::vector<uint8_t> &replacements, const T &arg)
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            for (auto c : arg)
            {
                replacements.push_back(c);
            }
        }
        else if constexpr (std::is_same_v<T, std::vector<uint8_t>>)
        {
            for (auto c : arg)
            {
                replacements.push_back(c);
            }
        }
        else if constexpr (std::is_integral_v<T> || std::is_same_v<T, float> || std::is_same_v<T, double>)
        {
            const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&arg);
            for (int i = 0; i < sizeof(T); ++i)
            {
                replacements.push_back(bytes[i]);
            }
        }
    }

    std::vector<uint8_t> parse_hex(const std::string &hex)
    {
        std::istringstream iss(hex);
        std::string s;
        std::vector<uint8_t> bytes;
        while (iss >> s)
        {
            if (s == "??")
            {
                bytes.push_back(0);
            }
            else
            {
                bytes.push_back((uint8_t)std::stoul(s, nullptr, 16));
            }
        }
        return bytes;
    }

    template <typename... Args>
    std::vector<uint8_t> parse_hex(const std::string &hex, Args... args)
    {
        std::istringstream iss(hex);
        std::string s;
        std::vector<uint8_t> bytes;
        std::vector<uint8_t> replacements;
        (add_replacements_helper(replacements, args), ...);
        auto index = 0;
        while (iss >> s)
        {
            if (s == "??")
            {
                if (index < replacements.size())
                {
                    bytes.push_back(replacements[index]);
                    ++index;
                }
                else
                {
                    bytes.push_back(0);
                }
            }
            else
            {
                bytes.push_back((uint8_t)std::stoul(s, nullptr, 16));
            }
        }
        return bytes;
    }

    std::string parse_bytes(const uint8_t *bytes, size_t size)
    {
        std::ostringstream oss;
        for (auto i = 0; i < size; ++i)
        {
            oss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)bytes[i];
            if (i != size - 1)
            {
                oss << " ";
            }
        }
        return oss.str();
    }

    std::pair<std::vector<uint8_t>, std::string> parse_hex_mask(const std::string &hex)
    {
        std::istringstream iss(hex);
        std::string s;
        std::vector<uint8_t> bytes;
        std::string mask;
        while (iss >> s)
        {
            if (s == "??")
            {
                bytes.push_back(0);
                mask.push_back('?');
            }
            else
            {
                bytes.push_back((uint8_t)std::stoul(s, nullptr, 16));
                mask.push_back('x');
            }
        }
        return {bytes, mask};
    }

    const uint8_t *find_pattern(const uint8_t *start, size_t size, const std::string &pattern)
    {
        auto [bytes, mask] = parse_hex_mask(pattern);
        return find_pattern(start, size, bytes, mask);
    }

    uint8_t *get_base_address(const wchar_t *module_name)
    {
        return (uint8_t *)GetModuleHandleW(module_name);
    }

    const uint8_t *find_pattern(const std::string &pattern)
    {
        auto base = zmod::get_base_address(nullptr);
        auto [bytes, mask] = parse_hex_mask(pattern);
        return find_pattern(base, 0x1000000, bytes, mask);
    }

    const uint8_t *find_wstring(const std::wstring &str)
    {
        auto base = zmod::get_base_address(nullptr);
        auto bytes = std::vector<uint8_t>(str.length() * sizeof(wchar_t));
        std::memcpy(bytes.data(), str.data(), bytes.size());
        auto mask = std::string(bytes.size(), 'x');
        return find_pattern(base, 0x1000000, bytes, mask);
    }

#ifdef WINMM

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

#endif
}