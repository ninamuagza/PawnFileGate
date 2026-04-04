# FileGate - HTTP File Transfer & REST API Framework

An open.mp server component that provides HTTP file upload/download functionality and a full REST API framework for SA-MP/open.mp servers.

## Features

- **File Upload Server** - Receive files via HTTP POST with validation
- **File Download API** - Serve files via HTTP GET
- **Outgoing Uploads** - Upload files to external servers
- **REST API Framework** - Create custom HTTP endpoints (GET, POST, PUT, PATCH, DELETE)
- **JSON Support** - Parse request bodies and build responses
- **Authorization** - Bearer token authentication per route
- **CRC32 Integrity** - File checksum validation

---

## Installation

1. Download the latest release for your platform (`.dll` for Windows, `.so` for Linux)
2. Place it in your open.mp server `components/` directory
3. Copy `FileGate.inc` to your Pawn compiler includes directory
4. Add `#include <FileGate>` to your script

---

## Quick Start

```pawn
#include <a_samp>
#include <FileGate>

new g_MapRoute = -1;
new g_ApiPlayers = -1;

public OnGameModeInit()
{
    // Start HTTP server on port 8080
    FileGate_Start(8080);
    
    // === FILE UPLOAD ROUTE ===
    g_MapRoute = FileGate_RegisterRoute("/maps", "scriptfiles/maps/", ".map,.json", 50);
    FileGate_AddKey(g_MapRoute, "upload-secret-key");
    
    // Enable REST API for files
    FileGate_AllowList(g_MapRoute, true);
    FileGate_AllowDownload(g_MapRoute, true);
    
    // === CUSTOM REST API ===
    g_ApiPlayers = FileGate_Route(HTTP_GET, "/api/players", "OnGetPlayers");
    FileGate_SetRouteAuth(g_ApiPlayers, "api-secret-key");
    
    return 1;
}

// Handle GET /api/players
public OnGetPlayers(requestId)
{
    FileGate_JsonStart(requestId);
    FileGate_JsonAddInt(requestId, "online", GetPlayerCount());
    FileGate_JsonAddInt(requestId, "max", GetMaxPlayers());
    FileGate_JsonSend(requestId, 200);
    return 1;
}

public OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], 
                      const filepath[], const crc32[], crcMatched)
{
    printf("[FileGate] File uploaded: %s", filename);
    return 1;
}
```

---

## HTTP Methods

| Constant | Method |
|----------|--------|
| `HTTP_GET` | GET |
| `HTTP_POST` | POST |
| `HTTP_PUT` | PUT |
| `HTTP_PATCH` | PATCH |
| `HTTP_DELETE` | DELETE |

---

## Server Control

```pawn
// Start/stop HTTP server
native bool:FileGate_Start(port);
native bool:FileGate_Stop();
native FileGate_IsRunning();
native FileGate_GetPort();
```

---

## File Upload Routes

```pawn
// Register upload endpoint
native FileGate_RegisterRoute(const endpoint[], const path[], const allowedExts[], maxSizeMb);

// Authorization
native bool:FileGate_AddKey(routeId, const key[]);
native bool:FileGate_RemoveKey(routeId, const key[]);

// Settings
native bool:FileGate_SetConflict(routeId, mode);      // CONFLICT_RENAME/OVERWRITE/REJECT
native bool:FileGate_SetCorruptAction(routeId, act);  // CORRUPT_DELETE/QUARANTINE/KEEP
native bool:FileGate_SetRequireCRC32(routeId, bool:required);
native bool:FileGate_RemoveRoute(routeId);

// REST API permissions for file routes
native bool:FileGate_AllowList(routeId, bool:allow);     // GET {route}/files
native bool:FileGate_AllowDownload(routeId, bool:allow); // GET {route}/files/{name}
native bool:FileGate_AllowDelete(routeId, bool:allow);   // DELETE {route}/files/{name}
native bool:FileGate_AllowInfo(routeId, bool:allow);     // GET {route}/files/{name}/info

// File operations
native FileGate_GetFileCount(routeId);
native FileGate_GetFileName(routeId, index, output[], outputSize);
native bool:FileGate_DeleteFile(routeId, const filename[]);
native FileGate_GetFileSize(routeId, const filename[]);
```

---

## Custom REST API Routes

```pawn
// Register custom endpoint
native FileGate_Route(method, const endpoint[], const callback[]);
native bool:FileGate_RemoveAPIRoute(routeId);
native bool:FileGate_SetRouteAuth(routeId, const key[]);

// Examples:
FileGate_Route(HTTP_GET, "/api/server", "OnGetServer");
FileGate_Route(HTTP_POST, "/api/ban", "OnPostBan");
FileGate_Route(HTTP_GET, "/api/player/{id}", "OnGetPlayer");  // URL params
FileGate_Route(HTTP_DELETE, "/api/vehicle/{id}", "OnDeleteVehicle");
```

---

## Request Data Access

