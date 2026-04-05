# Use Cases

This page maps common production scenarios to concrete PawnREST APIs.

## 1. Admin Panel API (Inbound REST)

**Scenario**  
Expose server status and moderation actions to a web dashboard.

**Typical flow**
1. Register routes with `REST_Route`.
2. Protect sensitive endpoints with `REST_SetRouteAuth`.
3. Read input via `REST_GetParam*`, `REST_GetQuery*`, and `RequestJson`.
4. Return structured payloads with `RespondNode`.

**Relevant APIs**  
`REST_Route`, `REST_SetRouteAuth`, `REST_GetRequest*`, `RequestJson`, `RespondNode`

## 2. Managed File Distribution

**Scenario**  
Use game server as controlled upload/download hub for maps or patch data.

**Typical flow**
1. Register upload route using `REST_RegisterRoute`.
2. Set auth and integrity controls (`REST_AddKey`, `REST_SetRequireCRC32`).
3. Enable required REST file operations (`REST_AllowList`, `REST_AllowDownload`, etc.).
4. Observe inbound lifecycle with upload callbacks.

**Relevant APIs**  
`REST_RegisterRoute`, `REST_SetConflict`, `REST_SetCorruptAction`, `REST_Allow*`, `OnFileUploaded`

## 3. External Service Integration (Outbound HTTP)

**Scenario**  
Call remote auth/leaderboard/store APIs from gamemode events.

**Typical flow**
1. Create one reusable client per service using `REST_RequestsClient`.
2. Dispatch requests with `REST_Request` / `REST_RequestJSON`.
3. Handle per-request success callbacks and global failure callbacks.
4. Inspect structured status/error metadata as needed.

**Relevant APIs**  
`REST_RequestsClient`, `REST_Request`, `REST_RequestJSON`, `OnRequestFailureDetailed`, `REST_GetRequestErrorCode`

## 4. Real-Time Event Stream Consumption (WebSocket)

**Scenario**  
Subscribe to external real-time event channels (moderation, orchestration, notifications).

**Typical flow**
1. Connect via `REST_WebSocketClient` or `REST_JsonWebSocketClient`.
2. Process incoming data in callback.
3. React to disconnect events and reconnect with your own retry strategy.

**Relevant APIs**  
`REST_WebSocketClient`, `REST_JsonWebSocketClient`, `REST_JsonWebSocketSend`, `OnWebSocketDisconnect`

## 5. Outbound File Delivery

**Scenario**  
Push local files from server to an external storage or CI endpoint.

**Typical flow**
1. Queue upload with `REST_UploadFile` or `REST_UploadFileWithClient`.
2. Track progress/status (`REST_GetUploadProgress`, `REST_GetUploadStatus`).
3. Handle rich success/failure callbacks.

**Relevant APIs**  
`REST_CreateUploadClient`, `REST_UploadFileWithClient`, `OnFileUploadCompleted`, `OnFileUploadFailure`

## 6. Unified Integration Layer

**Scenario**  
Use PawnREST as a single networking layer inside gamemode.

**Typical flow**
1. Inbound REST for control-plane endpoints.
2. Outbound HTTP for service-to-service operations.
3. WebSocket for push/event streams.
4. JSON node API for one consistent payload model.

**Outcome**
- fewer external dependencies
- consistent callback and error model
- simpler operational maintenance

## 7. Production Hardening Pattern

For real deployments, pair the above use-cases with:

1. route-level auth (`REST_AddKey`, `REST_SetRouteAuth`)
2. strict upload constraints (extension, size, CRC32)
3. TLS-enabled build when using HTTPS/WSS
4. explicit handling for structured error callbacks
