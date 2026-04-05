#include <a_samp>
#include <PawnREST>

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/crc", true))
    {
        new crc[16];
        if (REST_GetFileCRC32("scriptfiles/test.bin", crc, sizeof(crc)))
        {
            new msg[96];
            format(msg, sizeof(msg), "CRC32(scriptfiles/test.bin) = %s", crc);
            SendClientMessage(playerid, -1, msg);
        }
        else
        {
            SendClientMessage(playerid, 0xFF0000FF, "Failed to calculate CRC32.");
        }
        return 1;
    }

    if (!strcmp(cmdtext, "/crcverify", true))
    {
        new result = REST_VerifyCRC32("scriptfiles/test.bin", "DEADBEEF");
        new msg[96];
        format(msg, sizeof(msg), "Verify result = %d (1 match, 0 mismatch, -1 error)", result);
        SendClientMessage(playerid, -1, msg);
        return 1;
    }

    if (!strcmp(cmdtext, "/crccompare", true))
    {
        new result = REST_CompareFiles("scriptfiles/a.bin", "scriptfiles/b.bin");
        new msg[96];
        format(msg, sizeof(msg), "Compare result = %d (1 same, 0 different, -1 error)", result);
        SendClientMessage(playerid, -1, msg);
        return 1;
    }
    return 0;
}
