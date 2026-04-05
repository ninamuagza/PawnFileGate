#include <open.mp>
#include <PawnREST>

new g_TextSocket = -1;
new g_JsonSocket = -1;

public OnGameModeInit()
{
    g_TextSocket = REST_WebSocketClient("ws://127.0.0.1:3000/chat", "OnChatMessage");
    g_JsonSocket = REST_JsonWebSocketClient("ws://127.0.0.1:3000/events", "OnEventMessage");
    return 1;
}

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

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/wstext", true))
    {
        REST_WebSocketSend(g_TextSocket, "hello from PawnREST");
        SendClientMessage(playerid, -1, "Text websocket message sent.");
        return 1;
    }

    if (!strcmp(cmdtext, "/wsjson", true))
    {
        new msg = JsonObject("type", JsonString("ping"), "from", JsonString("server"));
        REST_JsonWebSocketSend(g_JsonSocket, msg);
        JsonCleanup(msg);
        SendClientMessage(playerid, -1, "JSON websocket message sent.");
        return 1;
    }
    return 0;
}

public OnChatMessage(socketId, const data[], dataLen)
{
    printf("[WS TEXT] socket=%d len=%d data=%s", socketId, dataLen, data);
    return 1;
}

public OnEventMessage(socketId, nodeId)
{
    new kind[32];
    JsonGetString(nodeId, "type", kind, sizeof(kind));
    printf("[WS JSON] socket=%d type=%s", socketId, kind);
    JsonCleanup(nodeId);
    return 1;
}

public OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode)
{
    printf("[WS DISC] socket=%d json=%d status=%d error=%d reasonLen=%d reason=%s", socketId, _:isJson, status, errorCode, reasonLen, reason);
    return 1;
}
