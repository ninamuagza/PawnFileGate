#include <open.mp>
#include <PawnREST>

new g_MapRoute = -1;

stock PrintRouteFiles(routeId)
{
    new count = FILE_GetCount(routeId);
    printf("[Files] route=%d count=%d", routeId, count);

    for (new i = 0; i < count; i++)
    {
        new name[128];
        if (FILE_GetName(routeId, i, name, sizeof(name)))
        {
            printf("  - %s", name);
        }
    }
}

public OnGameModeInit()
{
    REST_Start(8080);

    g_MapRoute = FILE_RegisterRoute("/maps", "scriptfiles/maps/", ".map,.json", 50);
    FILE_AddAuthKey(g_MapRoute, "upload-secret");
    FILE_SetConflict(g_MapRoute, CONFLICT_RENAME);
    FILE_SetCorruptAction(g_MapRoute, CORRUPT_QUARANTINE);
    FILE_SetRequireCRC32(g_MapRoute, true);

    // Enable REST API endpoints for file management:
    //   GET  /maps/files          -> list files (JSON: { success, count, files: ["a.map","b.json"] })
    //   GET  /maps/files/{name}   -> download file (raw binary)
    //   GET  /maps/files/{name}/info -> file metadata (JSON: { success, name, size, modified })
    //   DELETE /maps/files/{name} -> delete file (JSON: { success, deleted })
    FILE_AllowList(g_MapRoute, true);
    FILE_AllowDownload(g_MapRoute, true);
    FILE_AllowDelete(g_MapRoute, true);
    FILE_AllowInfo(g_MapRoute, true);
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
        if (FILE_GetName(g_MapRoute, 0, firstName, sizeof(firstName)))
        {
            if (FILE_Delete(g_MapRoute, firstName))
            {
                SendClientMessage(playerid, -1, "Deleted first file from /maps route.");
            }
        }
        return 1;
    }
    return 0;
}

public OnIncomingUploadCompleted(uploadId, routeId, const endpoint[], const filename[], const filepath[], const crc32[], crcMatched)
{
    printf("[Upload OK] id=%d route=%d endpoint=%s file=%s crc=%s match=%d", uploadId, routeId, endpoint, filename, crc32, crcMatched);
    return 1;
}

public OnIncomingUploadFailed(uploadId, const reason[], const crc32[])
{
    printf("[Upload FAIL] id=%d reason=%s crc=%s", uploadId, reason, crc32);
    return 1;
}

public OnIncomingUploadProgress(uploadId, percent)
{
    printf("[Upload PROGRESS] id=%d %d%%", uploadId, percent);
    return 1;
}