```pawn
// Basic request info
native FileGate_GetRequestIP(requestId, output[], outputSize);
native FileGate_GetRequestMethod(requestId);
native FileGate_GetRequestPath(requestId, output[], outputSize);
native FileGate_GetRequestBody(requestId, output[], outputSize);
native FileGate_GetRequestBodyLength(requestId);

// URL parameters ({id} from /api/player/{id})
native FileGate_GetParam(requestId, const name[], output[], outputSize);
native FileGate_GetParamInt(requestId, const name[]);

// Query string (?page=1&limit=10)
native FileGate_GetQuery(requestId, const name[], output[], outputSize);
native FileGate_GetQueryInt(requestId, const name[], defaultValue = 0);

// Headers
native FileGate_GetHeader(requestId, const name[], output[], outputSize);
```

---

## JSON Parsing (Request Body)

```pawn
// Basic types
native FileGate_JsonGetString(requestId, const key[], output[], outputSize);
native FileGate_JsonGetInt(requestId, const key[], defaultValue = 0);
native Float:FileGate_JsonGetFloat(requestId, const key[], Float:defaultValue = 0.0);
native bool:FileGate_JsonGetBool(requestId, const key[], bool:defaultValue = false);
native bool:FileGate_JsonHasKey(requestId, const key[]);
native FileGate_JsonArrayLength(requestId, const key[]);

// Nested values (dot notation: "user.profile.name" or "items[0].id")
native FileGate_JsonGetNested(requestId, const path[], output[], outputSize);
native FileGate_JsonGetNestedInt(requestId, const path[], defaultValue = 0);
```

---

## Response Methods

```pawn
// Direct response
native bool:FileGate_Respond(requestId, status, const body[], const contentType[] = "application/json");
native bool:FileGate_RespondJSON(requestId, status, const json[]);
native bool:FileGate_RespondError(requestId, status, const message[]);
native bool:FileGate_SetResponseHeader(requestId, const name[], const value[]);

// JSON builder
native bool:FileGate_JsonStart(requestId);
native bool:FileGate_JsonAddString(requestId, const key[], const value[]);
native bool:FileGate_JsonAddInt(requestId, const key[], value);
native bool:FileGate_JsonAddFloat(requestId, const key[], Float:value);
native bool:FileGate_JsonAddBool(requestId, const key[], bool:value);
native bool:FileGate_JsonAddNull(requestId, const key[]);
native bool:FileGate_JsonStartObject(requestId, const key[]);
native bool:FileGate_JsonEndObject(requestId);
native bool:FileGate_JsonStartArray(requestId, const key[]);
native bool:FileGate_JsonEndArray(requestId);
native bool:FileGate_JsonSend(requestId, status = 200);
```

---

## Outgoing Uploads

```pawn
// Upload file to external server
native FileGate_UploadFile(
    const url[],
    const filepath[],
    const filename[] = "",
    const authKey[] = "",
    const customHeaders[] = "",
    calculateCrc32 = 1,
    mode = UPLOAD_MODE_MULTIPART
);

native bool:FileGate_CancelUpload(uploadId);
native FileGate_GetUploadStatus(uploadId);
native FileGate_GetUploadProgress(uploadId);
native FileGate_GetUploadResponse(uploadId, output[], outputSize);
```

---

## CRC32 Utilities

```pawn
native FileGate_VerifyCRC32(const filepath[], const expectedCrc[]);
native FileGate_GetFileCRC32(const filepath[], output[], outputSize);
native FileGate_CompareFiles(const path1[], const path2[]);
```

---

## Callbacks

### File Upload (Incoming)
```pawn
forward OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], 
                       const filepath[], const crc32[], crcMatched);
forward OnFileFailedUpload(uploadId, const reason[], const crc32[]);
forward OnUploadProgress(uploadId, percent);
```

### File Upload (Outgoing)
```pawn
forward OnFileUploadStarted(uploadId);
forward OnFileUploadProgress(uploadId, percent);
forward OnFileUploadCompleted(uploadId, httpStatus, const responseBody[], const crc32[]);
forward OnFileUploadFailed(uploadId, const errorMessage[]);
```

---

## Built-in Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /health` | Server health check |
| `GET /stats` | Server statistics |
| `GET {route}/files` | List files (if allowed) |
| `GET {route}/files/{name}` | Download file (if allowed) |
| `GET {route}/files/{name}/info` | File metadata (if allowed) |
| `DELETE {route}/files/{name}` | Delete file (if allowed) |

---

## Client Examples

### JavaScript
```javascript
// GET players
const res = await fetch('http://localhost:8080/api/players', {
    headers: { 'Authorization': 'Bearer api-secret-key' }
});
const data = await res.json();

// POST ban
await fetch('http://localhost:8080/api/ban', {
    method: 'POST',
    headers: {
        'Authorization': 'Bearer api-secret-key',
        'Content-Type': 'application/json'
    },
    body: JSON.stringify({ playerId: 5, reason: 'cheating' })
});

// Upload file
const formData = new FormData();
formData.append('file', fileInput.files[0]);
await fetch('http://localhost:8080/maps', {
    method: 'POST',
    headers: { 'Authorization': 'Bearer upload-secret-key' },
    body: formData
});
```

