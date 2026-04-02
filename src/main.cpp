// open.mp HTTP File Receiver Component
// repurposed from easing-functions component

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>
#include <Server/Components/Timers/timers.hpp>

// cpp-httplib — header only, letakkan di deps/httplib.h
// https://github.com/yhirose/cpp-httplib
#include <httplib.h>

#include <atomic>
#include <chrono>
#include <filesystem>
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

// ─────────────────────────────────────────────────────────────────────────────
// Config — bisa di-override lewat config.json
// ─────────────────────────────────────────────────────────────────────────────
namespace Config {
    inline int    httpPort        = 8080;   // port HTTP server
    inline int    maxConcurrent   = 16;     // max upload simultan
    inline int    drainRateFps    = 30;     // seberapa sering EventBridge di-drain
    inline size_t tempCleanupSec  = 3600;   // hapus .tmp yang stuck setelah N detik
    inline std::string tempDir    = ".tmp/";
}

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────
class FileReceiverComponent;
static FileReceiverComponent* g_Component = nullptr;
static FileReceiverComponent* GetComponent() { return g_Component; }

// ─────────────────────────────────────────────────────────────────────────────
// ConflictMode — behaviour kalau file sudah ada di destination
// ─────────────────────────────────────────────────────────────────────────────
enum class ConflictMode {
    Rename,     // default — tambah suffix _1, _2, dst
    Overwrite,  // tulis langsung
    Reject      // tolak upload, return 409
};

// ─────────────────────────────────────────────────────────────────────────────
// UploadRoute — satu route yang di-register dari Pawn
// ─────────────────────────────────────────────────────────────────────────────
struct UploadRoute {
    int         routeId = -1;

    std::string endpoint;           // e.g. "/map"
    std::string destinationPath;    // e.g. "scriptfiles/maps/"

    std::vector<std::string>        allowedExtensions;  // e.g. {".map", ".json"}
    std::unordered_set<std::string> authorizedKeys;     // Bearer tokens
    size_t      maxSizeBytes = 10 * 1024 * 1024;        // default 10MB
    ConflictMode onConflict  = ConflictMode::Rename;
};

// ─────────────────────────────────────────────────────────────────────────────
// UploadEvent — dikirim dari HTTP thread ke game thread via EventBridge
// ─────────────────────────────────────────────────────────────────────────────
struct UploadEvent {
    enum class Type { Completed, Failed, Progress };

