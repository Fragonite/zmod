// #include <windows.h>
// #include <string>
// #include <filesystem>
// #include <map>
namespace zmod
{
    /**
     * @brief Convert a wide string to lowercase.
     * @param str The wide string to convert.
     * @return The lowercase wide string.
     */
    std::wstring to_lower(const std::wstring &str)
    {
        std::wstring lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
        return lower;
    }

    class ini
    {
    public:
        using map = std::map<std::pair<std::wstring, std::wstring>, std::wstring>;

    private:
        map data;
        std::filesystem::path path;

    public:
        /**
         * @brief Construct a new ini object.
         * @param path The path to the ini file.
         */
        ini(std::filesystem::path path) : path(path)
        {
        }

        /**
         * @brief Read data from an ini file.
         * @param path The path to the ini file.
         */
        void load()
        {
            wchar_t buffer[1024];
            for (const auto &[key, value] : data)
            {
                GetPrivateProfileStringW(key.first.c_str(), key.second.c_str(), value.c_str(), buffer, 1024, path.wstring().c_str());
                data[key] = buffer;
            }
        }

        /**
         * @brief Write data to an ini file.
         * @param path The path to the ini file.
         */
        void save()
        {
            for (const auto &[key, value] : data)
            {
                if (!(WritePrivateProfileStringW(key.first.c_str(), key.second.c_str(), value.c_str(), path.wstring().c_str())))
                    ;
            }
        }

        /**
         * @brief Check if a file exists.
         * @param path The path to the file.
         * @return True if the file exists, false otherwise.
         */
        bool exists()
        {
            return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
        }

        /**
         * @brief Set many values at once. Useful for setting defaults.
         * @param data The data to set.
         */
        void set_many(const map &data)
        {
            for (const auto &[key, value] : data)
            {
                this->data[key] = value;
            }
        }

        /**
         * @brief Get a wide string value.
         * @param key The key to get.
         * @return The value.
         */
        std::wstring get_wstring(const std::pair<std::wstring, std::wstring> &key)
        {
            return data[key];
        }

        /**
         * @brief Get a double value.
         * @param key The key to get.
         * @return The value.
         */
        double get_double(const std::pair<std::wstring, std::wstring> &key)
        {
            return std::wcstod(data[key].c_str(), nullptr);
        }

        /**
         * @brief Get a float value.
         * @param key The key to get.
         * @return The value.
         */
        float get_float(const std::pair<std::wstring, std::wstring> &key)
        {
            return std::wcstof(data[key].c_str(), nullptr);
        }

        /**
         * @brief Get an int (long) value.
         * @param key The key to get.
         * @return The value.
         */
        int get_int(const std::pair<std::wstring, std::wstring> &key, int base = 0)
        {
            return std::wcstol(data[key].c_str(), nullptr, base);
        }

        /**
         * @brief Get an unsigned int (long) value.
         * @param key The key to get.
         * @return The value.
         */
        unsigned int get_uint(const std::pair<std::wstring, std::wstring> &key, int base = 0)
        {
            return std::wcstoul(data[key].c_str(), nullptr, base);
        }

        /**
         * @brief Get a boolean value.
         * @param key The key to get.
         * @return The value.
         */
        bool get_bool(const std::pair<std::wstring, std::wstring> &key)
        {
            auto value = to_lower(data[key]);
            return value != L"0" && value != L"false";
        }
    };
    using ini_map = std::map<std::pair<std::wstring, std::wstring>, std::wstring>;

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

    /**
     * @brief Unprotect memory for reading, writing, and executing.
     * @param addr The address to unprotect.
     * @param size The size to unprotect.
     */
    template <typename T>
    bool unprotect(T *addr, size_t size)
    {
        DWORD old_protect;
        return VirtualProtect((void *)addr, size, PAGE_EXECUTE_READWRITE, &old_protect);
    }

    /**
     * @brief Write memory unsafely.
     * @param addr The address to write to.
     * @param data The data to write.
     * @param size The size to write.
     */
    template <typename T, typename U>
    void write_memory_unsafe(T *addr, const U *data, size_t size)
    {
        std::memcpy((void *)addr, (void *)data, size);
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

    template <typename T, typename U>
    void write_memory(T *address, const U *data, size_t size)
    {
        DWORD old_protect;
        if (!(VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &old_protect)))
            ;
        std::memcpy((void *)address, (void *)data, size);
        if (!(VirtualProtect((void *)address, size, old_protect, &old_protect)))
            ;
    }

    template <typename T>
    void add_replacements_helper(std::vector<uint8_t> &replacements, std::vector<std::pair<size_t, size_t>> &offsets, const T &arg)
    {
        // This will be the new offset for the latest argument in the replacements vector.
        auto offset = replacements.size();

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

        auto size = replacements.size() - offset;
        offsets.push_back({offset, size});
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
        std::vector<std::pair<size_t, size_t>> offsets;

        if (hex.find('.') != std::string::npos && hex.find('?') != std::string::npos)
        {
            return bytes;
        }

        (add_replacements_helper(replacements, offsets, args), ...);
        auto arg_index = 0;
        auto index = 0;

        while (iss >> s)
        {
            if (auto dot_count = std::count(s.begin(), s.end(), '.'); dot_count > 0)
            {
                for (auto i = 0; i < dot_count; ++i)
                {
                    auto offset = offsets[arg_index].first;
                    auto length = offsets[arg_index].second;

                    if (i < length)
                    {
                        bytes.push_back(replacements[offset + i]);
                    }
                    else
                    {
                        bytes.push_back(0);
                    }
                }
                ++arg_index;
            }
            else if (s == "??")
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

    /**
     * @brief Return the relative offset between two addresses.
     * @param next_instruction The address of the next instruction.
     * @param absolute The address to jump to.
     */
    template <typename T, typename U>
    int32_t rel(T *next_instruction, U *absolute)
    {
        return (int32_t)((intptr_t)absolute - (intptr_t)next_instruction);
    }

    /**
     * @brief Setup a call instruction.
     * @param pounce The address to call from.
     * @param splat The address to call.
     */
    template <typename T, typename U>
    void setup_call(T *pounce, U *splat)
    {
        auto jmp = zmod::parse_hex("E8 ?? ?? ?? ??", rel(pounce + 5, splat));
        zmod::write_memory(pounce, jmp.data(), jmp.size());
    }

    /**
     * @brief Setup a jump instruction.
     * @param pounce The address to jump from.
     * @param splat The address to jump to.
     */
    template <typename T, typename U>
    void setup_jmp(T *pounce, U *splat)
    {
        auto jmp = zmod::parse_hex("E9 ?? ?? ?? ??", rel(pounce + 5, splat));
        zmod::write_memory(pounce, jmp.data(), jmp.size());
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