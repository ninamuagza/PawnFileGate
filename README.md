# PawnREST - HTTP File Transfer & REST API Framework

An open.mp server component that provides HTTP file upload/download functionality and a full REST API framework for SA-MP/open.mp servers.

## Wiki

API documentation and use-case guides are available in [`wiki/`](./wiki/):

- [`Home`](./wiki/Home.md)
- [`Getting Started`](./wiki/Getting-Started.md)
- [`API Reference`](./wiki/API-Reference.md)
- [`JSON Node Guide`](./wiki/JSON-Node-Guide.md)
- [`Outbound HTTP Guide`](./wiki/Outbound-HTTP-Guide.md)
- [`WebSocket Guide`](./wiki/WebSocket-Guide.md)
- [`Callbacks`](./wiki/Callbacks.md)
- [`Use Cases`](./wiki/Use-Cases.md)
- [`Migration from pawn-requests`](./wiki/Migration-from-pawn-requests.md)

## Features

- **File Upload Server** - Receive files via HTTP POST with validation
- **File Download API** - Serve files via HTTP GET
- **Outgoing Uploads** - Upload files to external servers
- **Upload Clients** - Reuse base URL and default headers for outbound uploads
- **Outbound HTTP Requests** - RequestsClient-style `Request`/`RequestJSON`
- **WebSocket Clients** - Text and JSON websocket clients (`ws://` and optional `wss://`)
- **REST API Framework** - Create custom HTTP endpoints (GET, POST, PUT, PATCH, DELETE, HEAD, OPTIONS)
- **Node-only JSON API** - Parse/build JSON with node handles (pawn-requests style)
- **Authorization** - Bearer token authentication per route
- **TLS/HTTPS Support** - HTTPS server/client support when built with OpenSSL
- **Structured Error Callbacks** - Typed/coded failure metadata for outgoing uploads
- **CRC32 Integrity** - File checksum validation

---

## Installation

1. Download the latest release for your platform (`.dll` for Windows, `.so` for Linux)
2. Place it in your open.mp server `components/` directory
3. Copy `PawnREST.inc` to your Pawn compiler includes directory
4. Add `#include <PawnREST>` to your script

To enable TLS/HTTPS in source builds, configure with `-DPAWNREST_ENABLE_TLS=ON` and provide OpenSSL libraries compatible with your target architecture.
For release builds, CI also publishes a separate Linux static-OpenSSL artifact.
For local Docker builds of the static-SSL variant, run `BUILD_DIR=build-static-ssl PAWNREST_ENABLE_TLS=ON PAWNREST_TLS_STATIC_OPENSSL=ON ./docker/build.sh`.
`docker/build.sh` first tries prebuilt `omp-easing-functions/build:*` images and falls back to local Dockerfile builds only when needed.

---

## Quick Start

```pawn
#include <a_samp>
#include <PawnREST>

new g_MapRoute = -1;
new g_ApiPlayers = -1;

public OnGameModeInit()
{
    // Start HTTP server on port 8080
    PawnREST_Start(8080);
    
    // === FILE UPLOAD ROUTE ===
    g_MapRoute = PawnREST_RegisterRoute("/maps", "scriptfiles/maps/", ".map,.json", 50);
    PawnREST_AddKey(g_MapRoute, "upload-secret-key");
    
    // Enable REST API for files
    PawnREST_AllowList(g_MapRoute, true);
    PawnREST_AllowDownload(g_MapRoute, true);
    
    // === CUSTOM REST API ===
    g_ApiPlayers = PawnREST_Route(HTTP_GET, "/api/players", "OnGetPlayers");
    PawnREST_SetRouteAuth(g_ApiPlayers, "api-secret-key");
    
    return 1;
}

// Handle GET /api/players
public OnGetPlayers(requestId)
{
    new payload = JsonObject();
    JsonSetInt(payload, "online", GetPlayerCount());
    JsonSetInt(payload, "max", GetMaxPlayers());
    RespondNode(requestId, 200, payload);
    JsonCleanup(payload);
    return 1;
}

public OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], 
                      const filepath[], const crc32[], crcMatched)
{
    printf("[PawnREST] File uploaded: %s", filename);
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
| `HTTP_HEAD` | HEAD |
| `HTTP_OPTIONS` | OPTIONS |

---

## Server Control

```pawn
// Start/stop HTTP server
native bool:PawnREST_Start(port);
native bool:PawnREST_StartTLS(port, const certPath[], const keyPath[]);
native bool:PawnREST_Stop();
native PawnREST_IsRunning();
native PawnREST_GetPort();
native PawnREST_IsTLSEnabled();
```

---

## File Upload Routes

```pawn
// Register upload endpoint
native PawnREST_RegisterRoute(const endpoint[], const path[], const allowedExts[], maxSizeMb);

