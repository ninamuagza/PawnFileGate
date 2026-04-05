# Migration from pawn-requests

This document helps you migrate from `Southclaws/pawn-requests` to PawnREST.

## Quick mapping

| pawn-requests | PawnREST |
|---|---|
| `#include <requests>` | `#include <PawnREST>` |
| `RequestsClient(...)` | `RequestsClient(...)` |
| `Request(...)` | `Request(...)` |
| `RequestJSON(...)` | `RequestJSON(...)` |
| `OnRequestFailure(...)` | `OnRequestFailure(...)` |
| `JsonObject/JsonArray/JsonGet*/JsonSet*` | `JsonObject/JsonArray/JsonGet*/JsonSet*` |
| `WebSocketClient(...)` | `WebSocketClient(...)` |
| `JsonWebSocketClient(...)` | `JsonWebSocketClient(...)` |

## Key differences

1. **Handle types**
   - pawn-requests commonly uses tags (`Request:`, `Node:`).
   - PawnREST uses regular integer handles.

2. **Server inbound**
   - pawn-requests focuses on outbound features.
   - PawnREST includes an inbound REST server + upload routes.

3. **Error detail**
   - PawnREST adds `OnRequestFailureDetailed` for richer metadata.

4. **WebSocket server**
   - PawnREST currently focuses on **WebSocket clients** (no WS server yet).

5. **JSON builder style**
   - PawnREST supports inline builder style similar to pawn-requests:
     - `JsonObject("k", JsonString("v"), "n", JsonInt(1))`
     - `JsonArray(JsonInt(1), JsonInt(2), JsonInt(3))`

## Before/after example

### Before (pawn-requests)

```pawn
new RequestsClient:api = RequestsClient("https://api.example.com");
RequestJSON(api, "/users/1", HTTP_METHOD_GET, "OnUser");

public OnUser(Request:id, E_HTTP_STATUS:status, Node:node)
{
    new name[24];
    JsonGetString(node, "name", name);
    return 1;
}
```

### After (PawnREST)

```pawn
new api = RequestsClient("https://api.example.com");
RequestJSON(api, "/users/1", HTTP_METHOD_GET, "OnUser");

public OnUser(requestId, httpStatus, nodeId)
{
    new name[24];
    JsonGetString(nodeId, "name", name, sizeof(name));
    JsonCleanup(nodeId);
    return 1;
}
```

## Migration checklist

- [ ] Replace include.
- [ ] Align request/websocket callback signatures.
- [ ] Ensure every callback `nodeId` is cleaned up (`JsonCleanup`) when no longer needed.
- [ ] Verify error handlers (`OnRequestFailure` + `OnRequestFailureDetailed`).