    Type        type;
    int         uploadId  = -1;
    int         routeId   = -1;
    std::string endpoint;
    std::string filename;
    std::string filepath;   // relative dari server root (untuk Pawn)
    std::string reason;     // kalau Failed
    int         progressPct = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ValidationResult
// ─────────────────────────────────────────────────────────────────────────────
struct ValidationResult {
    bool        ok = false;
    std::string reason;
};

// ─────────────────────────────────────────────────────────────────────────────
// Validasi ditaruh disini
// ─────────────────────────────────────────────────────────────────────────────
// Helper — Sanitize filename (remove paths / ..)
// ─────────────────────────────────────────────────────────────────────────────
static std::string SanitizeFilename(const std::string& input)
{
    return std::filesystem::path(input).filename().string();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper — split "a,b,c" jadi vector
// ─────────────────────────────────────────────────────────────────────────────
static std::vector<std::string> SplitCSV(const std::string& s)
{
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token.erase(0, token.find_first_not_of(" \t"));
        auto last = token.find_last_not_of(" \t");
        if (last != std::string::npos)
            token.erase(last + 1);
        if (!token.empty())
            out.push_back(token);
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// FileReceiverComponent
// ─────────────────────────────────────────────────────────────────────────────
class FileReceiverComponent final
    : public IComponent
    , public PawnEventHandler
    , public PlayerConnectEventHandler
{
private:
    // ── open.mp pointers ────────────────────────────────────────────
    ICore*             core    = nullptr;
    IPawnComponent*    pawn    = nullptr;
    ITimersComponent*  timers  = nullptr;
    ITimer*            drainTimer = nullptr;

    // ── route registry ──────────────────────────────────────────────
    std::unordered_map<int, UploadRoute> routes;
    std::atomic<int> nextRouteId  { 0 };
    std::atomic<int> nextUploadId { 0 };

    // ── HTTP server (jalan di thread terpisah) ───────────────────────
    std::unique_ptr<httplib::Server> httpServer;
    std::thread                      httpThread;
    std::thread                      cleanupThread;
    std::atomic<bool>                shutdownFlag { false };
    std::string                      serverRootPath;

    // ── EventBridge ─────────────────────────────────────────────────
    // HTTP thread push ke sini, game thread drain di setiap tick
    std::mutex               eventMutex;
    std::vector<UploadEvent> pendingEvents;

    // ── Timer handler inner class (sama polanya seperti kode lama) ───
    class DrainTimerHandler : public TimerTimeOutHandler
    {
        FileReceiverComponent* owner;
    public:
        DrainTimerHandler(FileReceiverComponent* o) : owner(o) {}
        void timeout(ITimer&) override { owner->DrainEvents(); }
        void free(ITimer&) override { delete this; }
        ~DrainTimerHandler() = default;
    };

    // ─────────────────────────────────────────────────────────────────
    // Internal helpers
    // ─────────────────────────────────────────────────────────────────

    // generate nama temp file yang unik
    std::string MakeTempPath(int uploadId)
    {
        auto ts = std::chrono::system_clock::now().time_since_epoch().count();
        return Config::tempDir
             + "upload_" + std::to_string(uploadId)
             + "_" + std::to_string(ts) + ".tmp";
    }

    // strip server root → relative path untuk Pawn
    std::string ToRelativePath(const std::string& absPath)
    {
        if (absPath.rfind(serverRootPath, 0) == 0)
            return absPath.substr(serverRootPath.size());
        return absPath;
    }

    // resolve nama file final berdasarkan ConflictMode
    // return "" kalau Reject dan file sudah ada
    std::string ResolveDestPath(
        const std::string& dir,
        const std::string& filename,
        ConflictMode mode)
    {
        std::filesystem::path dest = dir + filename;

        if (!std::filesystem::exists(dest))
            return dest.string();

        switch (mode) {
            case ConflictMode::Overwrite:
                return dest.string();

            case ConflictMode::Reject:
                return "";  // caller interpret "" sebagai 409

            case ConflictMode::Rename:
            default: {
                std::string stem = dest.stem().string();
                std::string ext  = dest.extension().string();

                for (int i = 1; i <= 9999; ++i) {
                    std::filesystem::path c = dir + stem + "_" + std::to_string(i) + ext;
                    if (!std::filesystem::exists(c))
                        return c.string();
                }

                // fallback: pakai timestamp
                auto ts = std::chrono::system_clock::now().time_since_epoch().count();
                return dir + stem + "_" + std::to_string(ts) + ext;
            }
        }
    }

    // validasi extension dan ukuran
    ValidationResult ValidateUpload(
        const std::string& filename,
        size_t fileSize,
        const UploadRoute& route)
    {
        if (!route.allowedExtensions.empty()) {
            std::string ext = std::filesystem::path(filename).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            bool allowed = std::any_of(
                route.allowedExtensions.begin(),
                route.allowedExtensions.end(),
                [&](const std::string& e) { return e == ext; }
            );
            if (!allowed)
                return { false, "extension not allowed: " + ext };
        }

        if (fileSize > route.maxSizeBytes)
            return { false, "file too large (" + std::to_string(fileSize) + " bytes)" };

        return { true, "" };
    }

    // push event dari HTTP thread (thread-safe)
    void PushEvent(UploadEvent ev)
    {
        std::lock_guard<std::mutex> lock(eventMutex);
        pendingEvents.push_back(std::move(ev));
    }

    // ─────────────────────────────────────────────────────────────────
    // HandleUpload — dipanggil oleh httplib per-request
    // ─────────────────────────────────────────────────────────────────
    void HandleUploadStreaming(
        const httplib::Request& req,
        httplib::Response& res,
        const httplib::ContentReader& content_reader,
        int routeId)
    {
        // 1. route lookup
        auto it = routes.find(routeId);
        if (it == routes.end()) {
            res.status = 404;
            res.set_content(R"({"error":"route not found"})", "application/json");
            return;
        }
        const UploadRoute& route = it->second;

        // 2. auth check — Bearer token
        std::string authKey;
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.size() > 7 && authHeader.substr(0, 7) == "Bearer ")
            authKey = authHeader.substr(7);

        if (!route.authorizedKeys.empty() &&
            route.authorizedKeys.find(authKey) == route.authorizedKeys.end())
        {
            res.status = 401;
            res.set_content(R"({"error":"unauthorized"})", "application/json");
            return;
        }

        // 3. parse multipart
        if (!req.is_multipart_form_data()) {
            res.status = 400;
            res.set_content(R"({"error":"expected multipart/form-data"})", "application/json");
            return;
        }

        int uploadId = nextUploadId++;

        std::string final_filename;
        std::string tempPath;
        std::ofstream tempFile;
        bool is_target_file = false;
        size_t written = 0;
        int lastPct = -1;
        bool has_error = false;
        std::string errstr;

        // Try to get content-length for progress event tracking
        size_t expected_size = 0;
        auto clen = req.get_header_value("Content-Length");
        if (!clen.empty()) {
            try { expected_size = std::stoull(clen); } catch(...) {}
        }

        content_reader(
            [&](const httplib::FormData& file) {
                if (file.name == "file") {
                    is_target_file = true;
                    // Sanitize filename to prevent path traversal
                    final_filename = SanitizeFilename(file.filename);

                    if (final_filename.empty()) {
                        is_target_file = false;
                        has_error = true;
                        errstr = "invalid filename";
                        return false; 
                    }

                    // Validate upload early based on extension and predicted size
                    auto validation = ValidateUpload(final_filename, expected_size, route);
                    if (!validation.ok && expected_size > 0) { 
                        is_target_file = false;
                        has_error = true;
                        errstr = validation.reason;
                        return false; 
                    }

                    std::filesystem::create_directories(serverRootPath + Config::tempDir);
                    tempPath = MakeTempPath(uploadId);
                    tempFile.open(serverRootPath + tempPath, std::ios::binary);
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

                tempFile.write(data, data_length);
                if (tempFile.fail()) {
                    has_error = true;
                    errstr = "write error";
                    return false;
                }

                written += data_length;
                if (expected_size > 0) {
                    int pct = static_cast<int>((written * 100) / expected_size);
                    if (pct / 10 != lastPct / 10) {
                        lastPct = pct;
                        PushEvent({ UploadEvent::Type::Progress, uploadId, routeId,
                                    route.endpoint, final_filename, "", "", pct });
                    }
                }
                return true;
            }
        );

        if (tempFile.is_open()) tempFile.close();

        if (has_error) {
            res.status = 400; 
            if (errstr.find("io error") != std::string::npos || errstr.find("write error") != std::string::npos) {
                res.status = 500;
            } else if (errstr.find("too large") != std::string::npos || errstr.find("extension") != std::string::npos) {
                res.status = 422;
            }
            res.set_content(R"({"error":")" + errstr + R"("})", "application/json");

            if (!tempPath.empty()) {
                std::filesystem::remove(serverRootPath + tempPath);
            }
            PushEvent({ UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                        final_filename, "", errstr });
            return;
        }

        if (final_filename.empty()) {
            res.status = 400;
            res.set_content(R"({"error":"no file field found"})", "application/json");
            return;
        }

        // Final validation for file size, since chunk streaming doesn't know exact total size beforehand
        auto validation = ValidateUpload(final_filename, written, route);
        if (!validation.ok) {
            res.status = 422;
            res.set_content(R"({"error":")" + validation.reason + R"("})", "application/json");
            std::filesystem::remove(serverRootPath + tempPath);
            PushEvent({ UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                        final_filename, "", validation.reason });
            return;
        }

        // 6. resolve destination dan rename/move
        std::string finalPath = ResolveDestPath(
            serverRootPath + route.destinationPath,
            final_filename,
            route.onConflict
        );

        if (finalPath.empty()) {
            std::filesystem::remove(serverRootPath + tempPath);
            res.status = 409;
            res.set_content(R"({"error":"file already exists"})", "application/json");
            PushEvent({ UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                        final_filename, "", "conflict: file already exists" });
            return;
        }

        std::error_code ec;
        std::filesystem::rename(serverRootPath + tempPath, finalPath, ec);

        if (ec) {
            std::filesystem::copy_file(
                serverRootPath + tempPath, finalPath,
                std::filesystem::copy_options::overwrite_existing, ec
            );
            std::filesystem::remove(serverRootPath + tempPath);

            if (ec) {
                res.status = 500;
                res.set_content(R"({"error":"failed to move file"})", "application/json");
                PushEvent({ UploadEvent::Type::Failed, uploadId, routeId, route.endpoint,
                            final_filename, "", "move error: " + ec.message() });
                return;
            }
        }

        // 7. selesai

        std::string relativePath = ToRelativePath(finalPath);

        PushEvent({
            UploadEvent::Type::Completed,
            uploadId, routeId,
            route.endpoint,
            final_filename,
            relativePath
        });

        res.status = 200;
        res.set_content(
            R"({"uploadId":)"   + std::to_string(uploadId) +
            R"(,"path":")"      + relativePath + R"("})",
            "application/json"
        );
    }

    // ─────────────────────────────────────────────────────────────────
    // DrainEvents — dipanggil dari game thread via timer tick
    // swap-and-drain: lock sebentar, ambil semua, proses di luar lock
    // sama persis pola pendingCallbacks di kode lama
    // ─────────────────────────────────────────────────────────────────
    void DrainEvents()
    {
        if (!pawn) return;

        std::vector<UploadEvent> local;
        {
            std::lock_guard<std::mutex> lock(eventMutex);
            local.swap(pendingEvents);  // O(1), tidak copy data
        }

        if (local.empty()) return;

        for (const auto& ev : local) {
            switch (ev.type) {
                case UploadEvent::Type::Completed:
                    CallPawnEvent("OnFileUploaded",
                        ev.uploadId,
                        ev.routeId,
                        StringView(ev.endpoint),
                        StringView(ev.filename),
                        StringView(ev.filepath)
                    );
                    break;

                case UploadEvent::Type::Failed:
                    CallPawnEvent("OnFileFailedUpload",
                        ev.uploadId,
                        StringView(ev.reason)
                    );
                    break;

                case UploadEvent::Type::Progress:
                    CallPawnEvent("OnUploadProgress",
                        ev.uploadId,
                        ev.progressPct
                    );
                    break;
            }
        }
    }

    // helper dispatch ke main + side scripts
    // sama persis cara kode lama panggil OnAnimatorFinish
    template<typename... Args>
    void CallPawnEvent(const char* name, Args&&... args)
    {
        if (!pawn) return;

        if (auto script = pawn->mainScript())
            script->Call(name, DefaultReturnValue_False, std::forward<Args>(args)...);

        for (IPawnScript* script : pawn->sideScripts())
            script->Call(name, DefaultReturnValue_False, std::forward<Args>(args)...);
    }

    // ─────────────────────────────────────────────────────────────────
    // StartHttpServer — jalan di dedicated thread
    // ─────────────────────────────────────────────────────────────────
    bool StartHttpServer()
    {
        httpServer = std::make_unique<httplib::Server>();

        // fallback 404 untuk endpoint yang tidak terdaftar
        httpServer->set_error_handler([](const httplib::Request&, httplib::Response& res) {
            if (res.status == 404)
                res.set_content(R"({"error":"endpoint not found"})", "application/json");
        });

        if (!httpServer->bind_to_port("0.0.0.0", Config::httpPort)) {
            if (core) {
                core->printLn(" ");
                core->printLn("  ===============================================================");
                core->printLn("  [FILEGATE ERROR] GAGAL MENJALANKAN HTTP SERVER PADA PORT %d!", Config::httpPort);
                core->printLn("  >>> Port saat ini sedang dipakai oleh program lain (cth: Apache).");
                core->printLn("  >>> Silakan gunakan port lain melalui file config.json.");
                core->printLn("  ===============================================================");
                core->printLn(" ");
            }
            return false;
        }

        httpThread = std::thread([this]() {
            httpServer->listen_after_bind();
        });
        // JANGAN DETACH, karena kalau destructor berjalan dan httpServer.reset(), 
        // thread ini akan mengakses pointer yang sudah terhapus!
        return true;
    }

    // ─────────────────────────────────────────────────────────────────
    // StopHttpServer
    // ─────────────────────────────────────────────────────────────────
    void StopHttpServer()
    {
        if (httpServer) {
            httpServer->stop();
        }
        // Wajib join agar thread berhenti sebelum kita hapus httpServer
        if (httpThread.joinable()) {
            httpThread.join();
        }
        if (httpServer) {
            httpServer.reset();
        }
    }

public:
    PROVIDE_UID(0xF12A3B4C5D6E7F80);  // UUID baru, beda dari easing component

    ~FileReceiverComponent()
    {
        StopHttpServer();
        
        shutdownFlag = true;
        if (cleanupThread.joinable()) {
            cleanupThread.join();
        }

        if (pawn)
            pawn->getEventDispatcher().removeEventHandler(this);

        if (core)
            core->getPlayers().getPlayerConnectDispatcher().removeEventHandler(this);

        if (drainTimer) {
            drainTimer->kill();
            drainTimer = nullptr;
        }

        g_Component = nullptr;
    }

    // ── IComponent interface ─────────────────────────────────────────

    StringView componentName() const override
    {
        return "open.mp http-file-receiver";
    }

    SemanticVersion componentVersion() const override
    {
        return SemanticVersion(1, 0, 0, 0);
    }

    void onLoad(ICore* c) override
    {
        core = c;
        g_Component = this;

        // ambil server root path untuk relative path calculation
        serverRootPath = std::filesystem::current_path().string() + "/";

        // baca config.json (opsional, semua ada defaultnya)
        auto& cfg = core->getConfig();
        if (int* port = cfg.getInt("filereceiver.port"))
            Config::httpPort = *port;
        if (int* fps = cfg.getInt("filereceiver.drain_fps"))
            Config::drainRateFps = *fps;
        if (int* max = cfg.getInt("filereceiver.max_concurrent"))
            Config::maxConcurrent = *max;

        // buat folder temp
        std::filesystem::create_directories(serverRootPath + Config::tempDir);

        core->printLn(" ");
        core->printLn("  open.mp http-file-receiver loaded!");
        core->printLn("  HTTP Port  : %d", Config::httpPort);
        core->printLn("  Drain Rate : %d FPS", Config::drainRateFps);
        core->printLn("  Temp Dir   : %s", Config::tempDir.c_str());
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

        // timer untuk drain event dari HTTP thread ke Pawn
        // sama persis cara kode lama setup animation timer
        if (timers) {
            int intervalMs = 1000 / std::max(Config::drainRateFps, 1);
            drainTimer = timers->create(
                new DrainTimerHandler(this),
                std::chrono::milliseconds(intervalMs),
                true  // repeating
            );
        }

        // register player disconnect handler untuk cleanup upload yang pending
        if (core)
            core->getPlayers().getPlayerConnectDispatcher().addEventHandler(this);

        // mulai HTTP server di background thread
        if (StartHttpServer()) {
            core->printLn("  [FileReceiver] HTTP server listening on port %d", Config::httpPort);
        }

        cleanupThread = std::thread([this]() {
            while (!shutdownFlag) {
                // Sleep iteratively so we don't block immediate shutdown
                for (int i = 0; i < 60 && !shutdownFlag; ++i) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                if (shutdownFlag) break;

                try {
                    auto tempPathStr = serverRootPath + Config::tempDir;
                    if (std::filesystem::exists(tempPathStr)) {
                        for (const auto& entry : std::filesystem::directory_iterator(tempPathStr)) {
                            if (entry.is_regular_file() && entry.path().extension() == ".tmp") {
                                auto ftime = std::filesystem::last_write_time(entry);
                                auto duration = std::filesystem::file_time_type::clock::now() - ftime;
                                auto age_sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
                                
                                if (age_sec > static_cast<long long>(Config::tempCleanupSec)) {
                                    std::error_code ec;
                                    std::filesystem::remove(entry.path(), ec);
                                }
                            }
                        }
                    }
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

    void free() override   { delete this; }

    void reset() override
    {
        // hapus semua route saat gamemode restart
        // httplib tidak support unregister, hapus map saja
        routes.clear();

        // buang event yang pending, tidak relevan lagi
        std::lock_guard<std::mutex> lock(eventMutex);
        pendingEvents.clear();
    }

    // ── PawnEventHandler ─────────────────────────────────────────────

    void onAmxLoad(IPawnScript& script) override
    {
        pawn_natives::AmxLoad(script.GetAMX());
    }

    void onAmxUnload(IPawnScript&) override {}

    // ── PlayerConnectEventHandler ────────────────────────────────────

    void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason) override
    {
        // kalau ada keperluan track upload per-player di masa depan,
        // cleanup-nya dilakukan di sini
        // untuk sekarang, upload tidak di-bind ke player ID
    }

    // ─────────────────────────────────────────────────────────────────
    // Public API — dipanggil dari SCRIPT_API natives
    // ─────────────────────────────────────────────────────────────────

    // register route baru, return routeId atau -1 kalau gagal
    int RegisterRoute(
        const std::string& endpoint,
        const std::string& path,
        const std::string& allowedExts,
        int maxSizeMb)
    {
        if (endpoint.empty() || path.empty()) return -1;

        UploadRoute route;
        route.routeId         = nextRouteId++;
        route.endpoint        = endpoint;
        route.destinationPath = path;

        // normalize trailing slash
        if (route.destinationPath.back() != '/')
            route.destinationPath += '/';

        route.allowedExtensions = SplitCSV(allowedExts);
        route.maxSizeBytes      = static_cast<size_t>(std::max(maxSizeMb, 1)) * 1024 * 1024;

        // buat folder destination
        std::error_code ec;
        std::filesystem::create_directories(serverRootPath + route.destinationPath, ec);
        if (ec) {
            core->printLn("  [FileReceiver] WARNING: cannot create dir '%s': %s",
                route.destinationPath.c_str(), ec.message().c_str());
        }

        int id = route.routeId;
        routes[id] = std::move(route);

        // daftarkan ke httplib — tidak bisa di-unregister nanti
        // kalau route di-remove, kita cukup flag disabled = true
        httpServer->Post(endpoint.c_str(),
            [this, id](const httplib::Request& req, httplib::Response& res, const httplib::ContentReader& content_reader) {
                HandleUploadStreaming(req, res, content_reader, id);
            }
        );

        core->printLn("  [FileReceiver] Route registered: POST %s → %s",
            endpoint.c_str(),
            routes[id].destinationPath.c_str());

        return id;
    }

    bool AddKeyToRoute(int routeId, const std::string& key)
    {
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        it->second.authorizedKeys.insert(key);
        return true;
    }

    bool RemoveKeyFromRoute(int routeId, const std::string& key)
    {
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;
        it->second.authorizedKeys.erase(key);
        return true;
    }

    bool SetConflictMode(int routeId, int mode)
    {
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

    bool RemoveRoute(int routeId)
    {
        auto it = routes.find(routeId);
        if (it == routes.end()) return false;

        // tidak bisa unregister dari httplib — hapus dari map
        routes.erase(it);
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Entry point — wajib ada, balikin instance component
// ─────────────────────────────────────────────────────────────────────────────
COMPONENT_ENTRY_POINT()
{
    return new FileReceiverComponent();
}

// ─────────────────────────────────────────────────────────────────────────────
// Pawn Native constants
// ─────────────────────────────────────────────────────────────────────────────
// ConflictMode values yang bisa dipakai di Pawn script:
// #define CONFLICT_RENAME    0
// #define CONFLICT_OVERWRITE 1
// #define CONFLICT_REJECT    2

// ─────────────────────────────────────────────────────────────────────────────
// SCRIPT_API — natives yang di-expose ke Pawn
// ─────────────────────────────────────────────────────────────────────────────

// FileReceiver_RegisterRoute(const endpoint[], const path[], const exts[], maxSizeMb)
// return: routeId, atau -1 kalau gagal
SCRIPT_API(FileReceiver_RegisterRoute,
    int(const std::string& endpoint, const std::string& path,
        const std::string& allowedExts, int maxSizeMb))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->RegisterRoute(endpoint, path, allowedExts, maxSizeMb);
}

// FileReceiver_AddKey(routeId, const key[])
// attach API key ke route
SCRIPT_API(FileReceiver_AddKey, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->AddKeyToRoute(routeId, key);
}

// FileReceiver_RemoveKey(routeId, const key[])
SCRIPT_API(FileReceiver_RemoveKey, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveKeyFromRoute(routeId, key);
}

// FileReceiver_SetConflict(routeId, mode)
// mode: 0 = RENAME, 1 = OVERWRITE, 2 = REJECT
SCRIPT_API(FileReceiver_SetConflict, bool(int routeId, int mode))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetConflictMode(routeId, mode);
}

// FileReceiver_RemoveRoute(routeId)
SCRIPT_API(FileReceiver_RemoveRoute, bool(int routeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveRoute(routeId);
}

// ─────────────────────────────────────────────────────────────────────────────
// Pawn callback declarations (untuk include file .inc):
//
//   forward OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], const filepath[]);
//   forward OnFileFailedUpload(uploadId, const reason[]);
//   forward OnUploadProgress(uploadId, percent);
//
// Contoh penggunaan di gamemode:
//
//   new gRouteMap, gRouteModel;
//
//   public OnGameModeInit() {
//       gRouteMap   = FileReceiver_RegisterRoute("/map",   "scriptfiles/maps/", ".map,.json", 10);
//       gRouteModel = FileReceiver_RegisterRoute("/model", "models/",           ".dff,.txd",  50);
//
//       FileReceiver_AddKey(gRouteMap,   "secret_map_key");
//       FileReceiver_AddKey(gRouteModel, "secret_model_key");
//
//       FileReceiver_SetConflict(gRouteMap, CONFLICT_RENAME);
//       return 1;
//   }
//
//   public OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], const filepath[]) {
//       printf("[Upload] #%d selesai: %s -> %s", uploadId, filename, filepath);
//       return 1;
//   }
//
//   public OnUploadProgress(uploadId, percent) {
//       printf("[Upload] #%d progress: %d%%", uploadId, percent);
//       return 1;
//   }
//
//   public OnFileFailedUpload(uploadId, const reason[]) {
//       printf("[Upload] #%d gagal: %s", uploadId, reason);
//       return 1;
//   }
// ─────────────────────────────────────────────────────────────────────────────