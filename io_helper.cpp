using ini_map = std::map<std::pair<std::wstring, std::wstring>, std::wstring>;

void read_ini_file(std::filesystem::path path, ini_map &map)
{
    wchar_t buffer[1024];
    for (const auto &[key, value] : map)
    {
        GetPrivateProfileStringW(key.first.c_str(), key.second.c_str(), value.c_str(), buffer, 1024, path.wstring().c_str());
        map[{key.first, key.second}] = buffer;
    }
}

void create_ini_file(std::filesystem::path path, const ini_map &map)
{
    for (const auto &[key, value] : map)
    {
        WritePrivateProfileStringW(key.first.c_str(), key.second.c_str(), value.c_str(), path.wstring().c_str());
    }
}

bool file_exists(std::filesystem::path path)
{
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}