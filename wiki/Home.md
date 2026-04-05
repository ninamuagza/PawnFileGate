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

1. **Inbound API**: use `REST_Route` with Pawn callbacks.
2. **Outbound API**: use `REST_RequestsClient` with `REST_Request` / `REST_RequestJSON`.
3. **JSON**: use the node API (`JsonObject`, `JsonSet*`, `JsonGet*`, `RespondNode`).
