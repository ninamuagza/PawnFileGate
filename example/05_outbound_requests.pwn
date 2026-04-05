#include <open.mp>
#include <PawnREST>

new g_ApiClient = -1;
new g_LastRequestId = -1;

public OnGameModeInit()
{
    new headers[256];
    REST_RequestHeaders(headers, sizeof(headers), "Authorization", "Bearer api-token");
    REST_RequestHeadersAppend(headers, sizeof(headers), "X-Service", "game-server");

    g_ApiClient = REST_RequestsClient("https://api.example.com", headers, true);
    REST_SetRequestsClientHeader(g_ApiClient, "User-Agent", "PawnREST/1.0");

    g_LastRequestId = REST_Request(g_ApiClient, "/status", HTTP_METHOD_GET, "OnStatusText");

    new payload = JsonObject(
        "event", JsonString("player_join"),
        "playerId", JsonInt(12)
    );
    REST_RequestJSON(g_ApiClient, "/events", HTTP_METHOD_POST, "OnEventPosted", payload);
    JsonCleanup(payload);
    return 1;
}

public OnGameModeExit()
{
    if (g_LastRequestId != -1)
    {
        REST_CancelRequest(g_LastRequestId);
    }
    if (g_ApiClient != -1)
    {
        REST_RemoveRequestsClient(g_ApiClient);
    }
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/requeststatus", true))
    {
        new status = REST_GetRequestStatus(g_LastRequestId);
        new http = REST_GetRequestHttpStatus(g_LastRequestId);
        new err = REST_GetRequestErrorCode(g_LastRequestId);
        new msg[96];
        format(msg, sizeof(msg), "req=%d status=%d http=%d err=%d", g_LastRequestId, status, http, err);
        SendClientMessage(playerid, -1, msg);
        return 1;
    }
    return 0;
}

public OnStatusText(requestId, httpStatus, const data[], dataLen)
{
    printf("[REST_Request] id=%d status=%d len=%d body=%s", requestId, httpStatus, dataLen, data);
    return 1;
}

public OnEventPosted(requestId, httpStatus, nodeId)
{
    if (nodeId != -1)
    {
        new ok = JsonGetInt(nodeId, "ok", 0);
        printf("[REST_RequestJSON] id=%d status=%d ok=%d", requestId, httpStatus, ok);
        JsonCleanup(nodeId);
    }
    return 1;
}

public OnRequestFailure(requestId, errorCode, const errorMessage[], len)
{
    printf("[RequestFailure] id=%d code=%d len=%d msg=%s", requestId, errorCode, len, errorMessage);
    return 1;
}

public OnRequestFailureDetailed(requestId, errorCode, const errorType[], const errorMessage[], httpStatus)
{
    printf("[RequestFailureDetailed] id=%d code=%d type=%s http=%d msg=%s", requestId, errorCode, errorType, httpStatus, errorMessage);
    return 1;
}
