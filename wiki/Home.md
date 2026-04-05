# PawnREST Wiki

Structured documentation for PawnREST APIs and server-client implementation use cases.

## Start here

- [Getting Started](./Getting-Started.md)
- [API Reference](./API-Reference.md)
- [JSON Node Guide](./JSON-Node-Guide.md)
- [Outbound HTTP Guide](./Outbound-HTTP-Guide.md)
- [WebSocket Guide](./WebSocket-Guide.md)
- [Callbacks](./Callbacks.md)
- [Use Cases](./Use-Cases.md)
- [Migration from pawn-requests](./Migration-from-pawn-requests.md)

## Capability Snapshot

| Capability | PawnREST |
|---|---|
| Inbound HTTP REST Server | ✅ |
| Inbound File Upload Server | ✅ |
| Outbound HTTP Text/JSON Request | ✅ |
| Outbound File Upload | ✅ |
| WebSocket Client (Text/JSON) | ✅ |
| WebSocket Server | ❌ |
| Node-based JSON API | ✅ |
| TLS/HTTPS & WSS (build dependent) | ✅ |

## Usage Philosophy

1. **Inbound API**: use `PawnREST_Route` with Pawn callbacks.
2. **Outbound API**: use `RequestsClient` with `Request` / `RequestJSON`.
3. **JSON**: use the node API (`JsonObject`, `JsonSet*`, `JsonGet*`, `RespondNode`).
