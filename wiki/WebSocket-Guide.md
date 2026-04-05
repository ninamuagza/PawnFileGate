# WebSocket Guide

PawnREST provides outbound websocket clients for text and JSON message flows.

## 1. Connect a Text WebSocket

```pawn
new g_TextSocket = -1;

public OnGameModeInit()
{
    g_TextSocket = REST_WebSocketClient("ws://127.0.0.1:3000/chat", "OnChatMessage");
    return 1;
}

public OnChatMessage(socketId, const data[], dataLen)
{
    printf("[WS TEXT] socket=%d len=%d data=%s", socketId, dataLen, data);
    return 1;
}
```

Text callback signature:

```pawn
public YourTextSocketCallback(socketId, const data[], dataLen)
```

## 2. Connect a JSON WebSocket

```pawn
new g_JsonSocket = -1;

public OnGameModeInit()
{
    g_JsonSocket = REST_JsonWebSocketClient("ws://127.0.0.1:3000/events", "OnEventMessage");
    return 1;
}

public OnEventMessage(socketId, nodeId)
{
    new kind[24];
    JsonGetString(nodeId, "type", kind, sizeof(kind));
    printf("[WS JSON] socket=%d type=%s", socketId, kind);
    JsonCleanup(nodeId);
    return 1;
}
```

JSON callback signature:

```pawn
public YourJsonSocketCallback(socketId, nodeId)
```

## 3. Send Messages

### Send Text

```pawn
REST_WebSocketSend(g_TextSocket, "hello from server");
```

### Send JSON

```pawn
new msg = JsonObject("type", JsonString("ping"), "from", JsonString("server"));
REST_JsonWebSocketSend(g_JsonSocket, msg);
JsonCleanup(msg);
```

## 4. Detect Disconnects

```pawn
forward OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode);
```

- `status`: websocket close code (`1000` = normal close)
- `errorCode`: `PAWNREST_ERR_*` when disconnect is caused by transport/parser error

## 5. Close and Cleanup

```pawn
public OnGameModeExit()
{
    if (g_TextSocket != -1 && REST_IsWebSocketOpen(g_TextSocket))
    {
        REST_WebSocketClose(g_TextSocket, 1000, "shutdown");
        REST_RemoveWebSocketClient(g_TextSocket);
    }

    if (g_JsonSocket != -1 && REST_IsWebSocketOpen(g_JsonSocket))
    {
        REST_WebSocketClose(g_JsonSocket, 1000, "shutdown");
        REST_RemoveWebSocketClient(g_JsonSocket);
    }
    return 1;
}
```

## 6. Optional Headers and TLS Verification

Both constructors support custom headers and TLS verification:

```pawn
new headers[128];
REST_RequestHeaders(headers, sizeof(headers), "Authorization", "Bearer ws-token");

new socketId = REST_WebSocketClient("wss://example.com/realtime", "OnChatMessage", headers, true);
```

## 7. Reconnect Strategy

Recommended production pattern:

1. Store connection configuration (URL, callback, headers).
2. In `OnWebSocketDisconnect`, schedule reconnect with timer/backoff.
3. Recreate socket with `REST_WebSocketClient` or `REST_JsonWebSocketClient`.
4. Ensure old handle is removed with `REST_RemoveWebSocketClient`.
