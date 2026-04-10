#pragma rational Float

#include <core>
#include <float>
#include <string>

native format(output[], len, const string[], {Float,_}:...);

#include <PawnREST>

forward OnSmokeHealth(requestId);

main()
{
    new routeId = REST_RegisterAPIRoute(HTTP_METHOD_GET, "/api/health", "OnSmokeHealth");
    new fileRoute = FILE_RegisterRoute("/maps", "scriptfiles/maps/", ".map,.json", 16);
    new headers[128];
    new payload = JsonObject(
        "ok", JsonBool(true),
        "port", JsonInt(REST_GetPort())
    );

    REST_RequestHeaders(headers, sizeof(headers), "X-Smoke", "1");

    FILE_AddAuthKey(fileRoute, "secret");
    FILE_AllowList(fileRoute, true);
    FILE_AllowDownload(fileRoute, true);
    FILE_AllowDelete(fileRoute, false);
    FILE_AllowInfo(fileRoute, true);

    REST_SetRouteAuthKey(routeId, "secret");
    SetResponseHeader(0, "X-PawnREST-Smoke", headers);
    RespondNode(0, 200, payload);
    JsonCleanup(payload);

    return routeId + fileRoute;
}

public OnSmokeHealth(requestId)
{
    new reply = JsonObject(
        "method", JsonInt(REST_GetRequestMethod(requestId)),
        "tls", JsonBool(REST_IsTLSEnabled() != 0)
    );

    SetResponseHeader(requestId, "X-PawnREST-Smoke", "1");
    RespondNode(requestId, 200, reply);
    JsonCleanup(reply);
    return 1;
}

public OnIncomingUploadCompleted(uploadId, routeId, const endpoint[], const filename[], const filepath[], const crc32[], crcMatched)
{
    return uploadId + routeId + crcMatched + endpoint[0] + filename[0] + filepath[0] + crc32[0];
}

public OnIncomingUploadFailed(uploadId, const reason[], const crc32[])
{
    return uploadId + reason[0] + crc32[0];
}

public OnIncomingUploadProgress(uploadId, percent)
{
    return uploadId + percent;
}

public OnOutgoingUploadStarted(uploadId)
{
    return uploadId;
}

public OnOutgoingUploadProgress(uploadId, percent)
{
    return uploadId + percent;
}

public OnOutgoingUploadCompleted(uploadId, httpStatus, const responseBody[], const crc32[])
{
    return uploadId + httpStatus + responseBody[0] + crc32[0];
}

public OnOutgoingUploadFailed(uploadId, errorCode, const errorType[], const errorMessage[], httpStatus)
{
    return uploadId + errorCode + httpStatus + errorType[0] + errorMessage[0];
}

public OnRequestFailure(requestId, errorCode, const errorType[], const errorMessage[], httpStatus)
{
    return requestId + errorCode + httpStatus + errorType[0] + errorMessage[0];
}

public OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode)
{
    return socketId + _:isJson + status + reasonLen + errorCode + reason[0];
}
