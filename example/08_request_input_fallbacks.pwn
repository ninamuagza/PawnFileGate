#include <open.mp>
#include <PawnREST>

new g_AccountRoute = -1;
new g_AccountQueryRoute = -1;
new g_RegisterRoute = -1;

forward API_GetAccountByPath(requestId);
forward API_GetAccountByQuery(requestId);
forward API_RegisterAccount(requestId);

stock BuildHeaderKey(const key[], output[], outputSize)
{
    new outIdx = 0;
    if (outIdx < outputSize - 1) output[outIdx++] = 'x';
    if (outIdx < outputSize - 1) output[outIdx++] = '-';

    for (new i = 0; key[i] != EOS && outIdx < outputSize - 1; i++)
    {
        new ch = key[i];
        if (ch == '_')
        {
            ch = '-';
        }
        else if (ch >= 'A' && ch <= 'Z')
        {
            ch = ch - 'A' + 'a';
        }
        output[outIdx++] = ch;
    }

    output[outIdx] = EOS;
    return 1;
}

stock bool:GetRequestField(requestId, bodyNode, const key[], output[], outputSize)
{
    if (REST_GetParam(requestId, key, output, outputSize) && strlen(output))
    {
        return true;
    }

    if (REST_GetQuery(requestId, key, output, outputSize) && strlen(output))
    {
        return true;
    }

    new headerKey[48];
    BuildHeaderKey(key, headerKey, sizeof(headerKey));
    if (REST_GetHeader(requestId, headerKey, output, outputSize) && strlen(output))
    {
        return true;
    }

    if (bodyNode != -1 && JsonGetString(bodyNode, key, output, outputSize) && strlen(output))
    {
        return true;
    }

    output[0] = EOS;
    return false;
}

stock ReplyLookup(requestId, const source[])
{
    new discordId[32];
    if (!GetRequestField(requestId, -1, "discord_id", discordId, sizeof(discordId)))
    {
        RespondError(requestId, 400, "discord_id is required");
        return 1;
    }

    new payload = JsonObject(
        "success", JsonBool(true),
        "source", JsonString(source),
        "discord_id", JsonString(discordId)
    );
    RespondNode(requestId, 200, payload);
    JsonCleanup(payload);
    return 1;
}

public OnGameModeInit()
{
    REST_Start(8080);

    g_AccountRoute = REST_RegisterAPIRoute(HTTP_METHOD_GET, "/api/account/{discord_id}", "API_GetAccountByPath");
    g_AccountQueryRoute = REST_RegisterAPIRoute(HTTP_METHOD_GET, "/api/account/by-discord", "API_GetAccountByQuery");
    g_RegisterRoute = REST_RegisterAPIRoute(HTTP_METHOD_POST, "/api/account/register", "API_RegisterAccount");

    REST_SetRouteAuthKey(g_AccountRoute, "bot-secret");
    REST_SetRouteAuthKey(g_AccountQueryRoute, "bot-secret");
    REST_SetRouteAuthKey(g_RegisterRoute, "bot-secret");
    return 1;
}

public API_GetAccountByPath(requestId)
{
    return ReplyLookup(requestId, "path");
}

public API_GetAccountByQuery(requestId)
{
    return ReplyLookup(requestId, "query_or_header");
}

public API_RegisterAccount(requestId)
{
    new body = GetRequestJsonNode(requestId); // Optional body; query/header/path are also supported.

    new discordId[32], discordName[64], username[24];
    new bool:hasDiscordId = GetRequestField(requestId, body, "discord_id", discordId, sizeof(discordId));
    new bool:hasDiscordName = GetRequestField(requestId, body, "discord_name", discordName, sizeof(discordName));
    new bool:hasUsername = GetRequestField(requestId, body, "username", username, sizeof(username));

    if (body != -1)
    {
        JsonCleanup(body);
    }

    if (!hasDiscordId || !hasDiscordName || !hasUsername)
    {
        RespondError(requestId, 400, "discord_id, discord_name, and username are required");
        return 1;
    }

    new payload = JsonObject(
        "success", JsonBool(true),
        "discord_id", JsonString(discordId),
        "discord_name", JsonString(discordName),
        "username", JsonString(username)
    );
    RespondNode(requestId, 201, payload);
    JsonCleanup(payload);
    return 1;
}
