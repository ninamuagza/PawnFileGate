namespace FileUtils {
    inline bool FileExists(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        return f.good();
    }

    inline size_t FileSize(const std::string& path) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) return 0;
        return static_cast<size_t>(f.tellg());
    }

    inline bool CreateDirectory(const std::string& path) {
        #ifdef _WIN32
            return _mkdir(path.c_str()) == 0 || errno == EEXIST;
        #else
            return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
        #endif
    }

    inline bool RemoveFile(const std::string& path) {
        return std::remove(path.c_str()) == 0;
    }

    inline bool RenameFile(const std::string& oldPath, const std::string& newPath) {
        return std::rename(oldPath.c_str(), newPath.c_str()) == 0;
    }

    inline bool CopyFile(const std::string& src, const std::string& dst) {
        std::ifstream srcFile(src, std::ios::binary);
        if (!srcFile) return false;

        std::ofstream dstFile(dst, std::ios::binary);
        if (!dstFile) return false;

        dstFile << srcFile.rdbuf();
        return srcFile.good() && dstFile.good();
    }

    inline std::string GetCurrentWorkingDirectory() {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            std::string result(cwd);
            if (!result.empty() && result.back() != '/')
                result += '/';
            return result;
        }
        return "./";
    }

    inline std::int64_t GetFileModificationTime(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            return static_cast<std::int64_t>(st.st_mtime);
        }
        return 0;
    }

    inline void CleanupTempFiles(const std::string& tempDir, int maxAgeSeconds) {
        #ifdef _WIN32
            std::string searchPath = tempDir + "*.tmp";
            WIN32_FIND_DATAA findData;
            HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        std::string fileName = findData.cFileName;
                        if (fileName.size() > 4 && fileName.substr(fileName.size() - 4) == ".tmp") {
                            std::string fullPath = tempDir + fileName;

                            FILETIME ft = findData.ftLastWriteTime;
                            ULARGE_INTEGER ull;
                            ull.LowPart = ft.dwLowDateTime;
                            ull.HighPart = ft.dwHighDateTime;

                            __int64 fileTime = ull.QuadPart / 10000000 - 11644473600LL;
                            __int64 now = std::time(nullptr);

                            if (now - fileTime > maxAgeSeconds) {
                                DeleteFileA(fullPath.c_str());
                            }
                        }
                    }
                } while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
        #else
            DIR* dir = opendir(tempDir.c_str());
            if (dir) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    std::string name = entry->d_name;
                    if (name.size() > 4 && name.substr(name.size() - 4) == ".tmp") {
                        std::string fullPath = tempDir + name;

                        struct stat st;
                        if (stat(fullPath.c_str(), &st) == 0) {
                            time_t now = time(nullptr);
                            if (now - st.st_mtime > maxAgeSeconds) {
                                std::remove(fullPath.c_str());
                            }
                        }
                    }
                }
                closedir(dir);
            }
        #endif
    }
}

static std::string NormalizeSlashes(std::string s)
{
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}

static std::string Trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string SanitizeFilename(const std::string& input)
{
    std::string filename = NormalizeSlashes(input);

    size_t lastSlash = filename.find_last_of('/');
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    if (filename.empty() || filename == "." || filename == "..")
        return "";

    std::string out;
    out.reserve(filename.size());

    for (unsigned char c : filename) {
        if (c < 32) continue;
        switch (c) {
            case '<': case '>': case ':': case '"':
            case '/': case '\\': case '|': case '?': case '*':
                out += '_';
                break;
            default:
                out += static_cast<char>(c);
                break;
        }
    }

    while (!out.empty() && (out.back() == ' ' || out.back() == '.'))
        out.pop_back();

    if (out.empty() || out == "." || out == "..")
        return "";

    return out;
}

static std::string SanitizeRelativeDir(const std::string& input)
{
    std::string path = NormalizeSlashes(Trim(input));
    if (path.empty()) return "";

    // no absolute path
    if (!path.empty() && path[0] == '/') return "";
    if (path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':') return "";

    std::stringstream ss(path);
    std::string segment;
    std::vector<std::string> parts;

    while (std::getline(ss, segment, '/')) {
        segment = Trim(segment);
        if (segment.empty() || segment == ".") continue;
        if (segment == "..") return "";
        parts.push_back(segment);
    }

    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out += '/';
        out += parts[i];
    }
    if (!out.empty() && out.back() != '/') out += '/';
    return out;
}

