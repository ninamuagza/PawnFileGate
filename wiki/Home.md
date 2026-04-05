# PawnREST Wiki

Official documentation for PawnREST: an open.mp/SA-MP component that provides inbound HTTP routes, outbound HTTP/WebSocket clients, and a node-based JSON API for Pawn scripts.

## Documentation Index

| Page | Scope |
| --- | --- |
| [Getting Started](Getting-Started) | Install, bootstrap, and first working routes |
| [API Reference](API-Reference) | Complete native/constant reference from `PawnREST.inc` |
| [JSON Node Guide](JSON-Node-Guide) | JSON lifecycle, ownership, and composition patterns |
| [Outbound HTTP Guide](Outbound-HTTP-Guide) | Request client setup, callbacks, and failure handling |
| [WebSocket Guide](WebSocket-Guide) | Text/JSON websocket client usage and lifecycle |
| [Callbacks](Callbacks) | Callback signatures and runtime behavior |
| [Use Cases](Use-Cases) | Practical deployment patterns and integration ideas |

## Capability Matrix

| Capability | Supported |
| --- | --- |
| Inbound HTTP file upload routes | ✅ |
| Inbound custom REST endpoints | ✅ |
| Outbound HTTP text/JSON requests | ✅ |
| Outbound file uploads | ✅ |
| Outbound websocket client (text/json) | ✅ |
| Node-based JSON API | ✅ |
| TLS/HTTPS and WSS (build-dependent) | ✅ |
| WebSocket server mode | ❌ |

## API Conventions

1. Public native prefix is `REST_*`.
2. JSON node API uses concise `Json*` and `Respond*` helpers.
3. Outbound HTTP/websocket flows are asynchronous and callback-driven.
4. Route authentication uses `Authorization: Bearer <token>`.
5. Structured error codes use `PAWNREST_ERR_*`.

## Architecture Summary

1. **Inbound layer**: `REST_Start` + `REST_RegisterRoute` / `REST_Route`.
2. **Data layer**: request helpers (`REST_GetRequest*`) + node JSON API (`Json*`).
3. **Outbound layer**: `REST_RequestsClient`, `REST_Request*`, `REST_WebSocket*`.
4. **Integration layer**: callbacks for upload, request, and websocket events.
