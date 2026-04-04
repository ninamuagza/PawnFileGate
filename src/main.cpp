#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>
#include <Server/Components/Timers/timers.hpp>

#include <httplib.h>
#include <crc32.hpp>

#include <atomic>
#include <chrono>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <cerrno>

// Platform-specific includes
#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #include <windows.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <dirent.h>
    #include <ctime>
#endif

namespace Config {
    inline constexpr int DRAIN_FPS = 30;
    inline constexpr int MAX_CONCURRENT = 16;
    inline constexpr size_t TEMP_CLEANUP_SEC = 3600;
    inline const char* TEMP_DIR = ".tmp/";
    inline constexpr int UPLOAD_TIMEOUT_SEC = 300;
    inline constexpr size_t UPLOAD_CHUNK_SIZE = 65536; // 64KB
}

class FileGateComponent;
static FileGateComponent* g_Component = nullptr;
static FileGateComponent* GetComponent() { return g_Component; }

enum class ConflictMode {
    Rename,
    Overwrite,
    Reject
};

enum class CorruptAction {
    Delete,
    Quarantine,
    Keep
};

enum class UploadStatus {
    Pending = 0,
    Uploading = 1,
    Completed = 2,
    Failed = 3,
    Cancelled = 4
};

enum class UploadMode {
    Multipart = 0,
    Raw = 1
};

struct UploadRoute {
    int         routeId = -1;
    std::string endpoint;
    std::string destinationPath;
    std::vector<std::string> allowedExtensions;
    std::unordered_set<std::string> authorizedKeys;
    size_t      maxSizeBytes = 10 * 1024 * 1024;
    ConflictMode onConflict  = ConflictMode::Rename;
    CorruptAction onCorrupt  = CorruptAction::Delete;
    std::string quarantinePath = "quarantine/";
    bool        requireCrc32 = false;
    
    // REST API permissions for file routes
    bool        allowList = false;
    bool        allowDownload = false;
    bool        allowDelete = false;
    bool        allowInfo = false;
};

struct UploadEvent {
    enum class Type { Completed, Failed, Progress };

    Type        type;
    int         uploadId  = -1;
    int         routeId   = -1;
    std::string endpoint;
    std::string filename;
    std::string filepath;
    std::string reason;
    int         progressPct = 0;

    uint32_t    crc32Checksum = 0;
    bool        crc32Matched = true;
    std::string expectedCrc32;
};

struct OutgoingUpload {
    int         uploadId = -1;
    std::string url;
    std::string filepath;
    std::string filename;
    std::string authKey;
    std::string customHeader;
    bool        calculateCrc32 = true;
    UploadMode  mode = UploadMode::Multipart;

    UploadStatus status = UploadStatus::Pending;
    int         progressPct = 0;
    uint32_t    crc32Checksum = 0;
    std::string responseBody;
    int         httpStatus = 0;
    std::string errorMessage;
    size_t      bytesUploaded = 0;
    size_t      totalBytes = 0;

    std::shared_ptr<std::atomic<bool>> cancelToken = std::make_shared<std::atomic<bool>>(false);
};

struct OutgoingUploadEvent {
    enum class Type { Started, Progress, Completed, Failed };

    Type        type;
    int         uploadId = -1;
    int         progressPct = 0;
    uint32_t    crc32Checksum = 0;
    int         httpStatus = 0;
    std::string responseBody;
    std::string errorMessage;
};

struct ValidationResult {
    bool        ok = false;
    std::string reason;
};

// ═══════════════════════════════════════════════════════════════════════════
// REST API Infrastructure
// ═══════════════════════════════════════════════════════════════════════════

enum class HttpMethod {
    GET = 0,
    POST = 1,
    PUT = 2,
    PATCH = 3,
    DELETE_ = 4  // DELETE is a macro on some platforms
};

struct APIRoute {
    int                  routeId = -1;
    HttpMethod           method = HttpMethod::GET;
    std::string          endpoint;       // e.g. "/api/players" or "/api/player/{id}"
    std::string          callbackName;   // Pawn callback name
    std::unordered_set<std::string> authKeys;
    std::vector<std::string> paramNames; // extracted from {name} in endpoint
    std::regex           pattern;        // compiled regex for matching
    bool                 requireAuth = false;
};

struct RequestContext {
    int                  requestId = -1;
    HttpMethod           method = HttpMethod::GET;
    std::string          path;
    std::string          body;
    std::string          clientIP;
    std::unordered_map<std::string, std::string> params;   // URL params like {id}
    std::unordered_map<std::string, std::string> queries;  // Query string ?a=1&b=2
    std::unordered_map<std::string, std::string> headers;
    
    // Response building
    std::string          responseJson;
    std::vector<std::string> jsonStack;  // for nested objects/arrays
    bool                 jsonStarted = false;
    std::unordered_map<std::string, std::string> responseHeaders;
    
    // State
    bool                 responded = false;
    httplib::Response*   httpRes = nullptr;
};

struct APIRequestEvent {
    int         requestId = -1;
    int         routeId = -1;
    std::string callbackName;
};

namespace Json {
    inline std::string Escape(const std::string& s) {
        std::string out;
        out.reserve(s.size() + 16);
        for (unsigned char c : s) {
            switch (c) {
                case '\"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20) {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    } else {
                        out += static_cast<char>(c);
                    }
                    break;
            }
        }
        return out;
    }

    inline std::string Unescape(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                switch (s[i + 1]) {
                    case '"': out += '"'; ++i; break;
                    case '\\': out += '\\'; ++i; break;
                    case '/': out += '/'; ++i; break;
                    case 'b': out += '\b'; ++i; break;
                    case 'f': out += '\f'; ++i; break;
                    case 'n': out += '\n'; ++i; break;
                    case 'r': out += '\r'; ++i; break;
                    case 't': out += '\t'; ++i; break;
                    case 'u':
                        if (i + 5 < s.size()) {
                            char hex[5] = { s[i+2], s[i+3], s[i+4], s[i+5], 0 };
                            unsigned int cp = std::strtoul(hex, nullptr, 16);
                            if (cp < 0x80) out += static_cast<char>(cp);
                            i += 5;
                        }
                        break;
                    default: out += s[i]; break;
                }
            } else {
                out += s[i];
            }
        }
        return out;
    }

    inline std::string Str(const std::string& key, const std::string& value, bool comma = true) {
        return "\"" + Escape(key) + "\":\"" + Escape(value) + "\"" + (comma ? "," : "");
    }

    inline std::string Num(const std::string& key, long long value, bool comma = true) {
        return "\"" + Escape(key) + "\":" + std::to_string(value) + (comma ? "," : "");
    }

    inline std::string Float(const std::string& key, double value, bool comma = true) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6f", value);
        return "\"" + Escape(key) + "\":" + std::string(buf) + (comma ? "," : "");
    }

    inline std::string Bool(const std::string& key, bool value, bool comma = true) {
        return "\"" + Escape(key) + "\":" + std::string(value ? "true" : "false") + (comma ? "," : "");
    }

    inline std::string Null(const std::string& key, bool comma = true) {
        return "\"" + Escape(key) + "\":null" + (comma ? "," : "");
    }

    inline std::string Obj(std::initializer_list<std::string> fields) {
        std::string out = "{";
        for (const auto& f : fields) out += f;
        if (!out.empty() && out.back() == ',') out.pop_back();
        out += "}";
        return out;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // JSON Parser - Simple recursive descent parser for request bodies
    // ═══════════════════════════════════════════════════════════════════════
    
    inline void SkipWhitespace(const std::string& s, size_t& pos) {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) ++pos;
    }

    inline bool ParseString(const std::string& s, size_t& pos, std::string& out) {
        SkipWhitespace(s, pos);
        if (pos >= s.size() || s[pos] != '"') return false;
        ++pos;
        
        out.clear();
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\' && pos + 1 < s.size()) {
                ++pos;
                switch (s[pos]) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    default: out += s[pos]; break;
                }
            } else {
                out += s[pos];
            }
            ++pos;
        }
        if (pos >= s.size()) return false;
        ++pos; // skip closing quote
        return true;
    }

    inline bool ParseNumber(const std::string& s, size_t& pos, double& out) {
        SkipWhitespace(s, pos);
        size_t start = pos;
        
        if (pos < s.size() && s[pos] == '-') ++pos;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
        if (pos < s.size() && s[pos] == '.') {
            ++pos;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
        }
        if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
            ++pos;
            if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) ++pos;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
        }
        
        if (pos == start) return false;
        out = std::strtod(s.substr(start, pos - start).c_str(), nullptr);
        return true;
    }

    inline bool ParseBool(const std::string& s, size_t& pos, bool& out) {
        SkipWhitespace(s, pos);
        if (s.compare(pos, 4, "true") == 0) {
            out = true;
            pos += 4;
            return true;
        }
        if (s.compare(pos, 5, "false") == 0) {
            out = false;
            pos += 5;
            return true;
        }
        return false;
    }

    inline bool ParseNull(const std::string& s, size_t& pos) {
        SkipWhitespace(s, pos);
        if (s.compare(pos, 4, "null") == 0) {
            pos += 4;
            return true;
        }
        return false;
    }

    // Skip a JSON value (for navigating to nested keys)
    inline bool SkipValue(const std::string& s, size_t& pos) {
        SkipWhitespace(s, pos);
        if (pos >= s.size()) return false;
        
        char c = s[pos];
        if (c == '"') {
            std::string dummy;
            return ParseString(s, pos, dummy);
        } else if (c == '{') {
            ++pos;
            SkipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == '}') { ++pos; return true; }
            while (pos < s.size()) {
                std::string key;
                if (!ParseString(s, pos, key)) return false;
                SkipWhitespace(s, pos);
                if (pos >= s.size() || s[pos] != ':') return false;
                ++pos;
                if (!SkipValue(s, pos)) return false;
                SkipWhitespace(s, pos);
                if (pos < s.size() && s[pos] == '}') { ++pos; return true; }
                if (pos >= s.size() || s[pos] != ',') return false;
                ++pos;
            }
            return false;
        } else if (c == '[') {
            ++pos;
            SkipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ']') { ++pos; return true; }
            while (pos < s.size()) {
                if (!SkipValue(s, pos)) return false;
                SkipWhitespace(s, pos);
                if (pos < s.size() && s[pos] == ']') { ++pos; return true; }
                if (pos >= s.size() || s[pos] != ',') return false;
                ++pos;
            }
            return false;
        } else if (c == 't' || c == 'f') {
            bool dummy;
            return ParseBool(s, pos, dummy);
        } else if (c == 'n') {
            return ParseNull(s, pos);
        } else {
            double dummy;
            return ParseNumber(s, pos, dummy);
        }
    }

    // Find a key in JSON object, return position after ':'
    inline bool FindKey(const std::string& json, const std::string& key, size_t& outPos) {
        size_t pos = 0;
        SkipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != '{') return false;
        ++pos;
        
        while (pos < json.size()) {
            SkipWhitespace(json, pos);
            if (json[pos] == '}') return false;
            
            std::string k;
            if (!ParseString(json, pos, k)) return false;
            SkipWhitespace(json, pos);
            if (pos >= json.size() || json[pos] != ':') return false;
            ++pos;
            SkipWhitespace(json, pos);
            
            if (k == key) {
                outPos = pos;
                return true;
            }
            
            if (!SkipValue(json, pos)) return false;
            SkipWhitespace(json, pos);
            if (pos < json.size() && json[pos] == ',') ++pos;
        }
        return false;
    }

    // Find nested key using dot notation: "user.profile.name"
    inline bool FindNestedKey(const std::string& json, const std::string& path, size_t& outPos, std::string& subJson) {
        subJson = json;
        std::stringstream ss(path);
        std::string segment;
        
        while (std::getline(ss, segment, '.')) {
            // Check for array index: items[0]
            size_t bracketPos = segment.find('[');
            std::string key = segment;
            int arrayIndex = -1;
            
            if (bracketPos != std::string::npos) {
                key = segment.substr(0, bracketPos);
                size_t endBracket = segment.find(']', bracketPos);
                if (endBracket != std::string::npos) {
                    arrayIndex = std::atoi(segment.substr(bracketPos + 1, endBracket - bracketPos - 1).c_str());
                }
            }
            
            if (!key.empty()) {
                size_t valPos;
                if (!FindKey(subJson, key, valPos)) return false;
                
                // Extract the value as new subJson
                size_t start = valPos;
                size_t end = valPos;
                if (!SkipValue(subJson, end)) return false;
                subJson = subJson.substr(start, end - start);
            }
            
            // Handle array index
            if (arrayIndex >= 0) {
                size_t pos = 0;
                SkipWhitespace(subJson, pos);
                if (pos >= subJson.size() || subJson[pos] != '[') return false;
                ++pos;
                
                for (int i = 0; i <= arrayIndex; ++i) {
                    SkipWhitespace(subJson, pos);
                    if (i == arrayIndex) {
                        size_t start = pos;
                        size_t end = pos;
                        if (!SkipValue(subJson, end)) return false;
                        subJson = subJson.substr(start, end - start);
                        break;
                    }
                    if (!SkipValue(subJson, pos)) return false;
                    SkipWhitespace(subJson, pos);
                    if (pos < subJson.size() && subJson[pos] == ',') ++pos;
                }
            }
        }
        
        outPos = 0;
        return true;
    }

    // High-level getters
    inline std::string GetString(const std::string& json, const std::string& key, const std::string& def = "") {
        size_t pos;
        if (!FindKey(json, key, pos)) return def;
        std::string out;
        if (!ParseString(json, pos, out)) return def;
        return out;
    }

    inline int GetInt(const std::string& json, const std::string& key, int def = 0) {
        size_t pos;
        if (!FindKey(json, key, pos)) return def;
        double val;
        if (!ParseNumber(json, pos, val)) return def;
        return static_cast<int>(val);
    }

    inline double GetFloat(const std::string& json, const std::string& key, double def = 0.0) {
        size_t pos;
        if (!FindKey(json, key, pos)) return def;
        double val;
        if (!ParseNumber(json, pos, val)) return def;
        return val;
    }

    inline bool GetBool(const std::string& json, const std::string& key, bool def = false) {
        size_t pos;
        if (!FindKey(json, key, pos)) return def;
        bool val;
        if (!ParseBool(json, pos, val)) return def;
        return val;
    }

    inline bool HasKey(const std::string& json, const std::string& key) {
        size_t pos;
        return FindKey(json, key, pos);
    }

    inline int ArrayLength(const std::string& json, const std::string& key) {
        size_t pos;
        if (!key.empty()) {
            if (!FindKey(json, key, pos)) return -1;
        } else {
            pos = 0;
        }
        
        SkipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != '[') return -1;
        ++pos;
        SkipWhitespace(json, pos);
        if (json[pos] == ']') return 0;
        
        int count = 0;
        while (pos < json.size()) {
            if (!SkipValue(json, pos)) return -1;
            ++count;
            SkipWhitespace(json, pos);
            if (pos < json.size() && json[pos] == ']') break;
            if (pos >= json.size() || json[pos] != ',') return -1;
            ++pos;
        }
        return count;
    }

    // Nested getters
    inline std::string GetNestedString(const std::string& json, const std::string& path, const std::string& def = "") {
        size_t pos;
        std::string sub;
        if (!FindNestedKey(json, path, pos, sub)) return def;
        
        SkipWhitespace(sub, pos);
        std::string out;
        if (!ParseString(sub, pos, out)) return def;
        return out;
    }

    inline int GetNestedInt(const std::string& json, const std::string& path, int def = 0) {
        size_t pos;
        std::string sub;
        if (!FindNestedKey(json, path, pos, sub)) return def;
        
        SkipWhitespace(sub, pos);
        double val;
        if (!ParseNumber(sub, pos, val)) return def;
        return static_cast<int>(val);
    }
}

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