static std::vector<std::string> SplitCSV(const std::string& s)
{
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = Trim(token);
        if (!token.empty())
            out.push_back(token);
    }
    return out;
}

static std::vector<std::pair<std::string, std::string>> SplitHeaderPairs(const std::string& headerStr)
{
    std::vector<std::pair<std::string, std::string>> result;
    std::stringstream ss(headerStr);
    std::string pair;

    while (std::getline(ss, pair, '|')) {
        size_t colonPos = pair.find(':');
        if (colonPos != std::string::npos) {
            std::string key = Trim(pair.substr(0, colonPos));
            std::string val = Trim(pair.substr(colonPos + 1));
            if (!key.empty()) result.emplace_back(key, val);
        }
    }

    return result;
}

static std::unordered_map<std::string, std::string> ParseHeaderMap(const std::string& headerStr)
{
    std::unordered_map<std::string, std::string> headers;
    for (const auto& [key, value] : SplitHeaderPairs(headerStr)) {
        headers[key] = value;
    }
    return headers;
}

static void SetOrReplaceHeader(httplib::Headers& headers, const std::string& key, const std::string& value)
{
    auto existing = headers.equal_range(key);
    if (existing.first != existing.second) {
        headers.erase(existing.first, existing.second);
    }
    headers.emplace(key, value);
}

static std::string HeaderMapToString(const std::unordered_map<std::string, std::string>& headers)
{
    std::string output;
    bool first = true;
    for (const auto& [key, value] : headers) {
        if (!first) output += "|";
        output += key + ": " + value;
        first = false;
    }
    return output;
}

static std::string JoinUrlPath(const std::string& baseUrl, const std::string& path)
{
    if (path.empty()) return baseUrl;
    if (baseUrl.empty()) return path;
    if (path.find("://") != std::string::npos) return path;

    bool baseEndsWithSlash = !baseUrl.empty() && baseUrl.back() == '/';
    bool pathStartsWithSlash = !path.empty() && path.front() == '/';

    if (baseEndsWithSlash && pathStartsWithSlash) {
        return baseUrl + path.substr(1);
    }
    if (!baseEndsWithSlash && !pathStartsWithSlash) {
        return baseUrl + "/" + path;
    }
    return baseUrl + path;
}

namespace OutgoingError {
    inline constexpr int NONE = 0;
    inline constexpr int INVALID_URL = 1001;
    inline constexpr int UNSUPPORTED_SCHEME = 1002;
    inline constexpr int TLS_UNAVAILABLE = 1003;
    inline constexpr int TLS_INVALID_CERTS = 1004;
    inline constexpr int FILE_NOT_FOUND = 1005;
    inline constexpr int EMPTY_FILE = 1006;
    inline constexpr int CANCELLED = 1007;
    inline constexpr int HTTP_STATUS = 1008;
    inline constexpr int NETWORK = 1100;
    inline constexpr int TIMEOUT = 1101;
    inline constexpr int TLS_HANDSHAKE = 1102;
    inline constexpr int UNKNOWN = 1199;
    inline constexpr int JSON_PARSE = 1200;
    inline constexpr int WEBSOCKET = 1201;
}

static std::pair<int, std::string> ClassifyClientError(httplib::Error error)
{
    switch (error) {
        case httplib::Error::ConnectionTimeout:
        case httplib::Error::Timeout:
            return { OutgoingError::TIMEOUT, "timeout" };
        case httplib::Error::SSLConnection:
        case httplib::Error::SSLServerVerification:
        case httplib::Error::SSLServerHostnameVerification:
            return { OutgoingError::TLS_HANDSHAKE, "tls" };
        case httplib::Error::Connection:
        case httplib::Error::Read:
        case httplib::Error::Write:
        case httplib::Error::ConnectionClosed:
        case httplib::Error::ProxyConnection:
            return { OutgoingError::NETWORK, "network" };
        default:
            return { OutgoingError::UNKNOWN, "unknown" };
    }
}
