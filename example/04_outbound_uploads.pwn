#include <open.mp>
#include <PawnREST>

new g_UploadClient = -1;
new g_LastUploadId = -1;

public OnGameModeInit()
{
    new headers[256];
    REST_RequestHeaders(headers, sizeof(headers), "X-Server", "PawnREST");
    REST_RequestHeadersAppend(headers, sizeof(headers), "X-Env", "production");

    g_UploadClient = REST_CreateUploadClient("https://uploads.example.com", headers, true);
    if (g_UploadClient != -1)
    {
        REST_SetUploadClientHeader(g_UploadClient, "User-Agent", "PawnREST/1.0");
    }
    return 1;
}

public OnGameModeExit()
{
    if (g_LastUploadId != -1)
    {
        REST_CancelUpload(g_LastUploadId);
    }
    if (g_UploadClient != -1)
    {
        REST_RemoveUploadClient(g_UploadClient);
    }
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/uploadmap", true))
    {
        g_LastUploadId = REST_UploadFileWithClient(
            g_UploadClient,
            "/incoming/maps",
            "scriptfiles/maps/test.map",
            "test.map",
            "upload-token",
            "",
            1,
            UPLOAD_MODE_MULTIPART
        );
        SendClientMessage(playerid, -1, "Started outbound upload.");
        return 1;
    }

    if (!strcmp(cmdtext, "/uploadstatus", true))
    {
        new status = REST_GetUploadStatus(g_LastUploadId);
        new progress = REST_GetUploadProgress(g_LastUploadId);
        new msg[96];
        format(msg, sizeof(msg), "uploadId=%d status=%d progress=%d%%", g_LastUploadId, status, progress);
        SendClientMessage(playerid, -1, msg);
        return 1;
    }
    return 0;
}

public OnFileUploadStarted(uploadId)
{
    printf("[Outbound Upload] started id=%d", uploadId);
    return 1;
}

public OnFileUploadProgress(uploadId, percent)
{
    printf("[Outbound Upload] progress id=%d percent=%d", uploadId, percent);
    return 1;
}

public OnFileUploadCompleted(uploadId, httpStatus, const responseBody[], const crc32[])
{
    printf("[Outbound Upload] completed id=%d status=%d crc=%s body=%s", uploadId, httpStatus, crc32, responseBody);
    return 1;
}

public OnFileUploadFailed(uploadId, const errorMessage[])
{
    printf("[Outbound Upload] failed id=%d msg=%s", uploadId, errorMessage);
    return 1;
}

public OnFileUploadFailure(uploadId, errorCode, const errorType[], const errorMessage[], httpStatus)
{
    printf("[Outbound Upload] failure id=%d code=%d type=%s status=%d msg=%s", uploadId, errorCode, errorType, httpStatus, errorMessage);
    return 1;
}
