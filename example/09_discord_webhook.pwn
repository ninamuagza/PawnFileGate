#include <open.mp>
#include <PawnREST>

#define DISCORD_BASE_URL "https://discord.com"
// Replace with your own webhook path and keep the real token out of version control.
#define DISCORD_WEBHOOK_PATH "/api/webhooks/<webhook-id>/<webhook-token>"

new g_DiscordClient = -1;
new g_LastDiscordRequest = -1;

stock DiscordWebhookClientInit()
{
    new headers[256];
    REST_RequestHeaders(headers, sizeof(headers), "Content-Type", "application/json");
    REST_RequestHeadersAppend(headers, sizeof(headers), "User-Agent", "PawnREST/1.0");

    g_DiscordClient = REST_CreateRequestClient(DISCORD_BASE_URL, headers, true);
    return g_DiscordClient;
}

stock SendDiscordMessage(const message[])
{
    if (g_DiscordClient == -1 || !strlen(message))
    {
        return -1;
    }

    new payload = JsonObject();
    JsonSetString(payload, "content", message);

    new requestId = REST_RequestJSON(
        g_DiscordClient,
        DISCORD_WEBHOOK_PATH,
        HTTP_METHOD_POST,
        "OnDiscordWebhookPosted",
        payload
    );

    JsonCleanup(payload);
    return requestId;
}

stock SendDiscordEmbed(const title[], const description[], color = 0x5865F2)
{
    if (g_DiscordClient == -1)
    {
        return -1;
    }

    new embed = JsonObject();
    JsonSetString(embed, "title", title);
    JsonSetString(embed, "description", description);
    JsonSetInt(embed, "color", color);

    new embeds = JsonArray();
    JsonArrayAppend(embeds, embed);
    JsonCleanup(embed);

    new payload = JsonObject();
    JsonSetObject(payload, "embeds", embeds);
    JsonCleanup(embeds);

    new requestId = REST_RequestJSON(
        g_DiscordClient,
        DISCORD_WEBHOOK_PATH,
        HTTP_METHOD_POST,
        "OnDiscordWebhookPosted",
        payload
    );

    JsonCleanup(payload);
    return requestId;
}

public OnGameModeInit()
{
    DiscordWebhookClientInit();
    return 1;
}

public OnGameModeExit()
{
    if (g_LastDiscordRequest != -1)
    {
        REST_CancelRequest(g_LastDiscordRequest);
    }

    if (g_DiscordClient != -1)
    {
        REST_RemoveRequestsClient(g_DiscordClient);
        g_DiscordClient = -1;
    }
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/discordmsg", true))
    {
        g_LastDiscordRequest = SendDiscordMessage("PawnREST example: hello from open.mp");
        SendClientMessage(playerid, -1, "Queued Discord webhook message.");
        return 1;
    }

    if (!strcmp(cmdtext, "/discordembed", true))
    {
        g_LastDiscordRequest = SendDiscordEmbed(
            "PawnREST webhook example",
            "This embed was sent from a PawnREST request client."
        );
        SendClientMessage(playerid, -1, "Queued Discord webhook embed.");
        return 1;
    }

    if (!strcmp(cmdtext, "/discordstatus", true))
    {
        new status = REST_GetRequestStatus(g_LastDiscordRequest);
        new httpStatus = REST_GetRequestHttpStatus(g_LastDiscordRequest);
        new errorCode = REST_GetRequestErrorCode(g_LastDiscordRequest);
        new msg[96];
        format(msg, sizeof(msg), "req=%d status=%d http=%d err=%d", g_LastDiscordRequest, status, httpStatus, errorCode);
        SendClientMessage(playerid, -1, msg);
        return 1;
    }
    return 0;
}

public OnDiscordWebhookPosted(requestId, httpStatus, nodeId)
{
    if (httpStatus == 204)
    {
        printf("[Discord] webhook delivered id=%d status=%d", requestId, httpStatus);
    }
    else if (nodeId != -1 && JsonNodeType(nodeId) == JSON_NODE_OBJECT)
    {
        new errMsg[256];
        JsonGetString(nodeId, "message", errMsg, sizeof(errMsg));
        printf("[Discord] webhook response id=%d status=%d message=%s", requestId, httpStatus, errMsg);
    }
    else
    {
        printf("[Discord] webhook response id=%d status=%d", requestId, httpStatus);
    }

    if (nodeId != -1)
    {
        JsonCleanup(nodeId);
    }
    return 1;
}

public OnRequestFailure(requestId, errorCode, const errorType[], const errorMessage[], httpStatus)
{
    printf(
        "[Discord] request failed id=%d code=%d type=%s http=%d msg=%s",
        requestId,
        errorCode,
        errorType,
        httpStatus,
        errorMessage
    );
    return 1;
}
