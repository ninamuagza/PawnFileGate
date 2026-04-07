# Use Cases

This page maps common production scenarios to concrete PawnREST APIs.

## 1. Admin Panel API (Inbound REST)

**Scenario**  
Expose server status and moderation actions to a web dashboard.

**Typical flow**
1. Register routes with `REST_RegisterAPIRoute`.
2. Protect sensitive endpoints with `REST_SetRouteAuthKey`.
3. Read input via `REST_GetParam*`, `REST_GetQuery*`, and `GetRequestJsonNode`.
4. Return structured payloads with `RespondNode`.

**Relevant APIs**  
`REST_RegisterAPIRoute`, `REST_SetRouteAuthKey`, `REST_GetRequest*`, `GetRequestJsonNode`, `RespondNode`

## 2. Managed File Distribution

**Scenario**  
Use game server as controlled upload/download hub for maps or patch data.

**Typical flow**
1. Register upload route using `FILE_RegisterRoute`.
2. Set auth and integrity controls (`FILE_AddAuthKey`, `FILE_SetRequireCRC32`).
3. Enable required REST file operations (`FILE_AllowList`, `FILE_AllowDownload`, etc.).
4. Observe inbound lifecycle with upload callbacks.

**Relevant APIs**  
`FILE_RegisterRoute`, `FILE_SetConflict`, `FILE_SetCorruptAction`, `FILE_Allow*`, `OnIncomingUploadCompleted`

## 3. External Service Integration (Outbound HTTP)

**Scenario**  
Call remote auth/leaderboard/store APIs from gamemode events.

**Typical flow**
1. Create one reusable client per service using `REST_CreateRequestClient`.
2. Dispatch requests with `REST_Request` / `REST_RequestJSON`.
3. Handle per-request success callbacks and global failure callbacks.
4. Inspect structured status/error metadata as needed.

**Relevant APIs**  
`REST_CreateRequestClient`, `REST_Request`, `REST_RequestJSON`, `OnRequestFailure`, `REST_GetRequestErrorCode`

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
1. Queue upload with `FILE_Upload` or `FILE_UploadWithClient`.
2. Track progress/status (`FILE_GetUploadProgress`, `FILE_GetUploadStatus`).
3. Handle rich success/failure callbacks.

**Relevant APIs**  
`FILE_CreateUploadClient`, `FILE_UploadWithClient`, `OnOutgoingUploadCompleted`, `OnOutgoingUploadFailureDetailed`

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

1. route-level auth (`FILE_AddAuthKey`, `REST_SetRouteAuthKey`)
2. strict upload constraints (extension, size, CRC32)
3. TLS-enabled build when using HTTPS/WSS
4. explicit handling for structured error callbacks

## 8. Dataless Bot Gateway (Discord/Chat Bot)

**Scenario**  
Keep your bot/service layer stateless and let the open.mp server remain the single data authority.

**Typical flow**
1. Expose bot-only routes via `REST_RegisterAPIRoute` and protect them using `REST_SetRouteAuthKey`.
2. Read critical identifiers from path/query/header/body as needed (`REST_GetParam*`, `REST_GetQuery*`, `REST_GetHeader`, `GetRequestJsonNode`).
3. Return consistent error payloads (`RespondError`) for missing input, not-found, and unauthorized cases.
4. Keep bot-side logic focused on orchestration/UI while data access stays in gamemode callbacks.

**Relevant APIs**  
`REST_RegisterAPIRoute`, `REST_SetRouteAuthKey`, `REST_GetParam`, `REST_GetQuery`, `REST_GetHeader`, `GetRequestJsonNode`, `RespondNode`, `RespondError`

## 9. File Management via Bot/External Tools

**Scenario**  
Enable Discord bot or admin tools to list, upload, download, and delete files on the server without direct SSH access.

**Typical flow**
1. Register upload route with `FILE_RegisterRoute`.
2. Enable REST file ops: `FILE_AllowList`, `FILE_AllowDownload`, `FILE_AllowDelete`, `FILE_AllowInfo`.
3. Protect with `FILE_AddAuthKey`.
4. Bot/tool calls HTTP endpoints:
   - `GET {route}/files` — list files (returns `{ files: ["a.map", "b.map"] }`)
   - `GET {route}/files/{name}` — download file (raw binary)
   - `GET {route}/files/{name}/info` — file metadata
   - `DELETE {route}/files/{name}` — delete file
   - `POST {route}` — upload file (multipart/form-data)

**Client example (JavaScript/TypeScript):**
```js
// List files
const list = await fetch('http://server:8080/maps/files', {
  headers: { Authorization: 'Bearer secret-key' }
}).then(r => r.json());
// { success: true, count: 2, files: ["map1.map", "test.json"] }

// Download file
const data = await fetch('http://server:8080/maps/files/map1.map', {
  headers: { Authorization: 'Bearer secret-key' }
}).then(r => r.arrayBuffer());

// Upload file
const form = new FormData();
form.append('file', blob, 'newmap.map');
await fetch('http://server:8080/maps', {
  method: 'POST',
  headers: { Authorization: 'Bearer secret-key' },
  body: form
});

// Delete file
await fetch('http://server:8080/maps/files/oldmap.map', {
  method: 'DELETE',
  headers: { Authorization: 'Bearer secret-key' }
});
```

**Relevant APIs**  
`FILE_RegisterRoute`, `FILE_AddAuthKey`, `FILE_AllowList`, `FILE_AllowDownload`, `FILE_AllowDelete`, `FILE_AllowInfo`
