# Getting Started

## 1. Install

1. Put `pawnrest.so` / `pawnrest.dll` in your `components` folder.
2. Put `PawnREST.inc` in your Pawn include folder.
3. Add:

```pawn
#include <PawnREST>
```

## 2. Start Server

```pawn
public OnGameModeInit()
{
    PawnREST_Start(8080); // HTTP
    // PawnREST_StartTLS(8443, "cert.pem", "key.pem"); // HTTPS
    return 1;
}
```

## 3. Create a file upload route

```pawn
new g_MapRoute = -1;

public OnGameModeInit()
{
    PawnREST_Start(8080);
    g_MapRoute = PawnREST_RegisterRoute("/maps", "scriptfiles/maps/", ".map,.json", 50);
    PawnREST_AddKey(g_MapRoute, "upload-secret");
    PawnREST_AllowList(g_MapRoute, true);
    return 1;
}
```

## 4. Create a REST endpoint

```pawn
public OnGameModeInit()
{
    PawnREST_Route(HTTP_GET, "/api/healthz", "API_Healthz");
    return 1;
}

public API_Healthz(requestId)
{
    new node = JsonObject();
    JsonSetBool(node, "ok", true);
    RespondNode(requestId, 200, node);
    JsonCleanup(node);
    return 1;
}
```

## 5. Endpoint with JSON body

```pawn
PawnREST_Route(HTTP_POST, "/api/announce", "API_Announce");

public API_Announce(requestId)
{
    new body = RequestJson(requestId);
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
        RespondError(requestId, 400, "message required");
        return 1;
    }

    SendClientMessageToAll(-1, message);
    RespondJSON(requestId, 200, "{\"success\":true}");
    return 1;
}
```

## 6. Quick outbound request

```pawn
new g_API;

public OnGameModeInit()
{
    g_API = RequestsClient("https://api.example.com");
    Request(g_API, "/ping", HTTP_METHOD_GET, "OnPing");
    return 1;
}

public OnPing(requestId, httpStatus, const data[], dataLen)
{
    printf("Ping status=%d body=%s", httpStatus, data);
    return 1;
}
```
