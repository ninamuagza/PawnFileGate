# JSON Node Guide

PawnREST now uses a **node-only JSON** model.

## Quick lifecycle

1. Get a node (`RequestJson`, `JsonParse`, `JsonObject`, `JsonArray`, etc.).
2. Read/update the node.
3. Send it with (`RespondNode`) or stringify it (`JsonStringify`).
4. Release the handle with `JsonCleanup`.

## Parse request body

```pawn
public API_CreateUser(requestId)
{
    new body = RequestJson(requestId);
    if (body == -1)
    {
        RespondError(requestId, 400, "Invalid JSON");
        return 1;
    }

    new name[32];
    JsonGetString(body, "name", name, sizeof(name));
    JsonCleanup(body);

    new out = JsonObject();
    JsonSetBool(out, "ok", true);
    JsonSetString(out, "name", name);
    RespondNode(requestId, 201, out);
    JsonCleanup(out);
    return 1;
}
```

## Build nested JSON

```pawn
new payload = JsonObject();
new players = JsonArray();

for (new i = 0; i < MAX_PLAYERS; i++)
{
    if (!IsPlayerConnected(i)) continue;

    new entry = JsonObject();
    JsonSetInt(entry, "id", i);
    JsonSetInt(entry, "score", GetPlayerScore(i));
    JsonArrayAppend(players, entry);
    JsonCleanup(entry);
}

JsonSetObject(payload, "players", players);
JsonCleanup(players);
RespondNode(requestId, 200, payload);
JsonCleanup(payload);
```

## Builder style (more ergonomic)

```pawn
new payload = JsonObject(
    "name", JsonString("PawnREST"),
    "online", JsonInt(GetPlayerPoolSize()),
    "players", JsonArray(
        JsonObject("id", JsonInt(0), "name", JsonString("Alice")),
        JsonObject("id", JsonInt(1), "name", JsonString("Bob"))
    )
);

RespondNode(requestId, 200, payload);
JsonCleanup(payload);
```

## Read object/array child

```pawn
new body = RequestJson(requestId);
new profile = JsonGetObject(body, "profile");
if (profile != -1)
{
    new nickname[24];
    JsonGetString(profile, "nickname", nickname, sizeof(nickname));
    JsonCleanup(profile);
}
JsonCleanup(body);
```

## Best practices

- Always check `-1` for failed parse/get object operations.
- Always call `JsonCleanup` for node handles that are no longer needed.
- For simple responses, `RespondJSON` is fine; for complex responses, use `RespondNode`.