class FileGateComponent final
    : public IComponent
    , public PawnEventHandler
{
private:
    ICore*             core    = nullptr;
    IPawnComponent*    pawn    = nullptr;
    ITimersComponent*  timers  = nullptr;
    ITimer*            drainTimer = nullptr;

    // Receive (Server)
    std::unordered_map<int, UploadRoute> routes;
    mutable std::mutex routesMutex;
    std::atomic<int> nextRouteId  { 0 };
    std::atomic<int> nextUploadId { 0 };

    std::unique_ptr<httplib::Server> httpServer;
    std::thread                      httpThread;
    std::thread                      cleanupThread;
    std::atomic<bool>                shutdownFlag { false };
    std::atomic<bool>                isRunning { false };
    std::string                      serverRootPath;
    int                              currentPort = 0;

    std::mutex               eventMutex;
    std::vector<UploadEvent> pendingEvents;

    // Upload (Client)
    std::unordered_map<int, OutgoingUpload> outgoingUploads;
    mutable std::mutex outgoingMutex;
    std::atomic<int> nextOutgoingId { 0 };

    std::mutex                       outgoingEventMutex;
    std::vector<OutgoingUploadEvent> pendingOutgoingEvents;

    std::thread uploadWorkerThread;
    std::atomic<bool> uploadWorkerRunning { false };

    // REST API
    std::unordered_map<int, APIRoute> apiRoutes;
    mutable std::mutex apiRoutesMutex;
    std::atomic<int> nextApiRouteId { 0 };
    std::atomic<int> nextRequestId { 0 };
    
    std::unordered_map<int, std::shared_ptr<RequestContext>> activeRequests;
    mutable std::mutex requestsMutex;
    
    std::mutex apiEventMutex;
    std::vector<APIRequestEvent> pendingApiEvents;

    class DrainTimerHandler : public TimerTimeOutHandler
    {
        FileGateComponent* owner;
    public:
        DrainTimerHandler(FileGateComponent* o) : owner(o) {}
        void timeout(ITimer&) override { owner->DrainEvents(); }
        void free(ITimer&) override { delete this; }
        ~DrainTimerHandler() = default;
    };

private:
    std::string MakeTempPath(int uploadId)
    {
        auto ts = std::chrono::system_clock::now().time_since_epoch().count();
        return serverRootPath + Config::TEMP_DIR + "upload_" + std::to_string(uploadId) + "_" + std::to_string(ts) + ".tmp";
    }

    std::string ToRelativePath(const std::string& absPath)
    {
        if (absPath.rfind(serverRootPath, 0) == 0)
            return absPath.substr(serverRootPath.size());
        return absPath;
    }

    std::string ResolveDestPath(
        const std::string& dir,
        const std::string& filename,
        ConflictMode mode)
    {
        std::string dest = dir;
        if (!dest.empty() && dest.back() != '/')
            dest += '/';
        dest += filename;

        if (!FileUtils::FileExists(dest))
            return dest;

        switch (mode) {
            case ConflictMode::Overwrite:
                return dest;

            case ConflictMode::Reject:
                return "";

            case ConflictMode::Rename:
            default: {
                size_t dotPos = filename.find_last_of('.');
                std::string stem = (dotPos == std::string::npos) ? filename : filename.substr(0, dotPos);
                std::string ext  = (dotPos == std::string::npos) ? "" : filename.substr(dotPos);

                for (int i = 1; i <= 9999; ++i) {
                    std::string candidate = dir;
                    if (!candidate.empty() && candidate.back() != '/')
                        candidate += '/';
                    candidate += stem + "_" + std::to_string(i) + ext;

                    if (!FileUtils::FileExists(candidate))
                        return candidate;
                }

                auto ts = std::chrono::system_clock::now().time_since_epoch().count();
                std::string fallback = dir;
                if (!fallback.empty() && fallback.back() != '/')
                    fallback += '/';
                fallback += stem + "_" + std::to_string(ts) + ext;
                return fallback;
            }
        }
    }

    ValidationResult ValidateUpload(
        const std::string& filename,
        size_t fileSize,
        const UploadRoute& route)
    {
        if (!route.allowedExtensions.empty()) {
            size_t dotPos = filename.find_last_of('.');
            std::string ext = (dotPos == std::string::npos) ? "" : filename.substr(dotPos);

            std::transform(ext.begin(), ext.end(), ext.begin(),
                [](unsigned char c){ return std::tolower(c); });

            bool allowed = std::any_of(
                route.allowedExtensions.begin(),
                route.allowedExtensions.end(),
                [&](const std::string& e) {
                    std::string eLower = e;
                    std::transform(eLower.begin(), eLower.end(), eLower.begin(),
                        [](unsigned char c){ return std::tolower(c); });
                    return eLower == ext;
                }
            );
            if (!allowed)
                return { false, "extension not allowed: " + ext };
        }

        if (fileSize > route.maxSizeBytes)
            return { false, "file too large (" + std::to_string(fileSize) + " bytes)" };

        return { true, "" };
    }

    void PushEvent(UploadEvent ev)
    {
        std::lock_guard<std::mutex> lock(eventMutex);
        pendingEvents.push_back(std::move(ev));
    }

    void PushOutgoingEvent(OutgoingUploadEvent ev)
    {
        std::lock_guard<std::mutex> lock(outgoingEventMutex);
        pendingOutgoingEvents.push_back(std::move(ev));
    }

    bool IsCancelled(int uploadId)
    {
        std::lock_guard<std::mutex> lock(outgoingMutex);
        auto it = outgoingUploads.find(uploadId);
        if (it == outgoingUploads.end()) return true;
        return it->second.status == UploadStatus::Cancelled || it->second.cancelToken->load();
    }

    void UpdateOutgoingProgress(int uploadId, size_t current, size_t total, uint32_t crcValue)
    {
        int pct = 0;
        if (total > 0) pct = static_cast<int>((current * 100) / total);

        {
            std::lock_guard<std::mutex> lock(outgoingMutex);
            auto it = outgoingUploads.find(uploadId);
            if (it != outgoingUploads.end()) {
                it->second.progressPct = pct;
                it->second.bytesUploaded = current;
                it->second.totalBytes = total;
            }
        }

        PushOutgoingEvent({
            OutgoingUploadEvent::Type::Progress,
            uploadId, pct, crcValue, 0, "", ""
        });
    }

    void HandleUploadRaw(
        const httplib::Request& req,
        httplib::Response& res,
        const httplib::ContentReader& content_reader,
        int routeId,
        const UploadRoute& route,
        const std::string& expectedCrcHex,
        uint32_t expectedCrc)
    {
        int uploadId = nextUploadId++;
        CRC32 crc32;

        std::string final_filename = SanitizeFilename(req.get_header_value("X-Filename"));
        if (final_filename.empty()) {
            res.status = 400;
            res.set_content(Json::Obj({
                Json::Str("error", "missing or invalid X-Filename", false)
            }), "application/json");
            return;
        }

        size_t expected_size = 0;
        auto clen = req.get_header_value("Content-Length");
        if (!clen.empty()) {
            try { expected_size = static_cast<size_t>(std::stoull(clen)); } catch(...) {}
        }

        auto validation = ValidateUpload(final_filename, expected_size, route);
        if (!validation.ok && expected_size > 0) {
            res.status = 422;
            res.set_content(Json::Obj({
                Json::Str("error", validation.reason, false)
            }), "application/json");
            return;
        }

        FileUtils::CreateDirectory(serverRootPath + Config::TEMP_DIR);
        std::string tempPath = MakeTempPath(uploadId);
        std::ofstream tempFile(tempPath, std::ios::binary);
        if (!tempFile.is_open()) {
            res.status = 500;
            res.set_content(Json::Obj({
                Json::Str("error", "io error: cannot open temp file", false)
            }), "application/json");
            return;
        }

        size_t written = 0;
        int lastPctBucket = -1;
        bool has_error = false;
        std::string errstr;

        content_reader([&](const char* data, size_t data_length) {
            if (has_error) return false;

            if (written + data_length > route.maxSizeBytes) {
                has_error = true;
                errstr = "file too large (" + std::to_string(written + data_length) + " bytes)";
                return false;
            }

            crc32.update(data, data_length);

            tempFile.write(data, data_length);
            if (tempFile.fail()) {
                has_error = true;
                errstr = "write error";
                return false;
            }

            written += data_length;

            if (expected_size > 0) {
                int pct = static_cast<int>((written * 100) / expected_size);
                int bucket = pct / 10;
                if (bucket != lastPctBucket) {
                    lastPctBucket = bucket;
                    PushEvent({
                        UploadEvent::Type::Progress, uploadId, routeId,
                        route.endpoint, final_filename, "", "", pct
                    });
                }
            }

            return true;
        });

        tempFile.close();

        uint32_t finalCrc = crc32.final();
        std::string finalCrcHex = CRC32::toHex(finalCrc);

        if (has_error) {
            FileUtils::RemoveFile(tempPath);
            res.status = (errstr.find("write") != std::string::npos) ? 500 : 422;
            res.set_content(Json::Obj({
                Json::Str("error", errstr, false)
            }), "application/json");

            PushEvent({
                UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                final_filename, "", errstr, 0, finalCrc, false, expectedCrcHex
            });
            return;
        }

        validation = ValidateUpload(final_filename, written, route);
        if (!validation.ok) {
            FileUtils::RemoveFile(tempPath);
            res.status = 422;
            res.set_content(Json::Obj({
                Json::Str("error", validation.reason, false)
            }), "application/json");

            PushEvent({
                UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                final_filename, "", validation.reason, 0, finalCrc, false, expectedCrcHex
            });
            return;
        }

        bool crcMatched = true;
        if (expectedCrc != 0 && finalCrc != expectedCrc) {
            crcMatched = false;

            switch (route.onCorrupt) {
                case CorruptAction::Quarantine: {
                    FileUtils::CreateDirectory(serverRootPath + route.quarantinePath);
                    std::string qPath = serverRootPath + route.quarantinePath +
                        final_filename + "." + std::to_string(uploadId) + ".corrupt";
                    FileUtils::RenameFile(tempPath, qPath);
                    break;
                }
                case CorruptAction::Keep: {
                    std::string cPath = serverRootPath + route.destinationPath +
                        final_filename + ".corrupt";
                    FileUtils::RenameFile(tempPath, cPath);
                    break;
                }
                case CorruptAction::Delete:
                default:
                    FileUtils::RemoveFile(tempPath);
                    break;
            }

            res.status = 422;
            res.set_content(Json::Obj({
                Json::Str("error", "crc32 mismatch"),
                Json::Str("received", finalCrcHex),
                Json::Str("expected", expectedCrcHex, false)
            }), "application/json");

            PushEvent({
                UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                final_filename, "", "CRC32 mismatch: got " + finalCrcHex + ", expected " + expectedCrcHex,
                100, finalCrc, false, expectedCrcHex
            });
            return;
        }

        std::string finalPath = ResolveDestPath(
            serverRootPath + route.destinationPath,
            final_filename,
            route.onConflict
        );

        if (finalPath.empty()) {
            FileUtils::RemoveFile(tempPath);
            res.status = 409;
            res.set_content(Json::Obj({
                Json::Str("error", "file already exists", false)
            }), "application/json");

            PushEvent({
                UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                final_filename, "", "conflict: file already exists", 0, finalCrc, true, expectedCrcHex
            });
            return;
        }

        if (!FileUtils::RenameFile(tempPath, finalPath)) {
            if (!FileUtils::CopyFile(tempPath, finalPath)) {
                FileUtils::RemoveFile(tempPath);
                res.status = 500;
                res.set_content(Json::Obj({
                    Json::Str("error", "failed to move file", false)
                }), "application/json");

                PushEvent({
                    UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                    final_filename, "", "move error", 0, finalCrc, true, expectedCrcHex
                });
                return;
            }
            FileUtils::RemoveFile(tempPath);
        }

        std::string relativePath = ToRelativePath(finalPath);

        PushEvent({
            UploadEvent::Type::Completed,
            uploadId, routeId,
            route.endpoint,
            final_filename,
            relativePath,
            "", 100, finalCrc, crcMatched, expectedCrcHex
        });

        res.status = 200;
        res.set_content(Json::Obj({
            Json::Num("uploadId", uploadId),
            Json::Str("path", relativePath),
            Json::Str("crc32", finalCrcHex),
            Json::Num("size", static_cast<long long>(written), false)
        }), "application/json");
    }

    void HandleUploadMultipart(
        const httplib::Request& req,
        httplib::Response& res,
        const httplib::ContentReader& content_reader,
        int routeId,
        const UploadRoute& route,
        const std::string& expectedCrcHex,
        uint32_t expectedCrc)
    {
        int uploadId = nextUploadId++;
        CRC32 crc32;

        std::string final_filename;
        std::string tempPath;
        std::ofstream tempFile;
        bool is_target_file = false;
        size_t written = 0;
        int lastPctBucket = -1;
        bool has_error = false;
        std::string errstr;

        size_t expected_size = 0;
        auto clen = req.get_header_value("Content-Length");
        if (!clen.empty()) {
            try { expected_size = static_cast<size_t>(std::stoull(clen)); } catch(...) {}
        }

        content_reader(
            [&](const httplib::FormData& file) {
                if (file.name == "file") {
                    is_target_file = true;
                    final_filename = SanitizeFilename(file.filename);
                    crc32.reset();

                    if (final_filename.empty()) {
                        is_target_file = false;
                        has_error = true;
                        errstr = "invalid filename";
                        return false;
                    }

                    auto validation = ValidateUpload(final_filename, expected_size, route);
                    if (!validation.ok && expected_size > 0) {
                        is_target_file = false;
                        has_error = true;
                        errstr = validation.reason;
                        return false;
                    }

                    FileUtils::CreateDirectory(serverRootPath + Config::TEMP_DIR);
                    tempPath = MakeTempPath(uploadId);

                    tempFile.open(tempPath, std::ios::binary);
                    if (!tempFile.is_open()) {
                        has_error = true;
                        errstr = "io error: cannot open temp file";
                        return false;
                    }
                } else {
                    is_target_file = false;
                }
                return true;
            },
            [&](const char* data, size_t data_length) {
                if (has_error) return false;
                if (!is_target_file) return true;

                if (written + data_length > route.maxSizeBytes) {
                    has_error = true;
                    errstr = "file too large (" + std::to_string(written + data_length) + " bytes)";
                    return false;
                }

                crc32.update(data, data_length);

                tempFile.write(data, data_length);
                if (tempFile.fail()) {
                    has_error = true;
                    errstr = "write error";
                    return false;
                }

                written += data_length;
                if (expected_size > 0) {
                    int pct = static_cast<int>((written * 100) / expected_size);
                    int bucket = pct / 10;
                    if (bucket != lastPctBucket) {
                        lastPctBucket = bucket;
                        PushEvent({
                            UploadEvent::Type::Progress, uploadId, routeId,
                            route.endpoint, final_filename, "", "", pct
                        });
                    }
                }
                return true;
            }
        );

        if (tempFile.is_open()) tempFile.close();

        uint32_t finalCrc = crc32.final();
        std::string finalCrcHex = CRC32::toHex(finalCrc);

        if (has_error) {
            res.status = 400;
            if (errstr.find("io error") != std::string::npos || errstr.find("write error") != std::string::npos) {
                res.status = 500;
            } else if (errstr.find("too large") != std::string::npos || errstr.find("extension") != std::string::npos) {
                res.status = 422;
            }
            res.set_content(Json::Obj({
                Json::Str("error", errstr, false)
            }), "application/json");

            if (!tempPath.empty()) {
                FileUtils::RemoveFile(tempPath);
            }
            PushEvent({
                UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                final_filename, "", errstr, 0, finalCrc, false, expectedCrcHex
            });
            return;
        }

        if (final_filename.empty()) {
            res.status = 400;
            res.set_content(Json::Obj({
                Json::Str("error", "no file field found", false)
            }), "application/json");
            return;
        }

        auto validation = ValidateUpload(final_filename, written, route);
        if (!validation.ok) {
            res.status = 422;
            res.set_content(Json::Obj({
                Json::Str("error", validation.reason, false)
            }), "application/json");
            FileUtils::RemoveFile(tempPath);
            PushEvent({
                UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                final_filename, "", validation.reason, 0, finalCrc, false, expectedCrcHex
            });
            return;
        }

        bool crcMatched = true;
        if (expectedCrc != 0 && finalCrc != expectedCrc) {
            crcMatched = false;

            switch (route.onCorrupt) {
                case CorruptAction::Quarantine: {
                    FileUtils::CreateDirectory(serverRootPath + route.quarantinePath);
                    std::string qPath = serverRootPath + route.quarantinePath +
                        final_filename + "." + std::to_string(uploadId) + ".corrupt";
                    FileUtils::RenameFile(tempPath, qPath);
                    break;
                }
                case CorruptAction::Keep: {
                    std::string cPath = serverRootPath + route.destinationPath +
                        final_filename + ".corrupt";
                    FileUtils::RenameFile(tempPath, cPath);
                    break;
                }
                case CorruptAction::Delete:
                default:
                    FileUtils::RemoveFile(tempPath);
                    break;
            }

            res.status = 422;
            res.set_content(Json::Obj({
                Json::Str("error", "crc32 mismatch"),
                Json::Str("received", finalCrcHex),
                Json::Str("expected", expectedCrcHex, false)
            }), "application/json");

            PushEvent({
                UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                final_filename, "", "CRC32 mismatch: got " + finalCrcHex + ", expected " + expectedCrcHex,
                100, finalCrc, false, expectedCrcHex
            });
            return;
        }

        std::string finalPath = ResolveDestPath(
            serverRootPath + route.destinationPath,
            final_filename,
            route.onConflict
        );

        if (finalPath.empty()) {
            FileUtils::RemoveFile(tempPath);
            res.status = 409;
            res.set_content(Json::Obj({
                Json::Str("error", "file already exists", false)
            }), "application/json");
            PushEvent({
                UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                final_filename, "", "conflict: file already exists", 0, finalCrc, true, expectedCrcHex
            });
            return;
        }

        if (!FileUtils::RenameFile(tempPath, finalPath)) {
            if (!FileUtils::CopyFile(tempPath, finalPath)) {
                FileUtils::RemoveFile(tempPath);
                res.status = 500;
                res.set_content(Json::Obj({
                    Json::Str("error", "failed to move file", false)
                }), "application/json");
                PushEvent({
                    UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                    final_filename, "", "move error", 0, finalCrc, true, expectedCrcHex
                });
                return;
            }
            FileUtils::RemoveFile(tempPath);
        }

        std::string relativePath = ToRelativePath(finalPath);

        PushEvent({
            UploadEvent::Type::Completed,
            uploadId, routeId,
            route.endpoint,
            final_filename,
            relativePath,
            "", 100, finalCrc, crcMatched, expectedCrcHex
        });

        res.status = 200;
        res.set_content(Json::Obj({
            Json::Num("uploadId", uploadId),
            Json::Str("path", relativePath),
            Json::Str("crc32", finalCrcHex),
            Json::Num("size", static_cast<long long>(written), false)
        }), "application/json");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // RECEIVE ENTRY
    // ─────────────────────────────────────────────────────────────────────────
    void HandleUploadStreaming(
        const httplib::Request& req,
        httplib::Response& res,
        const httplib::ContentReader& content_reader,
        int routeId)
    {
        std::unique_lock<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) {
            res.status = 404;
            res.set_content(Json::Obj({
                Json::Str("error", "route not found", false)
            }), "application/json");
            return;
        }
        const UploadRoute route = it->second;
        lock.unlock();

        // Auth
        std::string authKey;
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.size() > 7 && authHeader.substr(0, 7) == "Bearer ")
            authKey = authHeader.substr(7);

        if (!route.authorizedKeys.empty() &&
            route.authorizedKeys.find(authKey) == route.authorizedKeys.end())
        {
            res.status = 401;
            res.set_content(Json::Obj({
                Json::Str("error", "unauthorized", false)
            }), "application/json");
            return;
        }

        // CRC32
        std::string expectedCrcHex = req.get_header_value("X-File-CRC32");
        uint32_t expectedCrc = expectedCrcHex.empty() ? 0 : CRC32::fromHex(expectedCrcHex);

        if (route.requireCrc32 && expectedCrcHex.empty()) {
            res.status = 400;
            res.set_content(Json::Obj({
                Json::Str("error", "X-File-CRC32 header required", false)
            }), "application/json");
            return;
        }

        if (req.is_multipart_form_data()) {
            HandleUploadMultipart(req, res, content_reader, routeId, route, expectedCrcHex, expectedCrc);
        } else {
            HandleUploadRaw(req, res, content_reader, routeId, route, expectedCrcHex, expectedCrc);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // UPLOAD (CLIENT)
    // ═══════════════════════════════════════════════════════════════════════
    void UploadWorker()
    {
        while (uploadWorkerRunning) {
            int uploadId = -1;
            {
                std::lock_guard<std::mutex> lock(outgoingMutex);
                for (auto& [id, upload] : outgoingUploads) {
                    if (upload.status == UploadStatus::Pending) {
                        upload.status = UploadStatus::Uploading;
                        uploadId = id;
                        break;
                    }
                }
            }

            if (uploadId == -1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            ProcessOutgoingUpload(uploadId);
        }
    }

    void ProcessOutgoingUpload(int uploadId)
    {
        OutgoingUpload upload;
        {
            std::lock_guard<std::mutex> lock(outgoingMutex);
            auto it = outgoingUploads.find(uploadId);
            if (it == outgoingUploads.end()) return;
            upload = it->second;
        }

        PushOutgoingEvent({
            OutgoingUploadEvent::Type::Started,
            uploadId, 0, 0, 0, "", ""
        });

        std::string scheme, host, path;
        int port = 80;
        if (!ParseUrl(upload.url, scheme, host, port, path)) {
            FailOutgoingUpload(uploadId, "invalid URL");
            return;
        }

        if (scheme != "http") {
            FailOutgoingUpload(uploadId, "only http:// is supported in v2");
            return;
        }

        std::string fullPath = serverRootPath + upload.filepath;
        if (!FileUtils::FileExists(fullPath)) {
            FailOutgoingUpload(uploadId, "file not found: " + fullPath);
            return;
        }

        size_t fileSize = FileUtils::FileSize(fullPath);
        if (fileSize == 0) {
            FailOutgoingUpload(uploadId, "empty file");
            return;
        }

        uint32_t crcValue = 0;
        std::string crcHex;
        if (upload.calculateCrc32) {
            crcValue = CRC32::fileChecksum(fullPath);
            crcHex = CRC32::toHex(crcValue);
        }

        {
            std::lock_guard<std::mutex> lock(outgoingMutex);
            auto it = outgoingUploads.find(uploadId);
            if (it != outgoingUploads.end()) {
                it->second.totalBytes = fileSize;
            }
        }

        httplib::Client client(host, port);
        client.set_connection_timeout(Config::UPLOAD_TIMEOUT_SEC);
        client.set_read_timeout(Config::UPLOAD_TIMEOUT_SEC);
        client.set_write_timeout(Config::UPLOAD_TIMEOUT_SEC);
        client.set_follow_location(true);

        httplib::Headers headers;
        if (!upload.authKey.empty()) {
            headers.emplace("Authorization", "Bearer " + upload.authKey);
        }
        if (!crcHex.empty()) {
            headers.emplace("X-File-CRC32", crcHex);
        }
        if (!upload.customHeader.empty()) {
            auto pairs = SplitHeaderPairs(upload.customHeader);
            for (auto& [key, val] : pairs) {
                headers.emplace(key, val);
            }
        }

        auto cancelToken = upload.cancelToken;

        // FIX: use httplib::Result instead of std::shared_ptr<httplib::Response>
        httplib::Result res(nullptr, httplib::Error::Unknown);
        if (upload.mode == UploadMode::Raw) {
            res = UploadRaw(client, uploadId, fullPath, upload.filename, fileSize, headers, crcValue, path, cancelToken);
        } else {
            res = UploadMultipart(client, uploadId, fullPath, upload.filename, fileSize, headers, crcValue, path, cancelToken);
        }

        if (IsCancelled(uploadId)) {
            {
                std::lock_guard<std::mutex> lock(outgoingMutex);
                auto it = outgoingUploads.find(uploadId);
                if (it != outgoingUploads.end()) {
                    it->second.status = UploadStatus::Cancelled;
                    it->second.errorMessage = "upload cancelled";
                }
            }

            PushOutgoingEvent({
                OutgoingUploadEvent::Type::Failed,
                uploadId, 0, crcValue, 0, "", "upload cancelled"
            });
            return;
        }

        if (!res) {
            FailOutgoingUpload(uploadId, "HTTP request failed");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(outgoingMutex);
            auto it = outgoingUploads.find(uploadId);
            if (it != outgoingUploads.end()) {
                it->second.httpStatus = res->status;
                it->second.responseBody = res->body;
                it->second.crc32Checksum = crcValue;
                it->second.bytesUploaded = fileSize;
                it->second.progressPct = 100;

                if (res->status >= 200 && res->status < 300) {
                    it->second.status = UploadStatus::Completed;
                } else {
                    it->second.status = UploadStatus::Failed;
                    it->second.errorMessage = "HTTP status " + std::to_string(res->status);
                }
            }
        }

        if (res->status >= 200 && res->status < 300) {
            PushOutgoingEvent({
                OutgoingUploadEvent::Type::Completed,
                uploadId, 100, crcValue, res->status, res->body, ""
            });
        } else {
            PushOutgoingEvent({
                OutgoingUploadEvent::Type::Failed,
                uploadId, 100, crcValue, res->status, res->body, "HTTP status " + std::to_string(res->status)
            });
        }
    }

    // FIX: return httplib::Result instead of std::shared_ptr<httplib::Response>
    httplib::Result UploadRaw(
        httplib::Client& client,
        int uploadId,
        const std::string& fullPath,
        const std::string& sendFilename,
        size_t fileSize,
        httplib::Headers headers,
        uint32_t crcValue,
        const std::string& path,
        std::shared_ptr<std::atomic<bool>> cancelToken)
    {
        headers.emplace("Content-Type", "application/octet-stream");
        headers.emplace("X-Filename", sendFilename.empty() ? SanitizeFilename(fullPath) : SanitizeFilename(sendFilename));

        auto file = std::make_shared<std::ifstream>(fullPath, std::ios::binary);
        if (!file->is_open()) {
            return httplib::Result(nullptr, httplib::Error::Unknown);
        }

        size_t sent = 0;
        int lastBucket = -1;

        // FIX: use [=] instead of [=, this] for C++17 compatibility
        auto provider = [=](size_t offset, size_t length, httplib::DataSink& sink) mutable {
            (void)offset;

            if (cancelToken->load()) {
                sink.done();
                return false;
            }

            std::vector<char> buf(Config::UPLOAD_CHUNK_SIZE);
            file->read(buf.data(), static_cast<std::streamsize>(std::min(length, buf.size())));
            std::streamsize got = file->gcount();

            if (got > 0) {
                sink.write(buf.data(), static_cast<size_t>(got));
                sent += static_cast<size_t>(got);
                return true;
            }

            sink.done();
            return true;
        };

        // Progress updates happen via a separate polling mechanism or
        // we track bytes in the provider and call UpdateOutgoingProgress.
        // Since provider captures by value and UpdateOutgoingProgress needs 'this',
        // we do a simpler approach: post-upload final progress.
        // For real progress during raw upload, use a wrapper approach below.

        // Use ContentProviderWithoutLength for streaming
        httplib::Result result = client.Post(path, headers, fileSize, provider, "application/octet-stream");

        file->close();

        if (result && !cancelToken->load()) {
            UpdateOutgoingProgress(uploadId, fileSize, fileSize, crcValue);
        }

        return result;
    }

    // FIX: return httplib::Result instead of std::shared_ptr<httplib::Response>
    // FIX: use httplib::UploadFormDataItems / httplib::UploadFormData with correct field names
    httplib::Result UploadMultipart(
        httplib::Client& client,
        int uploadId,
        const std::string& fullPath,
        const std::string& sendFilename,
        size_t fileSize,
        httplib::Headers headers,
        uint32_t crcValue,
        const std::string& path,
        std::shared_ptr<std::atomic<bool>> cancelToken)
    {
        // NOTE:
        // httplib multipart streaming API varies between versions.
        // For large files, prefer RAW mode which is fully streaming.
        // Multipart here uses a memory buffer for compatibility stability.

        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            return httplib::Result(nullptr, httplib::Error::Unknown);
        }

        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        if (!file) {
            return httplib::Result(nullptr, httplib::Error::Unknown);
        }

        UpdateOutgoingProgress(uploadId, fileSize / 2, fileSize, crcValue);

        // FIX: use UploadFormDataItems and UploadFormData with correct field names
        httplib::UploadFormDataItems items;
        httplib::UploadFormData fileItem;
        fileItem.name         = "file";
        fileItem.filename     = sendFilename.empty() ? SanitizeFilename(fullPath) : SanitizeFilename(sendFilename);
        fileItem.content_type = "application/octet-stream";
        fileItem.content.assign(buffer.data(), buffer.size());
        items.push_back(std::move(fileItem));

        if (cancelToken->load()) {
            return httplib::Result(nullptr, httplib::Error::Canceled);
        }

        httplib::Result result = client.Post(path, headers, items);

        if (!cancelToken->load()) {
            UpdateOutgoingProgress(uploadId, fileSize, fileSize, crcValue);
        }

        return result;
    }

    void FailOutgoingUpload(int uploadId, const std::string& reason)
    {
        {
            std::lock_guard<std::mutex> lock(outgoingMutex);
            auto it = outgoingUploads.find(uploadId);
            if (it != outgoingUploads.end()) {
                it->second.status = UploadStatus::Failed;
                it->second.errorMessage = reason;
            }
        }

        PushOutgoingEvent({
            OutgoingUploadEvent::Type::Failed,
            uploadId, 0, 0, 0, "", reason
        });
    }

    bool ParseUrl(const std::string& url, std::string& scheme, std::string& host, int& port, std::string& path)
    {
        size_t schemePos = url.find("://");
        if (schemePos == std::string::npos) {
            scheme = "http";
            schemePos = 0;
        } else {
            scheme = url.substr(0, schemePos);
            schemePos += 3;
        }

        size_t pathPos = url.find('/', schemePos);
        if (pathPos == std::string::npos) {
            host = url.substr(schemePos);
            path = "/";
        } else {
            host = url.substr(schemePos, pathPos - schemePos);
            path = url.substr(pathPos);
        }

        size_t portPos = host.find(':');
        if (portPos != std::string::npos) {
            try {
                port = std::stoi(host.substr(portPos + 1));
            } catch (...) {
                return false;
            }
            host = host.substr(0, portPos);
        } else {
            port = 80;
        }

        return !host.empty();
    }

    void DrainEvents()
    {
        if (!pawn) return;

        // Incoming
        std::vector<UploadEvent> localIncoming;
        {
            std::lock_guard<std::mutex> lock(eventMutex);
            localIncoming.swap(pendingEvents);
        }

        for (const auto& ev : localIncoming) {
            switch (ev.type) {
                case UploadEvent::Type::Completed: {
                    std::string crcHex = CRC32::toHex(ev.crc32Checksum);
                    CallPawnEvent("OnFileUploaded",
                        ev.uploadId,
                        ev.routeId,
                        StringView(ev.endpoint),
                        StringView(ev.filename),
                        StringView(ev.filepath),
                        StringView(crcHex),
                        ev.crc32Matched ? 1 : 0
                    );
                    break;
                }
                case UploadEvent::Type::Failed: {
                    std::string crcHex = CRC32::toHex(ev.crc32Checksum);
                    CallPawnEvent("OnFileFailedUpload",
                        ev.uploadId,
                        StringView(ev.reason),
                        StringView(crcHex)
                    );
                    break;
                }
                case UploadEvent::Type::Progress:
                    CallPawnEvent("OnUploadProgress",
                        ev.uploadId,
                        ev.progressPct
                    );
                    break;
            }
        }

        // Outgoing
        std::vector<OutgoingUploadEvent> localOutgoing;
        {
            std::lock_guard<std::mutex> lock(outgoingEventMutex);
            localOutgoing.swap(pendingOutgoingEvents);
        }

        for (const auto& ev : localOutgoing) {
            switch (ev.type) {
                case OutgoingUploadEvent::Type::Started:
                    CallPawnEvent("OnFileUploadStarted", ev.uploadId);
                    break;
                case OutgoingUploadEvent::Type::Progress:
                    CallPawnEvent("OnFileUploadProgress", ev.uploadId, ev.progressPct);
                    break;
                case OutgoingUploadEvent::Type::Completed: {
                    std::string crcHex = CRC32::toHex(ev.crc32Checksum);
                    CallPawnEvent("OnFileUploadCompleted",
                        ev.uploadId,
                        ev.httpStatus,
                        StringView(ev.responseBody),
                        StringView(crcHex)
                    );
                    break;
                }
                case OutgoingUploadEvent::Type::Failed:
                    CallPawnEvent("OnFileUploadFailed",
                        ev.uploadId,
                        StringView(ev.errorMessage)
                    );
                    break;
            }
        }

        // API Events
        DrainApiEvents();
    }

    template<typename... Args>
    void CallPawnEvent(const char* name, Args&&... args)
    {
        if (!pawn) return;

        if (auto script = pawn->mainScript())
            script->Call(name, DefaultReturnValue_False, std::forward<Args>(args)...);

        for (IPawnScript* script : pawn->sideScripts())
            script->Call(name, DefaultReturnValue_False, std::forward<Args>(args)...);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // REST API Router
    // ═══════════════════════════════════════════════════════════════════════════

    std::pair<std::regex, std::vector<std::string>> CompileEndpointPattern(const std::string& endpoint)
    {
        std::vector<std::string> paramNames;
        std::string pattern = "^";
        
        size_t i = 0;
        while (i < endpoint.size()) {
            if (endpoint[i] == '{') {
                size_t end = endpoint.find('}', i);
                if (end != std::string::npos) {
                    paramNames.push_back(endpoint.substr(i + 1, end - i - 1));
                    pattern += "([^/]+)";
                    i = end + 1;
                    continue;
                }
            }
            
            // Escape regex special chars
            char c = endpoint[i];
            if (c == '.' || c == '+' || c == '*' || c == '?' || 
                c == '^' || c == '$' || c == '(' || c == ')' ||
                c == '[' || c == ']' || c == '|' || c == '\\') {
                pattern += '\\';
            }
            pattern += c;
            ++i;
        }
        
        pattern += "$";
        return { std::regex(pattern), paramNames };
    }

    std::unordered_map<std::string, std::string> ParseQueryString(const std::string& query)
    {
        std::unordered_map<std::string, std::string> result;
        std::stringstream ss(query);
        std::string pair;
        
        while (std::getline(ss, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                std::string key = pair.substr(0, eq);
                std::string val = pair.substr(eq + 1);
                // Basic URL decode
                std::string decoded;
                for (size_t i = 0; i < val.size(); ++i) {
                    if (val[i] == '%' && i + 2 < val.size()) {
                        char hex[3] = { val[i+1], val[i+2], 0 };
                        decoded += static_cast<char>(std::strtol(hex, nullptr, 16));
                        i += 2;
                    } else if (val[i] == '+') {
                        decoded += ' ';
                    } else {
                        decoded += val[i];
                    }
                }
                result[key] = decoded;
            }
        }
        return result;
    }

    APIRoute* MatchApiRoute(HttpMethod method, const std::string& path, std::unordered_map<std::string, std::string>& params)
    {
        std::lock_guard<std::mutex> lock(apiRoutesMutex);
        
        for (auto& kv : apiRoutes) {
            APIRoute& route = kv.second;
            if (route.method != method) continue;
            
            std::smatch match;
            if (std::regex_match(path, match, route.pattern)) {
                params.clear();
                for (size_t i = 0; i < route.paramNames.size() && i + 1 < match.size(); ++i) {
                    params[route.paramNames[i]] = match[i + 1].str();
                }
                return &route;
            }
        }
        return nullptr;
    }

    bool CheckApiAuth(const APIRoute& route, const httplib::Request& req)
    {
        if (!route.requireAuth || route.authKeys.empty()) return true;
        
        auto auth = req.get_header_value("Authorization");
        if (auth.empty()) return false;
        
        if (auth.rfind("Bearer ", 0) == 0) {
            std::string token = auth.substr(7);
            return route.authKeys.count(token) > 0;
        }
        return false;
    }

    std::shared_ptr<RequestContext> CreateRequestContext(
        HttpMethod method,
        const httplib::Request& req,
        httplib::Response& res)
    {
        auto ctx = std::make_shared<RequestContext>();
        ctx->requestId = nextRequestId++;
        ctx->method = method;
        ctx->path = req.path;
        ctx->body = req.body;
        ctx->clientIP = req.remote_addr;
        ctx->httpRes = &res;
        
        // Parse query string
        if (!req.params.empty()) {
            for (const auto& kv : req.params) {
                ctx->queries[kv.first] = kv.second;
            }
        }
        
        // Copy relevant headers
        for (const auto& kv : req.headers) {
            ctx->headers[kv.first] = kv.second;
        }
        
        return ctx;
    }

    void HandleApiRequest(HttpMethod method, const httplib::Request& req, httplib::Response& res)
    {
        std::string path = req.path;
        std::unordered_map<std::string, std::string> urlParams;
        
        APIRoute* route = MatchApiRoute(method, path, urlParams);
        if (!route) {
            res.status = 404;
            res.set_content(Json::Obj({
                Json::Bool("success", false),
                Json::Str("error", "Endpoint not found", false)
            }), "application/json");
            return;
        }
        
        if (!CheckApiAuth(*route, req)) {
            res.status = 401;
            res.set_content(Json::Obj({
                Json::Bool("success", false),
                Json::Str("error", "Unauthorized", false)
            }), "application/json");
            return;
        }
        
        auto ctx = CreateRequestContext(method, req, res);
        ctx->params = urlParams;
        
        {
            std::lock_guard<std::mutex> lock(requestsMutex);
            activeRequests[ctx->requestId] = ctx;
        }
        
        // Queue callback event
        {
            std::lock_guard<std::mutex> lock(apiEventMutex);
            pendingApiEvents.push_back({
                ctx->requestId,
                route->routeId,
                route->callbackName
            });
        }
        
        // Wait for response (with timeout)
        auto start = std::chrono::steady_clock::now();
        while (!ctx->responded) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > std::chrono::seconds(30)) {
                res.status = 504;
                res.set_content(Json::Obj({
                    Json::Bool("success", false),
                    Json::Str("error", "Gateway timeout", false)
                }), "application/json");
                break;
            }
        }
        
        // Cleanup
        {
            std::lock_guard<std::mutex> lock(requestsMutex);
            activeRequests.erase(ctx->requestId);
        }
    }

    void DrainApiEvents()
    {
        std::vector<APIRequestEvent> events;
        {
            std::lock_guard<std::mutex> lock(apiEventMutex);
            events.swap(pendingApiEvents);
        }
        
        for (const auto& ev : events) {
            CallPawnEvent(ev.callbackName.c_str(), ev.requestId);
        }
    }

    void SetupApiHandlers()
    {
        if (!httpServer) return;
        
        // Generic handlers for all methods
        auto handleGet = [this](const httplib::Request& req, httplib::Response& res) {
            HandleApiRequest(HttpMethod::GET, req, res);
        };
        auto handlePost = [this](const httplib::Request& req, httplib::Response& res) {
            HandleApiRequest(HttpMethod::POST, req, res);
        };
        auto handlePut = [this](const httplib::Request& req, httplib::Response& res) {
            HandleApiRequest(HttpMethod::PUT, req, res);
        };
        auto handlePatch = [this](const httplib::Request& req, httplib::Response& res) {
            HandleApiRequest(HttpMethod::PATCH, req, res);
        };
        auto handleDelete = [this](const httplib::Request& req, httplib::Response& res) {
            HandleApiRequest(HttpMethod::DELETE_, req, res);
        };
        
        // Register all API routes
        std::lock_guard<std::mutex> lock(apiRoutesMutex);
        for (const auto& kv : apiRoutes) {
            const APIRoute& route = kv.second;
            std::string endpoint = route.endpoint;
            
            // Convert {param} to regex pattern for httplib
            std::string httpPattern = endpoint;
            size_t pos = 0;
            while ((pos = httpPattern.find('{', pos)) != std::string::npos) {
                size_t end = httpPattern.find('}', pos);
                if (end != std::string::npos) {
                    httpPattern.replace(pos, end - pos + 1, "([^/]+)");
                } else {
                    break;
                }
            }
            
            switch (route.method) {
                case HttpMethod::GET:
                    httpServer->Get(httpPattern.c_str(), handleGet);
                    break;
                case HttpMethod::POST:
                    httpServer->Post(httpPattern.c_str(), handlePost);
                    break;
                case HttpMethod::PUT:
                    httpServer->Put(httpPattern.c_str(), handlePut);
                    break;
                case HttpMethod::PATCH:
                    httpServer->Patch(httpPattern.c_str(), handlePatch);
                    break;
                case HttpMethod::DELETE_:
                    httpServer->Delete(httpPattern.c_str(), handleDelete);
                    break;
            }
        }
        
        // Built-in endpoints
        httpServer->Get("/health", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(Json::Obj({
                Json::Bool("success", true),
                Json::Str("status", "healthy"),
                Json::Num("uptime", static_cast<long long>(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now().time_since_epoch()
                    ).count()
                ), false)
            }), "application/json");
        });
        
        httpServer->Get("/stats", [this](const httplib::Request&, httplib::Response& res) {
            std::lock_guard<std::mutex> lock1(routesMutex);
            std::lock_guard<std::mutex> lock2(apiRoutesMutex);
            std::lock_guard<std::mutex> lock3(requestsMutex);
            
            res.set_content(Json::Obj({
                Json::Bool("success", true),
                Json::Num("fileRoutes", static_cast<long long>(routes.size())),
                Json::Num("apiRoutes", static_cast<long long>(apiRoutes.size())),
                Json::Num("activeRequests", static_cast<long long>(activeRequests.size()), false)
            }), "application/json");
        });
        
        // File route REST API endpoints
        SetupFileRouteHandlers();
    }
    
    void SetupFileRouteHandlers()
    {
        if (!httpServer) return;
        
        std::lock_guard<std::mutex> lock(routesMutex);
        for (const auto& kv : routes) {
            int capturedId = kv.first;
            std::string endpoint = kv.second.endpoint;
            
            // GET {endpoint}/files - List files
            httpServer->Get((endpoint + "/files").c_str(), 
                [this, capturedId](const httplib::Request& req, httplib::Response& res) {
                    HandleFileList(req, res, capturedId);
                });
            
            // GET {endpoint}/files/{filename} - Download file
            httpServer->Get((endpoint + "/files/(.+)").c_str(),
                [this, capturedId](const httplib::Request& req, httplib::Response& res) {
                    if (req.path.find("/info") != std::string::npos) return; // Skip if /info
                    HandleFileDownload(req, res, capturedId);
                });
            
            // GET {endpoint}/files/{filename}/info - File info
            httpServer->Get((endpoint + "/files/(.+)/info").c_str(),
                [this, capturedId](const httplib::Request& req, httplib::Response& res) {
                    HandleFileInfo(req, res, capturedId);
                });
            
            // DELETE {endpoint}/files/{filename} - Delete file
            httpServer->Delete((endpoint + "/files/(.+)").c_str(),
                [this, capturedId](const httplib::Request& req, httplib::Response& res) {
                    HandleFileDelete(req, res, capturedId);
                });
        }
    }
    
    bool CheckFileRouteAuth(const UploadRoute& route, const httplib::Request& req)
    {
        if (route.authorizedKeys.empty()) return true;
        
        auto auth = req.get_header_value("Authorization");
        if (auth.empty()) return false;
        
        if (auth.rfind("Bearer ", 0) == 0) {
            std::string token = auth.substr(7);
            return route.authorizedKeys.count(token) > 0;
        }
        return false;
    }
    
    void HandleFileList(const httplib::Request& req, httplib::Response& res, int routeId)
    {
        UploadRoute route;
        {
            std::lock_guard<std::mutex> lock(routesMutex);
            auto it = routes.find(routeId);
            if (it == routes.end()) {
                res.status = 404;
                res.set_content(Json::Obj({ Json::Str("error", "Route not found", false) }), "application/json");
                return;
            }
            route = it->second;
        }
        
        if (!route.allowList) {
            res.status = 403;
            res.set_content(Json::Obj({ Json::Str("error", "File listing not allowed", false) }), "application/json");
            return;
        }
        
        if (!CheckFileRouteAuth(route, req)) {
            res.status = 401;
            res.set_content(Json::Obj({ Json::Str("error", "Unauthorized", false) }), "application/json");
            return;
        }
        
        // Call Pawn callback for permission check
        // For now, just list files
        std::string destPath = serverRootPath + route.destinationPath;
        std::vector<std::string> files;
        
        #ifdef _WIN32
            std::string searchPath = destPath + "*";
            WIN32_FIND_DATAA findData;
            HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        files.push_back(findData.cFileName);
                    }
                } while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
        #else
            DIR* dir = opendir(destPath.c_str());
            if (dir) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    if (entry->d_type == DT_REG) {
                        files.push_back(entry->d_name);
                    }
                }
                closedir(dir);
            }
        #endif
        
        std::string json = "{\"success\":true,\"count\":" + std::to_string(files.size()) + ",\"files\":[";
        for (size_t i = 0; i < files.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + Json::Escape(files[i]) + "\"";
        }
        json += "]}";
        
        res.set_content(json, "application/json");
    }
    
    void HandleFileDownload(const httplib::Request& req, httplib::Response& res, int routeId)
    {
        UploadRoute route;
        {
            std::lock_guard<std::mutex> lock(routesMutex);
            auto it = routes.find(routeId);
            if (it == routes.end()) {
                res.status = 404;
                res.set_content(Json::Obj({ Json::Str("error", "Route not found", false) }), "application/json");
                return;
            }
            route = it->second;
        }
        
        if (!route.allowDownload) {
            res.status = 403;
            res.set_content(Json::Obj({ Json::Str("error", "File download not allowed", false) }), "application/json");
            return;
        }
        
        if (!CheckFileRouteAuth(route, req)) {
            res.status = 401;
            res.set_content(Json::Obj({ Json::Str("error", "Unauthorized", false) }), "application/json");
            return;
        }
        
        // Extract filename from path
        std::string filename;
        std::string pathPrefix = route.endpoint + "/files/";
        if (req.path.rfind(pathPrefix, 0) == 0) {
            filename = req.path.substr(pathPrefix.size());
        }
        
        std::string safeName = SanitizeFilename(filename);
        if (safeName.empty()) {
            res.status = 400;
            res.set_content(Json::Obj({ Json::Str("error", "Invalid filename", false) }), "application/json");
            return;
        }
        
        std::string fullPath = serverRootPath + route.destinationPath + safeName;
        if (!FileUtils::FileExists(fullPath)) {
            res.status = 404;
            res.set_content(Json::Obj({ Json::Str("error", "File not found", false) }), "application/json");
            return;
        }
        
        // Read and send file
        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            res.status = 500;
            res.set_content(Json::Obj({ Json::Str("error", "Failed to read file", false) }), "application/json");
            return;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        res.set_header("Content-Disposition", "attachment; filename=\"" + safeName + "\"");
        res.set_content(buffer.str(), "application/octet-stream");
    }
    
    void HandleFileInfo(const httplib::Request& req, httplib::Response& res, int routeId)
    {
        UploadRoute route;
        {
            std::lock_guard<std::mutex> lock(routesMutex);
            auto it = routes.find(routeId);
            if (it == routes.end()) {
                res.status = 404;
                res.set_content(Json::Obj({ Json::Str("error", "Route not found", false) }), "application/json");
                return;
            }
            route = it->second;
        }
        
        if (!route.allowInfo) {
            res.status = 403;
            res.set_content(Json::Obj({ Json::Str("error", "File info not allowed", false) }), "application/json");
            return;
        }
        
        if (!CheckFileRouteAuth(route, req)) {
            res.status = 401;
            res.set_content(Json::Obj({ Json::Str("error", "Unauthorized", false) }), "application/json");
            return;
        }
        
        // Extract filename from path
        std::string filename;
        std::string pathPrefix = route.endpoint + "/files/";
        std::string pathSuffix = "/info";
        if (req.path.rfind(pathPrefix, 0) == 0) {
            filename = req.path.substr(pathPrefix.size());
            if (filename.size() > pathSuffix.size() && 
                filename.substr(filename.size() - pathSuffix.size()) == pathSuffix) {
                filename = filename.substr(0, filename.size() - pathSuffix.size());
            }
        }
        
        std::string safeName = SanitizeFilename(filename);
        if (safeName.empty()) {
            res.status = 400;
            res.set_content(Json::Obj({ Json::Str("error", "Invalid filename", false) }), "application/json");
            return;
        }
        
        std::string fullPath = serverRootPath + route.destinationPath + safeName;
        if (!FileUtils::FileExists(fullPath)) {
            res.status = 404;
            res.set_content(Json::Obj({ Json::Str("error", "File not found", false) }), "application/json");
            return;
        }
        
        size_t fileSize = FileUtils::FileSize(fullPath);
        std::int64_t modTime = FileUtils::GetFileModificationTime(fullPath);
        uint32_t crc = CRC32::fileChecksum(fullPath);
        
        res.set_content(Json::Obj({
            Json::Bool("success", true),
            Json::Str("filename", safeName),
            Json::Num("size", static_cast<long long>(fileSize)),
            Json::Num("modified", modTime),
            Json::Str("crc32", CRC32::toHex(crc), false)
        }), "application/json");
    }
    
    void HandleFileDelete(const httplib::Request& req, httplib::Response& res, int routeId)
    {
        UploadRoute route;
        {
            std::lock_guard<std::mutex> lock(routesMutex);
            auto it = routes.find(routeId);
            if (it == routes.end()) {
                res.status = 404;
                res.set_content(Json::Obj({ Json::Str("error", "Route not found", false) }), "application/json");
                return;
            }
            route = it->second;
        }
        
        if (!route.allowDelete) {
            res.status = 403;
            res.set_content(Json::Obj({ Json::Str("error", "File deletion not allowed", false) }), "application/json");
            return;
        }
        
        if (!CheckFileRouteAuth(route, req)) {
            res.status = 401;
            res.set_content(Json::Obj({ Json::Str("error", "Unauthorized", false) }), "application/json");
            return;
        }
        
        // Extract filename from path
        std::string filename;
        std::string pathPrefix = route.endpoint + "/files/";
        if (req.path.rfind(pathPrefix, 0) == 0) {
            filename = req.path.substr(pathPrefix.size());
        }
        
        std::string safeName = SanitizeFilename(filename);
        if (safeName.empty()) {
            res.status = 400;
            res.set_content(Json::Obj({ Json::Str("error", "Invalid filename", false) }), "application/json");
            return;
        }
        
        std::string fullPath = serverRootPath + route.destinationPath + safeName;
        if (!FileUtils::FileExists(fullPath)) {
            res.status = 404;
            res.set_content(Json::Obj({ Json::Str("error", "File not found", false) }), "application/json");
            return;
        }
        
        if (FileUtils::RemoveFile(fullPath)) {
            res.set_content(Json::Obj({
                Json::Bool("success", true),
                Json::Str("message", "File deleted"),
                Json::Str("filename", safeName, false)
            }), "application/json");
        } else {
            res.status = 500;
            res.set_content(Json::Obj({ Json::Str("error", "Failed to delete file", false) }), "application/json");
        }
    }

    bool StartHttpServer(int port)
    {
        if (isRunning) {
            if (core) core->printLn("[FileGate] ERROR: Server already running on port %d", currentPort);
            return false;
        }

        httpServer = std::make_unique<httplib::Server>();

        httpServer->set_error_handler([](const httplib::Request&, httplib::Response& res) {
            if (res.status == 404) {
                res.set_content(Json::Obj({
                    Json::Str("error", "endpoint not found", false)
                }), "application/json");
            }
        });

        {
            std::unique_lock<std::mutex> lock(routesMutex);
            for (const auto& kv : routes) {
                // FIX: avoid structured binding capture bug in C++17
                // copy the id and endpoint to local variables before capturing
                int capturedId = kv.first;
                std::string capturedEndpoint = kv.second.endpoint;

                httpServer->Post(capturedEndpoint.c_str(),
                    [this, capturedId](
                        const httplib::Request& req,
                        httplib::Response& res,
                        const httplib::ContentReader& content_reader)
                    {
                        HandleUploadStreaming(req, res, content_reader, capturedId);
                    }
                );
            }
        }

        // Setup REST API handlers
        SetupApiHandlers();

        if (!httpServer->bind_to_port("0.0.0.0", port)) {
            if (core) {
                core->printLn(" ");
                core->printLn("  ===============================================================");
                core->printLn("  [FILEGATE ERROR] GAGAL MENJALANKAN HTTP SERVER PADA PORT %d!", port);
                core->printLn("  >>> Port saat ini sedang dipakai oleh program lain.");
                core->printLn("  >>> Silakan gunakan port lain.");
                core->printLn("  ===============================================================");
                core->printLn(" ");
            }
            httpServer.reset();
            return false;
        }

        currentPort = port;
        isRunning = true;

        httpThread = std::thread([this]() {
            httpServer->listen_after_bind();
        });

        if (core) {
            core->printLn("  [FileGate] HTTP server started on port %d", port);
        }

        return true;
    }

    void StopHttpServer()
    {
        if (httpServer) {
            httpServer->stop();
        }
        if (httpThread.joinable()) {
            httpThread.join();
        }
        if (httpServer) {
            httpServer.reset();
        }
        isRunning = false;
        currentPort = 0;
    }