// Authorization
native bool:PawnREST_AddKey(routeId, const key[]);
native bool:PawnREST_RemoveKey(routeId, const key[]);

// Settings
native bool:PawnREST_SetConflict(routeId, mode);      // CONFLICT_RENAME/OVERWRITE/REJECT
native bool:PawnREST_SetCorruptAction(routeId, act);  // CORRUPT_DELETE/QUARANTINE/KEEP
native bool:PawnREST_SetRequireCRC32(routeId, bool:required);
native bool:PawnREST_RemoveRoute(routeId);

// REST API permissions for file routes
native bool:PawnREST_AllowList(routeId, bool:allow);     // GET {route}/files
native bool:PawnREST_AllowDownload(routeId, bool:allow); // GET {route}/files/{name}
native bool:PawnREST_AllowDelete(routeId, bool:allow);   // DELETE {route}/files/{name}
native bool:PawnREST_AllowInfo(routeId, bool:allow);     // GET {route}/files/{name}/info

// File operations
native PawnREST_GetFileCount(routeId);
native PawnREST_GetFileName(routeId, index, output[], outputSize);
native bool:PawnREST_DeleteFile(routeId, const filename[]);
native PawnREST_GetFileSize(routeId, const filename[]);
```

---

## Custom REST API Routes

```pawn
// Register custom endpoint
native PawnREST_Route(method, const endpoint[], const callback[]);
native bool:PawnREST_RemoveAPIRoute(routeId);
native bool:PawnREST_SetRouteAuth(routeId, const key[]);

// Examples:
PawnREST_Route(HTTP_GET, "/api/server", "OnGetServer");
PawnREST_Route(HTTP_POST, "/api/ban", "OnPostBan");
PawnREST_Route(HTTP_GET, "/api/player/{id}", "OnGetPlayer");  // URL params
PawnREST_Route(HTTP_DELETE, "/api/vehicle/{id}", "OnDeleteVehicle");
```

---

## Request Data Access

```pawn
// Basic request info
native PawnREST_GetRequestIP(requestId, output[], outputSize);
native PawnREST_GetRequestMethod(requestId);
native PawnREST_GetRequestPath(requestId, output[], outputSize);
native PawnREST_GetRequestBody(requestId, output[], outputSize);
native PawnREST_GetRequestBodyLength(requestId);

// URL parameters ({id} from /api/player/{id})
native PawnREST_GetParam(requestId, const name[], output[], outputSize);
native PawnREST_GetParamInt(requestId, const name[]);

// Query string (?page=1&limit=10)
native PawnREST_GetQuery(requestId, const name[], output[], outputSize);
native PawnREST_GetQueryInt(requestId, const name[], defaultValue = 0);

// Headers
native PawnREST_GetHeader(requestId, const name[], output[], outputSize);
```

---

## JSON API (Node-only)

```pawn
// Parse JSON
native RequestJson(requestId);
native JsonParse(const json[]);
native JsonNodeType(nodeId);                          // JSON_NODE_*
native JsonStringify(nodeId, output[], outputSize);
native bool:JsonCleanup(nodeId);

// Constructors
native JsonObject(...);  // key, node pairs
native JsonArray(...);   // node list
native JsonString(const value[]);
native JsonInt(value);
native JsonFloat(Float:value);
native JsonBool(bool:value);
native JsonNull();
native JsonAppend(leftNodeId, rightNodeId);

// Object operations
native bool:JsonSetObject(objectNodeId, const key[], valueNodeId);
native bool:JsonSetString(objectNodeId, const key[], const value[]);
native bool:JsonSetInt(objectNodeId, const key[], value);
native bool:JsonSetFloat(objectNodeId, const key[], Float:value);
native bool:JsonSetBool(objectNodeId, const key[], bool:value);
native bool:JsonSetNull(objectNodeId, const key[]);
native bool:JsonHas(objectNodeId, const key[]);
native JsonGetObject(objectNodeId, const key[]);
native JsonGetString(objectNodeId, const key[], output[], outputSize);
native JsonGetInt(objectNodeId, const key[], defaultValue = 0);
native Float:JsonGetFloat(objectNodeId, const key[], Float:defaultValue = 0.0);
native bool:JsonGetBool(objectNodeId, const key[], bool:defaultValue = false);

