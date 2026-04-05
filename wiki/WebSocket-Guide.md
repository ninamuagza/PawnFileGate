# WebSocket Guide

PawnREST provides **WebSocket clients** for text and JSON.

## Text WebSocket client

```pawn
new g_WS;

public OnGameModeInit()
{
    g_WS = REST_WebSocketClient("ws://127.0.0.1:3000/chat", "OnChatMessage");
    return 1;
}

public OnChatMessage(socketId, const data[], dataLen)
{
    printf("[WS:%d] %s", socketId, data);
    return 1;
}
```

Send text:

```pawn
REST_WebSocketSend(g_WS, "hello from server");
```

## JSON WebSocket client

```pawn
new g_JWS;

public OnGameModeInit()
{
    g_JWS = REST_JsonWebSocketClient("ws://127.0.0.1:3000/events", "OnEventMessage");
    return 1;
}

public OnEventMessage(socketId, nodeId)
{
    new kind[24];
    JsonGetString(nodeId, "type", kind, sizeof(kind));
    printf("[JWS:%d] type=%s", socketId, kind);
    JsonCleanup(nodeId);
    return 1;
}
```

Send JSON:

```pawn
new msg = JsonObject();
JsonSetString(msg, "type", "ping");
REST_JsonWebSocketSend(g_JWS, msg);
JsonCleanup(msg);
```

## Disconnect callback

```pawn
forward OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode);
```

- `status`: close code (example: `1000` means normal close)
- `errorCode`: `PAWNREST_ERR_*` when a transport/parser error occurs

## Close and cleanup

```pawn
REST_WebSocketClose(g_WS, 1000, "bye");
REST_RemoveWebSocketClient(g_WS);
```

## TLS notes (wss://)

- `wss://` requires a TLS-enabled build (`PAWNREST_ENABLE_TLS=ON` + compatible OpenSSL).
- If TLS is not available, `wss://` connections will fail.