public:
    PROVIDE_UID(0xF12A3B4C5D6E7F80);

    ~FileGateComponent()
    {
        StopHttpServer();

        uploadWorkerRunning = false;
        if (uploadWorkerThread.joinable()) {
            uploadWorkerThread.join();
        }

        shutdownFlag = true;
        if (cleanupThread.joinable()) {
            cleanupThread.join();
        }

        if (pawn)
            pawn->getEventDispatcher().removeEventHandler(this);

        if (drainTimer) {
            drainTimer->kill();
            drainTimer = nullptr;
        }

        g_Component = nullptr;
    }

    StringView componentName() const override
    {
        return "open.mp http-file-transfer";
    }

    SemanticVersion componentVersion() const override
    {
        return SemanticVersion(2, 0, 0, 0);
    }

    void onLoad(ICore* c) override
    {
        core = c;
        g_Component = this;

        serverRootPath = FileUtils::GetCurrentWorkingDirectory();
        FileUtils::CreateDirectory(serverRootPath + Config::TEMP_DIR);

        core->printLn(" ");
        core->printLn("  open.mp http-file-transfer v2 loaded!");
        core->printLn("  Features   : RECEIVE (HTTP Server) + UPLOAD (HTTP Client)");
        core->printLn("  Modes      : RAW + MULTIPART");
        core->printLn("  Transport  : HTTP ONLY");
        core->printLn("  Temp Dir   : %s", Config::TEMP_DIR);
        core->printLn("  Root Path  : %s", serverRootPath.c_str());
        core->printLn(" ");

        setAmxLookups(core);
    }

    void onInit(IComponentList* components) override
    {
        pawn   = components->queryComponent<IPawnComponent>();
        timers = components->queryComponent<ITimersComponent>();

        if (pawn) {
            setAmxFunctions(pawn->getAmxFunctions());
            setAmxLookups(components);
            pawn->getEventDispatcher().addEventHandler(this);
        }

        if (timers) {
            int intervalMs = 1000 / Config::DRAIN_FPS;
            drainTimer = timers->create(
                new DrainTimerHandler(this),
                std::chrono::milliseconds(intervalMs),
                true
            );
        }

        uploadWorkerRunning = true;
        uploadWorkerThread = std::thread([this]() {
            UploadWorker();
        });

        cleanupThread = std::thread([this]() {
            while (!shutdownFlag) {
                for (int i = 0; i < 60 && !shutdownFlag; ++i) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                if (shutdownFlag) break;

                try {
                    FileUtils::CleanupTempFiles(
                        serverRootPath + Config::TEMP_DIR,
                        static_cast<int>(Config::TEMP_CLEANUP_SEC)
                    );
                } catch (...) {}
            }
        });
    }

    void onReady() override {}

    void onFree(IComponent* component) override
    {
        if (component == pawn) {
            pawn = nullptr;
            setAmxFunctions();
            setAmxLookups();
        }
        else if (component == timers) {
            timers   = nullptr;
            drainTimer = nullptr;
        }
    }

    void free() override { delete this; }

    void reset() override
    {
        std::unique_lock<std::mutex> lock(routesMutex);
        routes.clear();
        lock.unlock();

        std::lock_guard<std::mutex> lock2(eventMutex);
        pendingEvents.clear();

        std::lock_guard<std::mutex> lock3(outgoingMutex);
        outgoingUploads.clear();
    }

    void onAmxLoad(IPawnScript& script) override
    {
        pawn_natives::AmxLoad(script.GetAMX());
    }

    void onAmxUnload(IPawnScript&) override {}

    // Public API
    bool Start(int port) { return StartHttpServer(port); }
    bool Stop() {
        if (!isRunning) return false;
        StopHttpServer();
        if (core) core->printLn("  [FileGate] HTTP server stopped");
        return true;
    }
    bool IsRunning() const { return isRunning; }
    int GetPort() const { return currentPort; }

    // ═══════════════════════════════════════════════════════════════════════
    // RECEIVE API
    // ═══════════════════════════════════════════════════════════════════════
    int RegisterRoute(
        const std::string& endpoint,
        const std::string& path,
        const std::string& allowedExts,
        int maxSizeMb)
    {
        if (endpoint.empty() || path.empty()) return -1;
        if (endpoint[0] != '/') return -1;

        std::string safePath = SanitizeRelativeDir(path);
        if (safePath.empty()) return -1;

        UploadRoute route;
        route.routeId         = nextRouteId++;
        route.endpoint        = endpoint;
        route.destinationPath = safePath;
        route.allowedExtensions = SplitCSV(allowedExts);
        route.maxSizeBytes      = static_cast<size_t>(std::max(maxSizeMb, 1)) * 1024 * 1024;
        route.requireCrc32      = false;

        FileUtils::CreateDirectory(serverRootPath + route.destinationPath);

        int id = route.routeId;
        std::string capturedEndpoint = route.endpoint;

        std::unique_lock<std::mutex> lock(routesMutex);
        routes[id] = std::move(route);
        lock.unlock();

        if (isRunning && httpServer) {
            // FIX: capture id and endpoint as local copies, not from structured binding
            int capturedId = id;
            httpServer->Post(capturedEndpoint.c_str(),
                [this, capturedId](
                    const httplib::Request& req,
                    httplib::Response& res,
                    const httplib::ContentReader& content_reader)
                {
                    HandleUploadStreaming(req, res, content_reader, capturedId);
                }
            );
        }

        if (core) {
            core->printLn("  [FileGate] Route registered: POST %s -> %s",
                capturedEndpoint.c_str(), safePath.c_str());
        }

        return id;
    }

    bool AddKeyToRoute(int routeId, const std::string& key)
    {
        std::unique_lock<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        it->second.authorizedKeys.insert(key);
        return true;
    }

    bool RemoveKeyFromRoute(int routeId, const std::string& key)
    {
        std::unique_lock<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        it->second.authorizedKeys.erase(key);
        return true;
    }

    bool SetConflictMode(int routeId, int mode)
    {
        std::unique_lock<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;

        switch (mode) {
            case 0: it->second.onConflict = ConflictMode::Rename;    break;
            case 1: it->second.onConflict = ConflictMode::Overwrite; break;
            case 2: it->second.onConflict = ConflictMode::Reject;    break;
            default: return false;
        }
        return true;
    }

    bool SetCorruptAction(int routeId, int action)
    {
        std::unique_lock<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;

        switch (action) {
            case 0: it->second.onCorrupt = CorruptAction::Delete;     break;
            case 1: it->second.onCorrupt = CorruptAction::Quarantine; break;
            case 2: it->second.onCorrupt = CorruptAction::Keep;       break;
            default: return false;
        }
        return true;
    }

    bool SetRequireCRC32(int routeId, bool required)
    {
        std::unique_lock<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        it->second.requireCrc32 = required;
        return true;
    }

    bool RemoveRoute(int routeId)
    {
        std::unique_lock<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        routes.erase(it);
        return true;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // REST API
    // ═══════════════════════════════════════════════════════════════════════
    
    int RegisterApiRoute(int method, const std::string& endpoint, const std::string& callback)
    {
        if (endpoint.empty() || callback.empty()) return -1;
        if (endpoint[0] != '/') return -1;
        if (method < 0 || method > 4) return -1;
        
        APIRoute route;
        route.routeId = nextApiRouteId++;
        route.method = static_cast<HttpMethod>(method);
        route.endpoint = endpoint;
        route.callbackName = callback;
        route.requireAuth = false;
        
        auto compiled = CompileEndpointPattern(endpoint);
        route.pattern = compiled.first;
        route.paramNames = compiled.second;
        
        int id = route.routeId;
        
        {
            std::lock_guard<std::mutex> lock(apiRoutesMutex);
            apiRoutes[id] = std::move(route);
        }
        
        // If server is already running, we need to add the handler
        if (isRunning && httpServer) {
            std::string httpPattern = endpoint;
            size_t pos = 0;
            while ((pos = httpPattern.find('{', pos)) != std::string::npos) {
                size_t end = httpPattern.find('}', pos);
                if (end != std::string::npos) {
                    httpPattern.replace(pos, end - pos + 1, "([^/]+)");
                } else {
                    break;
                }
            }
            
            switch (static_cast<HttpMethod>(method)) {
                case HttpMethod::GET:
                    httpServer->Get(httpPattern.c_str(), [this](const httplib::Request& req, httplib::Response& res) {
                        HandleApiRequest(HttpMethod::GET, req, res);
                    });
                    break;
                case HttpMethod::POST:
                    httpServer->Post(httpPattern.c_str(), [this](const httplib::Request& req, httplib::Response& res) {
                        HandleApiRequest(HttpMethod::POST, req, res);
                    });
                    break;
                case HttpMethod::PUT:
                    httpServer->Put(httpPattern.c_str(), [this](const httplib::Request& req, httplib::Response& res) {
                        HandleApiRequest(HttpMethod::PUT, req, res);
                    });
                    break;
                case HttpMethod::PATCH:
                    httpServer->Patch(httpPattern.c_str(), [this](const httplib::Request& req, httplib::Response& res) {
                        HandleApiRequest(HttpMethod::PATCH, req, res);
                    });
                    break;
                case HttpMethod::DELETE_:
                    httpServer->Delete(httpPattern.c_str(), [this](const httplib::Request& req, httplib::Response& res) {
                        HandleApiRequest(HttpMethod::DELETE_, req, res);
                    });
                    break;
            }
        }
        
        const char* methodNames[] = { "GET", "POST", "PUT", "PATCH", "DELETE" };
        if (core) {
            core->printLn("  [FileGate] API route registered: %s %s -> %s()",
                methodNames[method], endpoint.c_str(), callback.c_str());
        }
        
        return id;
    }
    
    bool RemoveApiRoute(int routeId)
    {
        std::lock_guard<std::mutex> lock(apiRoutesMutex);
        auto it = apiRoutes.find(routeId);
        if (it == apiRoutes.end()) return false;
        apiRoutes.erase(it);
        return true;
    }
    
    bool SetApiRouteAuth(int routeId, const std::string& key)
    {
        std::lock_guard<std::mutex> lock(apiRoutesMutex);
        auto it = apiRoutes.find(routeId);
        if (it == apiRoutes.end()) return false;
        it->second.authKeys.insert(key);
        it->second.requireAuth = true;
        return true;
    }
    
    // File route permissions
    bool SetAllowList(int routeId, bool allow)
    {
        std::lock_guard<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        it->second.allowList = allow;
        return true;
    }
    
    bool SetAllowDownload(int routeId, bool allow)
    {
        std::lock_guard<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        it->second.allowDownload = allow;
        return true;
    }
    
    bool SetAllowDelete(int routeId, bool allow)
    {
        std::lock_guard<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        it->second.allowDelete = allow;
        return true;
    }
    
    bool SetAllowInfo(int routeId, bool allow)
    {
        std::lock_guard<std::mutex> lock(routesMutex);
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        it->second.allowInfo = allow;
        return true;
    }
    
    // File operations
    int GetRouteFileCount(int routeId)
    {
        std::string destPath;
        {
            std::lock_guard<std::mutex> lock(routesMutex);
            auto it = routes.find(routeId);
            if (it == routes.end()) return -1;
            destPath = serverRootPath + it->second.destinationPath;
        }
        
        int count = 0;
        #ifdef _WIN32
            std::string searchPath = destPath + "*";
            WIN32_FIND_DATAA findData;
            HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        ++count;
                    }
                } while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
        #else
            DIR* dir = opendir(destPath.c_str());
            if (dir) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    if (entry->d_type == DT_REG) {
                        ++count;
                    }
                }
                closedir(dir);
            }
        #endif
        return count;
    }
    
    std::string GetRouteFileName(int routeId, int index)
    {
        std::string destPath;
        {
            std::lock_guard<std::mutex> lock(routesMutex);
            auto it = routes.find(routeId);
            if (it == routes.end()) return "";
            destPath = serverRootPath + it->second.destinationPath;
        }
        
        int count = 0;
        std::string result;
        
        #ifdef _WIN32
            std::string searchPath = destPath + "*";
            WIN32_FIND_DATAA findData;
            HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        if (count == index) {
                            result = findData.cFileName;
                            break;
                        }
                        ++count;
                    }
                } while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
        #else
            DIR* dir = opendir(destPath.c_str());
            if (dir) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    if (entry->d_type == DT_REG) {
                        if (count == index) {
                            result = entry->d_name;
                            break;
                        }
                        ++count;
                    }
                }
                closedir(dir);
            }
        #endif
        return result;
    }
    
    bool DeleteRouteFile(int routeId, const std::string& filename)
    {
        std::string destPath;
        {
            std::lock_guard<std::mutex> lock(routesMutex);
            auto it = routes.find(routeId);
            if (it == routes.end()) return false;
            destPath = serverRootPath + it->second.destinationPath;
        }
        
        std::string safeName = SanitizeFilename(filename);
        if (safeName.empty()) return false;
        
        std::string fullPath = destPath + safeName;
        return FileUtils::RemoveFile(fullPath);
    }
    
    size_t GetRouteFileSize(int routeId, const std::string& filename)
    {
        std::string destPath;
        {
            std::lock_guard<std::mutex> lock(routesMutex);
            auto it = routes.find(routeId);
            if (it == routes.end()) return 0;
            destPath = serverRootPath + it->second.destinationPath;
        }
        
        std::string safeName = SanitizeFilename(filename);
        if (safeName.empty()) return 0;
        
        std::string fullPath = destPath + safeName;
        return FileUtils::FileSize(fullPath);
    }
    
    // Request data access
    std::shared_ptr<RequestContext> GetRequest(int requestId)
    {
        std::lock_guard<std::mutex> lock(requestsMutex);
        auto it = activeRequests.find(requestId);
        if (it == activeRequests.end()) return nullptr;
        return it->second;
    }
    
    std::string GetRequestIP(int requestId)
    {
        auto ctx = GetRequest(requestId);
        return ctx ? ctx->clientIP : "";
    }
    
    int GetRequestMethod(int requestId)
    {
        auto ctx = GetRequest(requestId);
        return ctx ? static_cast<int>(ctx->method) : -1;
    }
    
    std::string GetRequestPath(int requestId)
    {
        auto ctx = GetRequest(requestId);
        return ctx ? ctx->path : "";
    }
    
    std::string GetRequestBody(int requestId)
    {
        auto ctx = GetRequest(requestId);
        return ctx ? ctx->body : "";
    }
    
    int GetRequestBodyLength(int requestId)
    {
        auto ctx = GetRequest(requestId);
        return ctx ? static_cast<int>(ctx->body.size()) : 0;
    }
    
    std::string GetParam(int requestId, const std::string& name)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return "";
        auto it = ctx->params.find(name);
        return (it != ctx->params.end()) ? it->second : "";
    }
    
    int GetParamInt(int requestId, const std::string& name)
    {
        std::string val = GetParam(requestId, name);
        return val.empty() ? 0 : std::atoi(val.c_str());
    }
    
    std::string GetQuery(int requestId, const std::string& name)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return "";
        auto it = ctx->queries.find(name);
        return (it != ctx->queries.end()) ? it->second : "";
    }
    
    int GetQueryInt(int requestId, const std::string& name, int defaultVal)
    {
        std::string val = GetQuery(requestId, name);
        return val.empty() ? defaultVal : std::atoi(val.c_str());
    }
    
    std::string GetHeader(int requestId, const std::string& name)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return "";
        auto it = ctx->headers.find(name);
        return (it != ctx->headers.end()) ? it->second : "";
    }
    
    // JSON parsing from request body
    std::string JsonGetString(int requestId, const std::string& key, const std::string& def)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return def;
        return Json::GetString(ctx->body, key, def);
    }
    
    int JsonGetInt(int requestId, const std::string& key, int def)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return def;
        return Json::GetInt(ctx->body, key, def);
    }
    
    double JsonGetFloat(int requestId, const std::string& key, double def)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return def;
        return Json::GetFloat(ctx->body, key, def);
    }
    
    bool JsonGetBool(int requestId, const std::string& key, bool def)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return def;
        return Json::GetBool(ctx->body, key, def);
    }
    
    bool JsonHasKey(int requestId, const std::string& key)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return false;
        return Json::HasKey(ctx->body, key);
    }
    
    int JsonArrayLength(int requestId, const std::string& key)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return -1;
        return Json::ArrayLength(ctx->body, key);
    }
    
    std::string JsonGetNested(int requestId, const std::string& path, const std::string& def)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return def;
        return Json::GetNestedString(ctx->body, path, def);
    }
    
    int JsonGetNestedInt(int requestId, const std::string& path, int def)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx) return def;
        return Json::GetNestedInt(ctx->body, path, def);
    }
    
    // Response methods
    bool Respond(int requestId, int status, const std::string& body, const std::string& contentType)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || ctx->responded || !ctx->httpRes) return false;
        
        for (const auto& kv : ctx->responseHeaders) {
            ctx->httpRes->set_header(kv.first.c_str(), kv.second.c_str());
        }
        
        ctx->httpRes->status = status;
        ctx->httpRes->set_content(body, contentType.c_str());
        ctx->responded = true;
        return true;
    }
    
    bool RespondJSON(int requestId, int status, const std::string& json)
    {
        return Respond(requestId, status, json, "application/json");
    }
    
    bool RespondError(int requestId, int status, const std::string& message)
    {
        return RespondJSON(requestId, status, Json::Obj({
            Json::Bool("success", false),
            Json::Str("error", message, false)
        }));
    }
    
    bool SetResponseHeader(int requestId, const std::string& name, const std::string& value)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || ctx->responded) return false;
        ctx->responseHeaders[name] = value;
        return true;
    }
    
    // JSON builder for responses
    bool JsonStart(int requestId)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || ctx->jsonStarted) return false;
        ctx->responseJson = "{";
        ctx->jsonStarted = true;
        ctx->jsonStack.clear();
        ctx->jsonStack.push_back("object");
        return true;
    }
    
    bool JsonAddString(int requestId, const std::string& key, const std::string& value)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted) return false;
        if (ctx->responseJson.back() != '{' && ctx->responseJson.back() != '[' && ctx->responseJson.back() != ',')
            ctx->responseJson += ",";
        if (!ctx->jsonStack.empty() && ctx->jsonStack.back() == "object" && !key.empty()) {
            ctx->responseJson += "\"" + Json::Escape(key) + "\":";
        }
        ctx->responseJson += "\"" + Json::Escape(value) + "\"";
        return true;
    }
    
    bool JsonAddInt(int requestId, const std::string& key, int value)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted) return false;
        if (ctx->responseJson.back() != '{' && ctx->responseJson.back() != '[' && ctx->responseJson.back() != ',')
            ctx->responseJson += ",";
        if (!ctx->jsonStack.empty() && ctx->jsonStack.back() == "object" && !key.empty()) {
            ctx->responseJson += "\"" + Json::Escape(key) + "\":";
        }
        ctx->responseJson += std::to_string(value);
        return true;
    }
    
    bool JsonAddFloat(int requestId, const std::string& key, double value)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted) return false;
        if (ctx->responseJson.back() != '{' && ctx->responseJson.back() != '[' && ctx->responseJson.back() != ',')
            ctx->responseJson += ",";
        if (!ctx->jsonStack.empty() && ctx->jsonStack.back() == "object" && !key.empty()) {
            ctx->responseJson += "\"" + Json::Escape(key) + "\":";
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6f", value);
        ctx->responseJson += buf;
        return true;
    }
    
    bool JsonAddBool(int requestId, const std::string& key, bool value)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted) return false;
        if (ctx->responseJson.back() != '{' && ctx->responseJson.back() != '[' && ctx->responseJson.back() != ',')
            ctx->responseJson += ",";
        if (!ctx->jsonStack.empty() && ctx->jsonStack.back() == "object" && !key.empty()) {
            ctx->responseJson += "\"" + Json::Escape(key) + "\":";
        }
        ctx->responseJson += value ? "true" : "false";
        return true;
    }
    
    bool JsonAddNull(int requestId, const std::string& key)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted) return false;
        if (ctx->responseJson.back() != '{' && ctx->responseJson.back() != '[' && ctx->responseJson.back() != ',')
            ctx->responseJson += ",";
        if (!ctx->jsonStack.empty() && ctx->jsonStack.back() == "object" && !key.empty()) {
            ctx->responseJson += "\"" + Json::Escape(key) + "\":";
        }
        ctx->responseJson += "null";
        return true;
    }
    
    bool JsonStartObject(int requestId, const std::string& key)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted) return false;
        if (ctx->responseJson.back() != '{' && ctx->responseJson.back() != '[' && ctx->responseJson.back() != ',')
            ctx->responseJson += ",";
        if (!ctx->jsonStack.empty() && ctx->jsonStack.back() == "object" && !key.empty()) {
            ctx->responseJson += "\"" + Json::Escape(key) + "\":";
        }
        ctx->responseJson += "{";
        ctx->jsonStack.push_back("object");
        return true;
    }
    
    bool JsonEndObject(int requestId)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted || ctx->jsonStack.empty()) return false;
        if (ctx->jsonStack.back() != "object") return false;
        ctx->jsonStack.pop_back();
        ctx->responseJson += "}";
        return true;
    }
    
    bool JsonStartArray(int requestId, const std::string& key)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted) return false;
        if (ctx->responseJson.back() != '{' && ctx->responseJson.back() != '[' && ctx->responseJson.back() != ',')
            ctx->responseJson += ",";
        if (!ctx->jsonStack.empty() && ctx->jsonStack.back() == "object" && !key.empty()) {
            ctx->responseJson += "\"" + Json::Escape(key) + "\":";
        }
        ctx->responseJson += "[";
        ctx->jsonStack.push_back("array");
        return true;
    }
    
    bool JsonEndArray(int requestId)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted || ctx->jsonStack.empty()) return false;
        if (ctx->jsonStack.back() != "array") return false;
        ctx->jsonStack.pop_back();
        ctx->responseJson += "]";
        return true;
    }
    
    bool JsonSend(int requestId, int status)
    {
        auto ctx = GetRequest(requestId);
        if (!ctx || !ctx->jsonStarted) return false;
        
        // Close all open structures
        while (!ctx->jsonStack.empty()) {
            if (ctx->jsonStack.back() == "object") {
                ctx->responseJson += "}";
            } else {
                ctx->responseJson += "]";
            }
            ctx->jsonStack.pop_back();
        }
        
        return RespondJSON(requestId, status, ctx->responseJson);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // UPLOAD API (Client)
    // ═══════════════════════════════════════════════════════════════════════
    int QueueUpload(
        const std::string& url,
        const std::string& filepath,
        const std::string& filename,
        const std::string& authKey,
        const std::string& customHeaders,
        bool calculateCrc32,
        int mode)
    {
        if (url.empty() || filepath.empty()) return -1;

        std::string safeFile = NormalizeSlashes(filepath);
        if (safeFile.find("..") != std::string::npos) return -1;
        if (!safeFile.empty() && safeFile[0] == '/') return -1;

        std::string fullPath = serverRootPath + safeFile;
        if (!FileUtils::FileExists(fullPath)) {
            if (core) core->printLn("[FileGate] Upload failed: file not found %s", fullPath.c_str());
            return -1;
        }

        OutgoingUpload upload;
        upload.uploadId = nextOutgoingId++;
        upload.url = url;
        upload.filepath = safeFile;
        upload.filename = filename.empty() ? SanitizeFilename(filepath) : SanitizeFilename(filename);
        upload.authKey = authKey;
        upload.customHeader = customHeaders;
        upload.calculateCrc32 = calculateCrc32;
        upload.status = UploadStatus::Pending;
        upload.totalBytes = FileUtils::FileSize(fullPath);
        upload.mode = (mode == 1) ? UploadMode::Raw : UploadMode::Multipart;

        int id = upload.uploadId;

        {
            std::lock_guard<std::mutex> lock(outgoingMutex);
            outgoingUploads[id] = std::move(upload);
        }

        if (core) {
            core->printLn("[FileGate] Upload queued #%d: %s -> %s (%s)",
                id, filepath.c_str(), url.c_str(),
                (mode == 1 ? "RAW" : "MULTIPART"));
        }

        return id;
    }

    bool CancelUpload(int uploadId)
    {
        std::lock_guard<std::mutex> lock(outgoingMutex);
        auto it = outgoingUploads.find(uploadId);
        if (it == outgoingUploads.end()) return false;

        if (it->second.status == UploadStatus::Completed ||
            it->second.status == UploadStatus::Failed ||
            it->second.status == UploadStatus::Cancelled) {
            return false;
        }

        it->second.status = UploadStatus::Cancelled;
        if (it->second.cancelToken) {
            it->second.cancelToken->store(true);
        }
        return true;
    }

    int GetUploadStatus(int uploadId)
    {
        std::lock_guard<std::mutex> lock(outgoingMutex);
        auto it = outgoingUploads.find(uploadId);
        if (it == outgoingUploads.end()) return -1;
        return static_cast<int>(it->second.status);
    }

    int GetUploadProgress(int uploadId)
    {
        std::lock_guard<std::mutex> lock(outgoingMutex);
        auto it = outgoingUploads.find(uploadId);
        if (it == outgoingUploads.end()) return -1;
        return it->second.progressPct;
    }

    bool GetUploadResponse(int uploadId, std::string& output, int maxLen)
    {
        std::lock_guard<std::mutex> lock(outgoingMutex);
        auto it = outgoingUploads.find(uploadId);
        if (it == outgoingUploads.end()) {
            output = "";
            return false;
        }

        const std::string& resp = it->second.responseBody;
        if (maxLen > 0 && resp.length() > static_cast<size_t>(maxLen - 1)) {
            output = resp.substr(0, maxLen - 1);
        } else {
            output = resp;
        }
        return it->second.status == UploadStatus::Completed;
    }
};

