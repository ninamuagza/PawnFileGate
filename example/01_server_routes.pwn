#include <open.mp>
#include <PawnREST>

new g_HealthRoute = -1;
new g_EchoRoute = -1;
new g_HeadRoute = -1;
new g_OptionsRoute = -1;

stock GetMethodName(method, output[], outputSize)
{
    switch (method)
    {
        case HTTP_METHOD_GET: format(output, outputSize, "GET");
        case HTTP_METHOD_POST: format(output, outputSize, "POST");
        case HTTP_METHOD_PUT: format(output, outputSize, "PUT");
        case HTTP_METHOD_PATCH: format(output, outputSize, "PATCH");
        case HTTP_METHOD_DELETE: format(output, outputSize, "DELETE");
        case HTTP_METHOD_HEAD: format(output, outputSize, "HEAD");
        case HTTP_METHOD_OPTIONS: format(output, outputSize, "OPTIONS");
        default: format(output, outputSize, "UNKNOWN");
    }
}

public OnGameModeInit()
{
    REST_Start(8080);

    g_HealthRoute = REST_Route(HTTP_METHOD_GET, "/api/health", "API_Health");
    g_EchoRoute = REST_Route(HTTP_METHOD_POST, "/api/echo", "API_Echo");
    g_HeadRoute = REST_Route(HTTP_METHOD_HEAD, "/api/ping", "API_HeadPing");
    g_OptionsRoute = REST_Route(HTTP_METHOD_OPTIONS, "/api/ping", "API_OptionsPing");

    REST_SetRouteAuth(g_EchoRoute, "api-secret");
    return 1;
}

public API_Health(requestId)
{
    new node = JsonObject(
        "ok", JsonBool(true),
        "port", JsonInt(REST_GetPort()),
        "tls", JsonBool(REST_IsTLSEnabled() != 0)
    );
    RespondNode(requestId, 200, node);
    JsonCleanup(node);
    return 1;
}

public API_Echo(requestId)
{
    new methodName[16], ip[64], path[128], body[512];
    GetMethodName(REST_GetRequestMethod(requestId), methodName, sizeof(methodName));
    REST_GetRequestIP(requestId, ip, sizeof(ip));
    REST_GetRequestPath(requestId, path, sizeof(path));
    REST_GetRequestBody(requestId, body, sizeof(body));

    new reply = JsonObject(
        "method", JsonString(methodName),
        "ip", JsonString(ip),
        "path", JsonString(path),
        "body", JsonString(body)
    );
    SetResponseHeader(requestId, "X-PawnREST", "server-routes-example");
    RespondNode(requestId, 200, reply);
    JsonCleanup(reply);
    return 1;
}

public API_HeadPing(requestId)
{
    SetResponseHeader(requestId, "X-Server", "PawnREST");
    Respond(requestId, 200, "");
    return 1;
}

public API_OptionsPing(requestId)
{
    SetResponseHeader(requestId, "Allow", "GET,POST,PUT,PATCH,DELETE,HEAD,OPTIONS");
    Respond(requestId, 204, "");
    return 1;
}
