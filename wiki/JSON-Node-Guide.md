# JSON Node Guide

PawnREST uses a handle-based JSON node API. This model is designed for predictable memory ownership and composable JSON construction in Pawn.

## Core Lifecycle

1. Acquire a node handle (`RequestJson`, `JsonParse`, `JsonObject`, `JsonArray`, `JsonGetObject`, `JsonArrayObject`).
2. Read or mutate data.
3. Serialize (`JsonStringify`) or respond (`RespondNode`).
4. Release handles with `JsonCleanup` when no longer needed.

If a function fails, it usually returns `-1` (node id) or `false` (bool).

## Ownership Rules (Important)

| Operation | Ownership behavior |
| --- | --- |
| `RequestJson`, `JsonParse`, `JsonObject`, `JsonArray`, `JsonGetObject`, `JsonArrayObject` | Returns a handle you must `JsonCleanup` |
| `JsonSetObject`, `JsonArrayAppend` | Clones/copies the input node into target; source handle can still be cleaned up |
| `JsonAppend(left, right)` | Consumes `left` and `right` handles internally and returns a new merged handle |

## Parse Request Body

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

    new out = JsonObject(
        "ok", JsonBool(true),
        "name", JsonString(name)
    );
    RespondNode(requestId, 201, out);
    JsonCleanup(out);
    return 1;
}
```

## Build Object and Array Nodes

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

## Variadic Builder Style

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

## Merge Objects/Arrays with `JsonAppend`

```pawn
new base = JsonObject("ok", JsonBool(true));
new extra = JsonObject("source", JsonString("api"));

new merged = JsonAppend(base, extra); // base and extra are consumed
if (merged == -1)
{
    RespondError(requestId, 500, "JsonAppend failed");
    return 1;
}

RespondNode(requestId, 200, merged);
JsonCleanup(merged);
```

## Access Child Nodes

```pawn
new body = RequestJson(requestId);
if (body == -1) return 1;

new profile = JsonGetObject(body, "profile");
if (profile != -1)
{
    new nickname[24];
    JsonGetString(profile, "nickname", nickname, sizeof(nickname));
    JsonCleanup(profile);
}

JsonCleanup(body);
```

## Type-Aware Reads

Use typed getters to avoid manual parsing:

- `JsonGetString`
- `JsonGetInt`
- `JsonGetFloat`
- `JsonGetBool`

Use defaults for missing or mismatched fields:

```pawn
new page = JsonGetInt(nodeId, "page", 1);
new bool:ok = JsonGetBool(nodeId, "ok", false);
```

## Practical Guidelines

1. Always release handles returned by node-producing functions.
2. Validate `RequestJson` and child-node lookups (`-1`) before reading.
3. Prefer `RespondNode` for structured output and `RespondJSON` for simple static JSON.
4. Keep JSON composition close to your endpoint logic to reduce ownership mistakes.