COMPONENT_ENTRY_POINT()
{
    return new FileGateComponent();
}

// ═══════════════════════════════════════════════════════════════════════════
// PAWN NATIVES
// ═══════════════════════════════════════════════════════════════════════════

// Server Control
SCRIPT_API(FileGate_Start, bool(int port))
{
    auto c = GetComponent();
    if (!c) return false;
    if (port <= 0 || port > 65535) return false;
    return c->Start(port);
}

SCRIPT_API(FileGate_Stop, bool())
{
    auto c = GetComponent();
    if (!c) return false;
    return c->Stop();
}

SCRIPT_API(FileGate_IsRunning, int())
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->IsRunning() ? 1 : 0;
}

SCRIPT_API(FileGate_GetPort, int())
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetPort();
}

// Receive Routes
SCRIPT_API(FileGate_RegisterRoute,
    int(const std::string& endpoint, const std::string& path,
        const std::string& allowedExts, int maxSizeMb))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->RegisterRoute(endpoint, path, allowedExts, maxSizeMb);
}

SCRIPT_API(FileGate_AddKey, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->AddKeyToRoute(routeId, key);
}

SCRIPT_API(FileGate_RemoveKey, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveKeyFromRoute(routeId, key);
}

SCRIPT_API(FileGate_SetConflict, bool(int routeId, int mode))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetConflictMode(routeId, mode);
}

