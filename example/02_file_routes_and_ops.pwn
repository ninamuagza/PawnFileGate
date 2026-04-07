#include <open.mp>
#include <PawnREST>

new g_MapRoute = -1;

stock PrintRouteFiles(routeId)
{
    new count = REST_GetFileCount(routeId);
    printf("[Files] route=%d count=%d", routeId, count);

    for (new i = 0; i < count; i++)
    {
        new name[128];
        if (REST_GetFileName(routeId, i, name, sizeof(name)))
        {
            printf("  - %s", name);
        }
    }
}

public OnGameModeInit()
{
    REST_Start(8080);

    g_MapRoute = REST_RegisterRoute("/maps", "scriptfiles/maps/", ".map,.json", 50);
    REST_AddKey(g_MapRoute, "upload-secret");
    REST_SetConflict(g_MapRoute, CONFLICT_RENAME);
    REST_SetCorruptAction(g_MapRoute, CORRUPT_QUARANTINE);
    REST_SetRequireCRC32(g_MapRoute, true);

    // Enable REST API endpoints for file management:
    //   GET  /maps/files          -> list files (JSON: { success, count, files: ["a.map","b.json"] })
    //   GET  /maps/files/{name}   -> download file (raw binary)
    //   GET  /maps/files/{name}/info -> file metadata (JSON: { success, name, size, modified })
    //   DELETE /maps/files/{name} -> delete file (JSON: { success, deleted })
    REST_AllowList(g_MapRoute, true);
    REST_AllowDownload(g_MapRoute, true);
    REST_AllowDelete(g_MapRoute, true);
    REST_AllowInfo(g_MapRoute, true);
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/maps", true))
    {
        PrintRouteFiles(g_MapRoute);
        return 1;
    }

    if (!strcmp(cmdtext, "/deletemap", true))
    {
        new firstName[128];
        if (REST_GetFileName(g_MapRoute, 0, firstName, sizeof(firstName)))
        {
            if (REST_DeleteFile(g_MapRoute, firstName))
            {
                SendClientMessage(playerid, -1, "Deleted first file from /maps route.");
            }
        }
        return 1;
    }
    return 0;
}

public OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], const filepath[], const crc32[], crcMatched)
{
    printf("[Upload OK] id=%d route=%d endpoint=%s file=%s crc=%s match=%d", uploadId, routeId, endpoint, filename, crc32, crcMatched);
    return 1;
}

public OnFileFailedUpload(uploadId, const reason[], const crc32[])
{
    printf("[Upload FAIL] id=%d reason=%s crc=%s", uploadId, reason, crc32);
    return 1;
}

public OnUploadProgress(uploadId, percent)
{
    printf("[Upload PROGRESS] id=%d %d%%", uploadId, percent);
    return 1;
}