// Array operations
native JsonArrayLength(arrayNodeId);
native JsonArrayObject(arrayNodeId, index);
native bool:JsonArrayAppend(arrayNodeId, valueNodeId);
native bool:JsonArrayAppendString(arrayNodeId, const value[]);
native bool:JsonArrayAppendInt(arrayNodeId, value);
native bool:JsonArrayAppendFloat(arrayNodeId, Float:value);
native bool:JsonArrayAppendBool(arrayNodeId, bool:value);
native bool:JsonArrayAppendNull(arrayNodeId);
```

Builder style (pawn-requests style):
```pawn
new payload = JsonObject(
    "name", JsonString("PawnREST"),
    "players", JsonArray(
        JsonObject("id", JsonInt(0), "name", JsonString("Alice")),
        JsonObject("id", JsonInt(1), "name", JsonString("Bob"))
    )
);
```

## Response Methods

```pawn
native bool:Respond(requestId, status, const body[], const contentType[] = "application/json");
native bool:RespondJSON(requestId, status, const json[]);
native bool:RespondNode(requestId, status, nodeId);
native bool:RespondError(requestId, status, const message[]);
native bool:SetResponseHeader(requestId, const name[], const value[]);
```

---

## Outgoing Uploads

```pawn
// Upload file to external server
native PawnREST_UploadFile(
    const url[],
    const filepath[],
    const filename[] = "",
    const authKey[] = "",
    const customHeaders[] = "",
    calculateCrc32 = 1,
    mode = UPLOAD_MODE_MULTIPART,
    bool:verifyTls = true
);

// Reusable upload clients
native PawnREST_CreateUploadClient(const baseUrl[], const defaultHeaders[] = "", bool:verifyTls = true);
native bool:PawnREST_RemoveUploadClient(clientId);
native bool:PawnREST_SetUploadClientHeader(clientId, const name[], const value[]);
native bool:PawnREST_RemoveUploadClientHeader(clientId, const name[]);
native PawnREST_UploadFileWithClient(clientId, const path[], const filepath[], const filename[] = "", const authKey[] = "", const customHeaders[] = "", calculateCrc32 = 1, mode = UPLOAD_MODE_MULTIPART);

native bool:PawnREST_CancelUpload(uploadId);
native PawnREST_GetUploadStatus(uploadId);
native PawnREST_GetUploadProgress(uploadId);
native PawnREST_GetUploadResponse(uploadId, output[], outputSize);
native PawnREST_GetUploadErrorCode(uploadId);
native PawnREST_GetUploadErrorType(uploadId, output[], outputSize);
native PawnREST_GetUploadHttpStatus(uploadId);
```

---

## Outbound Requests (pawn-requests style)

```pawn
// Reusable HTTP client
native RequestsClient(const endpoint[], const defaultHeaders[] = "", bool:verifyTls = true);
native bool:RemoveRequestsClient(clientId);
native bool:SetRequestsClientHeader(clientId, const name[], const value[]);
native bool:RemoveRequestsClientHeader(clientId, const name[]);

// Async requests
native Request(clientId, const path[], method, const callback[], const body[] = "", const headers[] = "");
native RequestJSON(clientId, const path[], method, const callback[], jsonNodeId = -1, const headers[] = "");

// Optional state polling
native RequestStatus(requestId);      // REQUEST_*
native RequestHTTPStatus(requestId);
native RequestErrorCode(requestId);   // PAWNREST_ERR_*
native RequestErrorType(requestId, output[], outputSize);
native RequestResponse(requestId, output[], outputSize);
```

**Callback signatures**
```pawn
// Request(...)
public OnTextResponse(requestId, httpStatus, const data[], dataLen)

// RequestJSON(...)
public OnJsonResponse(requestId, httpStatus, nodeId)

// transport/internal failures
forward OnRequestFailure(requestId, errorCode, const errorMessage[], len);
forward OnRequestFailureDetailed(requestId, errorCode, const errorType[], const errorMessage[], httpStatus);
```

---

## WebSocket Client

```pawn
native WebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);
native JsonWebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);
native bool:WebSocketSend(socketId, const data[]);
native bool:JsonWebSocketSend(socketId, nodeId);
native bool:WebSocketClose(socketId, status = 1000, const reason[] = "");
native bool:RemoveWebSocketClient(socketId);
native bool:IsWebSocketOpen(socketId);
```

**Callback signatures**
```pawn
// WebSocketClient
public OnSocketText(socketId, const data[], dataLen)