SCRIPT_API(FileGate_SetCorruptAction, bool(int routeId, int action))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetCorruptAction(routeId, action);
}

SCRIPT_API(FileGate_SetRequireCRC32, bool(int routeId, bool required))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetRequireCRC32(routeId, required);
}

SCRIPT_API(FileGate_RemoveRoute, bool(int routeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveRoute(routeId);
}

// ═══════════════════════════════════════════════════════════════════════════
// REST API Natives
// ═══════════════════════════════════════════════════════════════════════════

SCRIPT_API(FileGate_Route, int(int method, const std::string& endpoint, const std::string& callback))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->RegisterApiRoute(method, endpoint, callback);
}

SCRIPT_API(FileGate_RemoveAPIRoute, bool(int routeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveApiRoute(routeId);
}

SCRIPT_API(FileGate_SetRouteAuth, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetApiRouteAuth(routeId, key);
}

// Request data access
SCRIPT_API(FileGate_GetRequestIP, int(int requestId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string ip = c->GetRequestIP(requestId);
    output = ip.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return ip.empty() ? 0 : 1;
}

SCRIPT_API(FileGate_GetRequestMethod, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetRequestMethod(requestId);
}

SCRIPT_API(FileGate_GetRequestPath, int(int requestId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string path = c->GetRequestPath(requestId);
    output = path.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return path.empty() ? 0 : 1;
}

SCRIPT_API(FileGate_GetRequestBody, int(int requestId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string body = c->GetRequestBody(requestId);
    output = body.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return static_cast<int>(body.size());
}

SCRIPT_API(FileGate_GetRequestBodyLength, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetRequestBodyLength(requestId);
}

// URL parameters
SCRIPT_API(FileGate_GetParam, int(int requestId, const std::string& name, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->GetParam(requestId, name);
    output = val.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return val.empty() ? 0 : 1;
}

SCRIPT_API(FileGate_GetParamInt, int(int requestId, const std::string& name))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetParamInt(requestId, name);
}

// Query string
SCRIPT_API(FileGate_GetQuery, int(int requestId, const std::string& name, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->GetQuery(requestId, name);
    output = val.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return val.empty() ? 0 : 1;
}

SCRIPT_API(FileGate_GetQueryInt, int(int requestId, const std::string& name, int defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->GetQueryInt(requestId, name, defaultValue);
}

// Headers
SCRIPT_API(FileGate_GetHeader, int(int requestId, const std::string& name, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->GetHeader(requestId, name);
    output = val.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return val.empty() ? 0 : 1;
}

// JSON parsing from request body
SCRIPT_API(FileGate_JsonGetString, int(int requestId, const std::string& key, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->JsonGetString(requestId, key, "");
    output = val.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return val.empty() ? 0 : 1;
}

SCRIPT_API(FileGate_JsonGetInt, int(int requestId, const std::string& key, int defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->JsonGetInt(requestId, key, defaultValue);
}

SCRIPT_API(FileGate_JsonGetFloat, float(int requestId, const std::string& key, float defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return static_cast<float>(c->JsonGetFloat(requestId, key, defaultValue));
}

SCRIPT_API(FileGate_JsonGetBool, bool(int requestId, const std::string& key, bool defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->JsonGetBool(requestId, key, defaultValue);
}

SCRIPT_API(FileGate_JsonHasKey, bool(int requestId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonHasKey(requestId, key);
}

SCRIPT_API(FileGate_JsonArrayLength, int(int requestId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonArrayLength(requestId, key);
}

SCRIPT_API(FileGate_JsonGetNested, int(int requestId, const std::string& path, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->JsonGetNested(requestId, path, "");
    output = val.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return val.empty() ? 0 : 1;
}

SCRIPT_API(FileGate_JsonGetNestedInt, int(int requestId, const std::string& path, int defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->JsonGetNestedInt(requestId, path, defaultValue);
}

// Response methods
SCRIPT_API(FileGate_Respond, bool(int requestId, int status, const std::string& body, const std::string& contentType))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->Respond(requestId, status, body, contentType);
}

SCRIPT_API(FileGate_RespondJSON, bool(int requestId, int status, const std::string& json))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RespondJSON(requestId, status, json);
}

SCRIPT_API(FileGate_RespondError, bool(int requestId, int status, const std::string& message))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RespondError(requestId, status, message);
}

SCRIPT_API(FileGate_SetResponseHeader, bool(int requestId, const std::string& name, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetResponseHeader(requestId, name, value);
}

// JSON builder for responses
SCRIPT_API(FileGate_JsonStart, bool(int requestId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonStart(requestId);
}

SCRIPT_API(FileGate_JsonAddString, bool(int requestId, const std::string& key, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonAddString(requestId, key, value);
}

SCRIPT_API(FileGate_JsonAddInt, bool(int requestId, const std::string& key, int value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonAddInt(requestId, key, value);
}

SCRIPT_API(FileGate_JsonAddFloat, bool(int requestId, const std::string& key, float value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonAddFloat(requestId, key, value);
}

SCRIPT_API(FileGate_JsonAddBool, bool(int requestId, const std::string& key, bool value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonAddBool(requestId, key, value);
}

SCRIPT_API(FileGate_JsonAddNull, bool(int requestId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonAddNull(requestId, key);
}

SCRIPT_API(FileGate_JsonStartObject, bool(int requestId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonStartObject(requestId, key);
}

SCRIPT_API(FileGate_JsonEndObject, bool(int requestId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonEndObject(requestId);
}

SCRIPT_API(FileGate_JsonStartArray, bool(int requestId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonStartArray(requestId, key);
}

SCRIPT_API(FileGate_JsonEndArray, bool(int requestId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonEndArray(requestId);
}

SCRIPT_API(FileGate_JsonSend, bool(int requestId, int status))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonSend(requestId, status);
}

// File Route Permission Natives
SCRIPT_API(FileGate_AllowList, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowList(routeId, allow);
}

SCRIPT_API(FileGate_AllowDownload, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowDownload(routeId, allow);
}

SCRIPT_API(FileGate_AllowDelete, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowDelete(routeId, allow);
}

SCRIPT_API(FileGate_AllowInfo, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowInfo(routeId, allow);
}

// File Operation Natives
SCRIPT_API(FileGate_GetFileCount, int(int routeId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetRouteFileCount(routeId);
}

SCRIPT_API(FileGate_GetFileName, int(int routeId, int index, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string name = c->GetRouteFileName(routeId, index);
    output = name.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return name.empty() ? 0 : 1;
}

SCRIPT_API(FileGate_DeleteFile, bool(int routeId, const std::string& filename))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->DeleteRouteFile(routeId, filename);
}

SCRIPT_API(FileGate_GetFileSize, int(int routeId, const std::string& filename))
{
    auto c = GetComponent();
    if (!c) return 0;
    return static_cast<int>(c->GetRouteFileSize(routeId, filename));
}

// Upload (Client) Natives
// mode: 0 = multipart, 1 = raw
SCRIPT_API(FileGate_UploadFile,
    int(const std::string& url, const std::string& filepath,
        const std::string& filename, const std::string& authKey,
        const std::string& customHeaders, int calculateCrc32, int mode))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->QueueUpload(url, filepath, filename, authKey, customHeaders, calculateCrc32 != 0, mode);
}

SCRIPT_API(FileGate_CancelUpload, bool(int uploadId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->CancelUpload(uploadId);
}

SCRIPT_API(FileGate_GetUploadStatus, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetUploadStatus(uploadId);
}

SCRIPT_API(FileGate_GetUploadProgress, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetUploadProgress(uploadId);
}

SCRIPT_API(FileGate_GetUploadResponse, int(int uploadId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) {
        output = "";
        return 0;
    }
    return c->GetUploadResponse(uploadId, output, outputSize) ? 1 : 0;
}

// CRC32 Utilities
SCRIPT_API(FileGate_VerifyCRC32, int(const std::string& filepath, const std::string& expectedCrc))
{
    auto c = GetComponent();
    if (!c) return -1;

    std::string base = FileUtils::GetCurrentWorkingDirectory();
    std::string fullPath = base + filepath;

    if (!FileUtils::FileExists(fullPath)) return -1;

    uint32_t calculated = CRC32::fileChecksum(fullPath);
    uint32_t expected = CRC32::fromHex(expectedCrc);

    return (calculated == expected) ? 1 : 0;
}

SCRIPT_API(FileGate_GetFileCRC32, int(const std::string& filepath, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) {
        output = "0";
        return 0;
    }

    std::string base = FileUtils::GetCurrentWorkingDirectory();
    std::string fullPath = base + filepath;

    if (!FileUtils::FileExists(fullPath)) {
        output = "0";
        return 0;
    }

    uint32_t crc = CRC32::fileChecksum(fullPath);
    std::string hex = CRC32::toHex(crc);

    if (outputSize > 0) {
        size_t copyLen = std::min(static_cast<size_t>(outputSize - 1), hex.length());
        output.assign(hex.c_str(), copyLen);
    } else {
        output = "";
    }
    return 1;
}

SCRIPT_API(FileGate_CompareFiles, int(const std::string& path1, const std::string& path2))
{
    auto c = GetComponent();
    if (!c) return -1;

    std::string base = FileUtils::GetCurrentWorkingDirectory();
    std::string fullPath1 = base + path1;
    std::string fullPath2 = base + path2;

    if (!FileUtils::FileExists(fullPath1) || !FileUtils::FileExists(fullPath2)) return -1;

    uint32_t crc1 = CRC32::fileChecksum(fullPath1);
    uint32_t crc2 = CRC32::fileChecksum(fullPath2);

    if (crc1 == 0 || crc2 == 0) return -1;
    return (crc1 == crc2) ? 1 : 0;
}