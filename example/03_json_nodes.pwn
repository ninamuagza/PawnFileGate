#include <open.mp>
#include <PawnREST>

public OnGameModeInit()
{
    REST_Start(8080);
    REST_RegisterAPIRoute(HTTP_METHOD_POST, "/api/json-test", "API_JsonTest");

    new parsed = JsonParse("{\"server\":\"PawnREST\",\"online\":42}");
    if (parsed != -1)
    {
        new out[256];
        JsonStringify(parsed, out, sizeof(out));
        printf("[Parsed JSON] %s", out);
        JsonCleanup(parsed);
    }
    return 1;
}

public API_JsonTest(requestId)
{
    new body = GetRequestJsonNode(requestId);
    if (body == -1)
    {
        RespondError(requestId, 400, "Invalid JSON");
        return 1;
    }

    new playerName[24];
    JsonGetString(body, "name", playerName, sizeof(playerName));
    new score = JsonGetInt(body, "score", 0);
    JsonCleanup(body);

    new left = JsonObject(
        "name", JsonString(playerName),
        "score", JsonInt(score)
    );

    new right = JsonObject(
        "ok", JsonBool(true),
        "source", JsonString("json-nodes-example")
    );

    new payload = JsonAppend(left, right); // left/right consumed by JsonAppend
    if (payload == -1)
    {
        RespondError(requestId, 500, "JsonAppend failed");
        return 1;
    }

    new players = JsonArray(
        JsonObject("id", JsonInt(0), "name", JsonString("Alice")),
        JsonObject("id", JsonInt(1), "name", JsonString("Bob"))
    );
    JsonSetObject(payload, "players", players);
    JsonCleanup(players);

    RespondNode(requestId, 200, payload);
    JsonCleanup(payload);
    return 1;
}