// JsonWebSocketClient
public OnSocketJson(socketId, nodeId)

forward OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode);
```

---

## CRC32 Utilities

```pawn
native PawnREST_VerifyCRC32(const filepath[], const expectedCrc[]);
native PawnREST_GetFileCRC32(const filepath[], output[], outputSize);
native PawnREST_CompareFiles(const path1[], const path2[]);
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
forward OnFileUploadFailure(uploadId, errorCode, const errorType[], const errorMessage[], httpStatus);
```

### Outbound Request / WebSocket
```pawn
forward OnRequestFailure(requestId, errorCode, const errorMessage[], len);
forward OnRequestFailureDetailed(requestId, errorCode, const errorType[], const errorMessage[], httpStatus);
forward OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode);
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
#include <PawnREST>

new g_MapRoute = -1;

public OnGameModeInit()
{
    PawnREST_Start(8080);
    
    // File upload route
    g_MapRoute = PawnREST_RegisterRoute("/maps", "scriptfiles/maps/", ".map,.json", 50);
    PawnREST_AddKey(g_MapRoute, "secret-key");
    PawnREST_AllowList(g_MapRoute, true);
    PawnREST_AllowDownload(g_MapRoute, true);
    PawnREST_AllowInfo(g_MapRoute, true);
    
    // Custom API routes
    PawnREST_Route(HTTP_GET, "/api/server", "API_GetServer");
    PawnREST_Route(HTTP_GET, "/api/players", "API_GetPlayers");
    PawnREST_Route(HTTP_POST, "/api/announce", "API_PostAnnounce");
    PawnREST_Route(HTTP_GET, "/api/player/{id}", "API_GetPlayer");
    
    return 1;
}

public API_GetServer(requestId)
{
    new payload = JsonObject();
    JsonSetString(payload, "name", "My Server");
    JsonSetInt(payload, "players", GetOnlineCount());
    JsonSetInt(payload, "maxPlayers", GetMaxPlayers());
    RespondNode(requestId, 200, payload);
    JsonCleanup(payload);
    return 1;
}

public API_GetPlayers(requestId)
{
    new page = PawnREST_GetQueryInt(requestId, "page", 1);
    new limit = PawnREST_GetQueryInt(requestId, "limit", 10);
    
    new payload = JsonObject();
    new players = JsonArray();
    JsonSetInt(payload, "page", page);
    
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (!IsPlayerConnected(i)) continue;
        
        new name[MAX_PLAYER_NAME];
        GetPlayerName(i, name, sizeof(name));
        
        new entry = JsonObject();
        JsonSetInt(entry, "id", i);
        JsonSetString(entry, "name", name);
        JsonSetInt(entry, "score", GetPlayerScore(i));
        JsonArrayAppend(players, entry);
        JsonCleanup(entry);
    }
    
    JsonSetObject(payload, "players", players);
    JsonCleanup(players);
    RespondNode(requestId, 200, payload);
    JsonCleanup(payload);
    return 1;
}

public API_PostAnnounce(requestId)
{
    new body = RequestJson(requestId);
    if (body == -1)
    {
        RespondError(requestId, 400, "Invalid JSON body");
        return 1;
    }

    new message[256];
    JsonGetString(body, "message", message, sizeof(message));
    JsonCleanup(body);
    
    if (strlen(message) == 0)
    {
        RespondError(requestId, 400, "Message required");
        return 1;
    }
    
    SendClientMessageToAll(-1, message);
    
    new payload = JsonObject();
    JsonSetBool(payload, "success", true);
    JsonSetString(payload, "message", "Announcement sent");
    RespondNode(requestId, 200, payload);
    JsonCleanup(payload);
    return 1;
}

public API_GetPlayer(requestId)
{
    new playerId = PawnREST_GetParamInt(requestId, "id");
    
    if (!IsPlayerConnected(playerId))
    {
        RespondError(requestId, 404, "Player not found");
        return 1;
    }
    
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerId, name, sizeof(name));
    
    new payload = JsonObject();
    JsonSetInt(payload, "id", playerId);
    JsonSetString(payload, "name", name);
    JsonSetInt(payload, "score", GetPlayerScore(playerId));
    JsonSetInt(payload, "ping", GetPlayerPing(playerId));
    RespondNode(requestId, 200, payload);
    JsonCleanup(payload);
    return 1;
}

public OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], 
                      const filepath[], const crc32[], crcMatched)
{
    printf("[PawnREST] Uploaded: %s -> %s (CRC: %s)", filename, filepath, crc32);
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
