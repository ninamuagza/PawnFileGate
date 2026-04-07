# Getting Started

This page walks through a minimal, production-shaped setup: server bootstrap, file route, custom API route, JSON request handling, and a quick validation flow.

## 1. Install

1. Copy `PawnREST.so` (Linux) or `PawnREST.dll` (Windows) to your server `components/` directory.
2. Copy `PawnREST.inc` to your Pawn include directory.
3. Add the include in your gamemode:

```pawn
#include <PawnREST>
```

Linux note:

- The plugin architecture must match your server runtime (`32-bit` vs `64-bit`).
- Check with: `file components/PawnREST.so`
- If your environment requires 32-bit, build with `-m32`:

```bash
cmake -S . -B build-32 -G Ninja \
  -DCMAKE_C_FLAGS=-m32 \
  -DCMAKE_CXX_FLAGS=-m32 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-32 --parallel
```

`docker/build.sh` already compiles with `-m32` by default.

## 2. Bootstrap the HTTP Server

```pawn
public OnGameModeInit()
{
    // HTTP
    REST_Start(8080);

    // HTTPS (TLS build required)
    // REST_StartTLS(8443, "cert.pem", "key.pem");
    return 1;
}
```

## 3. Register an Inbound File Route

```pawn
new g_MapRoute = -1;

public OnGameModeInit()
{
    REST_Start(8080);

    g_MapRoute = FILE_RegisterRoute("/maps", "scriptfiles/maps/", ".map,.json", 50);
    FILE_AddAuthKey(g_MapRoute, "upload-secret");
    FILE_SetRequireCRC32(g_MapRoute, true);
    FILE_AllowList(g_MapRoute, true);
    FILE_AllowDownload(g_MapRoute, true);
    return 1;
}
```

Route behavior:
- Upload endpoint: `POST /maps`
- File list endpoint: `GET /maps/files`
- Download endpoint: `GET /maps/files/{filename}`

## 4. Register a Custom REST Endpoint

```pawn
new g_HealthRoute = -1;

public OnGameModeInit()
{
    REST_Start(8080);
    g_HealthRoute = REST_RegisterAPIRoute(HTTP_METHOD_GET, "/api/healthz", "API_Healthz");
    return 1;
}

public API_Healthz(requestId)
{
    new payload = JsonObject(
        "ok", JsonBool(true),
        "port", JsonInt(REST_GetPort())
    );
    RespondNode(requestId, 200, payload);
    JsonCleanup(payload);
    return 1;
}
```

## 5. Handle JSON Request Bodies

```pawn
REST_RegisterAPIRoute(HTTP_METHOD_POST, "/api/announce", "API_Announce");

public API_Announce(requestId)
{
    new body = GetRequestJsonNode(requestId);
    if (body == -1)
    {
        RespondError(requestId, 400, "Invalid JSON");
        return 1;
    }

    new message[144];
    JsonGetString(body, "message", message, sizeof(message));
    JsonCleanup(body);

    if (!strlen(message))
    {
        RespondError(requestId, 400, "message is required");
        return 1;
    }

    SendClientMessageToAll(-1, message);

    new response = JsonObject("success", JsonBool(true));
    RespondNode(requestId, 200, response);
    JsonCleanup(response);
    return 1;
}
```

## 6. Optional: Quick Outbound Request

```pawn
new g_API = -1;

public OnGameModeInit()
{
    g_API = REST_CreateRequestClient("https://api.example.com");
    REST_Request(g_API, "/ping", HTTP_METHOD_GET, "OnPing");
    return 1;
}

public OnPing(requestId, httpStatus, const data[], dataLen)
{
    printf("request=%d status=%d len=%d body=%s", requestId, httpStatus, dataLen, data);
    return 1;
}
```

## 7. Validate with curl

```bash
# health route
curl http://127.0.0.1:8080/api/healthz

# JSON endpoint
curl -X POST http://127.0.0.1:8080/api/announce \
  -H "Content-Type: application/json" \
  -d '{"message":"Hello from API"}'
```

## 8. Next Steps

1. Review all natives in [API Reference](API-Reference).
2. Follow [JSON Node Guide](JSON-Node-Guide) for ownership/lifecycle rules.
3. Add outbound integrations with [Outbound HTTP Guide](Outbound-HTTP-Guide) and [WebSocket Guide](WebSocket-Guide).
4. Use [Troubleshooting](Troubleshooting) for architecture mismatch, route 404, and missing-input diagnostics.