### curl
```bash
# Health check
curl http://localhost:8080/health

# Get players
curl -H "Authorization: Bearer api-secret-key" http://localhost:8080/api/players

# Upload file
curl -X POST -H "Authorization: Bearer upload-secret-key" \
     -F "file=@mymap.map" http://localhost:8080/maps

# List files
curl -H "Authorization: Bearer upload-secret-key" http://localhost:8080/maps/files

# Download file
curl -H "Authorization: Bearer upload-secret-key" \
     http://localhost:8080/maps/files/mymap.map -o mymap.map
```

---

## Complete Example

```pawn
#include <a_samp>
#include <FileGate>

new g_MapRoute = -1;

public OnGameModeInit()
{
    FileGate_Start(8080);
    
    // File upload route
    g_MapRoute = FileGate_RegisterRoute("/maps", "scriptfiles/maps/", ".map,.json", 50);
    FileGate_AddKey(g_MapRoute, "secret-key");
    FileGate_AllowList(g_MapRoute, true);
    FileGate_AllowDownload(g_MapRoute, true);
    FileGate_AllowInfo(g_MapRoute, true);
    
    // Custom API routes
    FileGate_Route(HTTP_GET, "/api/server", "API_GetServer");
    FileGate_Route(HTTP_GET, "/api/players", "API_GetPlayers");
    FileGate_Route(HTTP_POST, "/api/announce", "API_PostAnnounce");
    FileGate_Route(HTTP_GET, "/api/player/{id}", "API_GetPlayer");
    
    return 1;
}

public API_GetServer(requestId)
{
    FileGate_JsonStart(requestId);
    FileGate_JsonAddString(requestId, "name", "My Server");
    FileGate_JsonAddInt(requestId, "players", GetOnlineCount());
    FileGate_JsonAddInt(requestId, "maxPlayers", GetMaxPlayers());
    FileGate_JsonSend(requestId, 200);
    return 1;
}

public API_GetPlayers(requestId)
{
    new page = FileGate_GetQueryInt(requestId, "page", 1);
    new limit = FileGate_GetQueryInt(requestId, "limit", 10);
    
    FileGate_JsonStart(requestId);
    FileGate_JsonAddInt(requestId, "page", page);
    FileGate_JsonStartArray(requestId, "players");
    
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (!IsPlayerConnected(i)) continue;
        
        new name[MAX_PLAYER_NAME];
        GetPlayerName(i, name, sizeof(name));
        
        FileGate_JsonStartObject(requestId, "");
        FileGate_JsonAddInt(requestId, "id", i);
        FileGate_JsonAddString(requestId, "name", name);
        FileGate_JsonAddInt(requestId, "score", GetPlayerScore(i));
        FileGate_JsonEndObject(requestId);
    }
    
    FileGate_JsonEndArray(requestId);
    FileGate_JsonSend(requestId, 200);
    return 1;
}

public API_PostAnnounce(requestId)
{
    new message[256];
    FileGate_JsonGetString(requestId, "message", message, sizeof(message));
    
    if (strlen(message) == 0)
    {
        FileGate_RespondError(requestId, 400, "Message required");
        return 1;
    }
    
    SendClientMessageToAll(-1, message);
    
    FileGate_JsonStart(requestId);
    FileGate_JsonAddBool(requestId, "success", true);
    FileGate_JsonAddString(requestId, "message", "Announcement sent");
    FileGate_JsonSend(requestId, 200);
    return 1;
}

public API_GetPlayer(requestId)
{
    new playerId = FileGate_GetParamInt(requestId, "id");
    
    if (!IsPlayerConnected(playerId))
    {
        FileGate_RespondError(requestId, 404, "Player not found");
        return 1;
    }
    
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerId, name, sizeof(name));
    
    FileGate_JsonStart(requestId);
    FileGate_JsonAddInt(requestId, "id", playerId);
    FileGate_JsonAddString(requestId, "name", name);
    FileGate_JsonAddInt(requestId, "score", GetPlayerScore(playerId));
    FileGate_JsonAddInt(requestId, "ping", GetPlayerPing(playerId));
    FileGate_JsonSend(requestId, 200);
    return 1;
}

public OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], 
                      const filepath[], const crc32[], crcMatched)
{
    printf("[FileGate] Uploaded: %s -> %s (CRC: %s)", filename, filepath, crc32);
    return 1;
}

stock GetOnlineCount()
{
    new count = 0;
    for (new i = 0; i < MAX_PLAYERS; i++)
        if (IsPlayerConnected(i)) count++;
    return count;
}
```

---

## Credits

- [AmyrAhmady](https://github.com/AmyrAhmady) for the open.mp component SDK
- [yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib) for the HTTP library
- [Fanorisky](https://github.com/Fanorisky)
