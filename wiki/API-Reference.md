# API Reference

Quick reference for the main API surface in `PawnREST.inc`.

## Constants

- HTTP method: `HTTP_GET`, `HTTP_POST`, `HTTP_PUT`, `HTTP_PATCH`, `HTTP_DELETE`, `HTTP_HEAD`, `HTTP_OPTIONS`
- Alias method: `HTTP_METHOD_*`
- Upload mode: `UPLOAD_MODE_MULTIPART`, `UPLOAD_MODE_RAW`
- Upload status: `UPLOAD_*`
- Request status: `REQUEST_*`
- JSON node type: `JSON_NODE_*` / `PAWNREST_NODE_*`
- Error code: `PAWNREST_ERR_*`

## Server Control

```pawn
native bool:PawnREST_Start(port);
native bool:PawnREST_StartTLS(port, const certPath[], const keyPath[]);
native bool:PawnREST_Stop();
native PawnREST_IsRunning();
native PawnREST_GetPort();
native PawnREST_IsTLSEnabled();
```

## Inbound Upload Routes

```pawn
native PawnREST_RegisterRoute(const endpoint[], const path[], const allowedExts[], maxSizeMb);
native bool:PawnREST_AddKey(routeId, const key[]);
native bool:PawnREST_RemoveKey(routeId, const key[]);
native bool:PawnREST_SetConflict(routeId, mode);
native bool:PawnREST_SetCorruptAction(routeId, action);
native bool:PawnREST_SetRequireCRC32(routeId, bool:required);
native bool:PawnREST_RemoveRoute(routeId);
```

## REST API Routes

```pawn
native PawnREST_Route(method, const endpoint[], const callback[]);
native bool:PawnREST_RemoveAPIRoute(routeId);
native bool:PawnREST_SetRouteAuth(routeId, const key[]);
```

## Request Data Access

```pawn
native PawnREST_GetRequestIP(requestId, output[], outputSize);
native PawnREST_GetRequestMethod(requestId);
native PawnREST_GetRequestPath(requestId, output[], outputSize);
native PawnREST_GetRequestBody(requestId, output[], outputSize);
native PawnREST_GetRequestBodyLength(requestId);
native PawnREST_GetParam(requestId, const name[], output[], outputSize);
native PawnREST_GetParamInt(requestId, const name[]);
native PawnREST_GetQuery(requestId, const name[], output[], outputSize);
native PawnREST_GetQueryInt(requestId, const name[], defaultValue = 0);
native PawnREST_GetHeader(requestId, const name[], output[], outputSize);
```

## JSON + Response (Concise API)

```pawn
native JsonParse(const json[]);
native RequestJson(requestId);
native JsonNodeType(nodeId);
native JsonStringify(nodeId, output[], outputSize);
native bool:JsonCleanup(nodeId);

native JsonObject(...);
native JsonArray(...);
native JsonString(const value[]);
native JsonInt(value);
native JsonFloat(Float:value);
native JsonBool(bool:value);
native JsonNull();
native JsonAppend(leftNodeId, rightNodeId);

native bool:JsonSetObject(objectNodeId, const key[], valueNodeId);
native bool:JsonSetString(objectNodeId, const key[], const value[]);
native bool:JsonSetInt(objectNodeId, const key[], value);
native bool:JsonSetFloat(objectNodeId, const key[], Float:value);
native bool:JsonSetBool(objectNodeId, const key[], bool:value);
native bool:JsonSetNull(objectNodeId, const key[]);
native bool:JsonHas(objectNodeId, const key[]);
native JsonGetObject(objectNodeId, const key[]);
native JsonGetString(objectNodeId, const key[], output[], outputSize);
native JsonGetInt(objectNodeId, const key[], defaultValue = 0);
native Float:JsonGetFloat(objectNodeId, const key[], Float:defaultValue = 0.0);
native bool:JsonGetBool(objectNodeId, const key[], bool:defaultValue = false);

native JsonArrayLength(arrayNodeId);
native JsonArrayObject(arrayNodeId, index);
native bool:JsonArrayAppend(arrayNodeId, valueNodeId);
native bool:JsonArrayAppendString(arrayNodeId, const value[]);
native bool:JsonArrayAppendInt(arrayNodeId, value);
native bool:JsonArrayAppendFloat(arrayNodeId, Float:value);
native bool:JsonArrayAppendBool(arrayNodeId, bool:value);
native bool:JsonArrayAppendNull(arrayNodeId);

native bool:Respond(requestId, status, const body[], const contentType[] = "application/json");
native bool:RespondJSON(requestId, status, const json[]);
native bool:RespondNode(requestId, status, nodeId);
native bool:RespondError(requestId, status, const message[]);
native bool:SetResponseHeader(requestId, const name[], const value[]);
```

## File Route REST Permissions + File Ops

```pawn
native bool:PawnREST_AllowList(routeId, bool:allow);
native bool:PawnREST_AllowDownload(routeId, bool:allow);
native bool:PawnREST_AllowDelete(routeId, bool:allow);
native bool:PawnREST_AllowInfo(routeId, bool:allow);

native PawnREST_GetFileCount(routeId);
native PawnREST_GetFileName(routeId, index, output[], outputSize);
native bool:PawnREST_DeleteFile(routeId, const filename[]);
native PawnREST_GetFileSize(routeId, const filename[]);
```

## Outgoing File Upload API

```pawn
native PawnREST_UploadFile(const url[], const filepath[], const filename[] = "", const authKey[] = "", const customHeaders[] = "", calculateCrc32 = 1, mode = UPLOAD_MODE_MULTIPART, bool:verifyTls = true);
native PawnREST_CreateUploadClient(const baseUrl[], const defaultHeaders[] = "", bool:verifyTls = true);
native bool:PawnREST_RemoveUploadClient(clientId);
native bool:PawnREST_SetUploadClientHeader(clientId, const name[], const value[]);
native bool:PawnREST_RemoveUploadClientHeader(clientId, const name[]);
native PawnREST_UploadFileWithClient(clientId, const path[], const filepath[], const filename[] = "", const authKey[] = "", const customHeaders[] = "", calculateCrc32 = 1, mode = UPLOAD_MODE_MULTIPART);
native bool:PawnREST_CancelUpload(uploadId);
native PawnREST_GetUploadStatus(uploadId);
native PawnREST_GetUploadProgress(uploadId);
native PawnREST_GetUploadResponse(uploadId, output[], outputSize);
native PawnREST_GetUploadErrorCode(uploadId);
native PawnREST_GetUploadErrorType(uploadId, output[], outputSize);
native PawnREST_GetUploadHttpStatus(uploadId);
```

## Outbound HTTP API (Requests-style)

```pawn
native RequestsClient(const endpoint[], const defaultHeaders[] = "", bool:verifyTls = true);
native bool:RemoveRequestsClient(clientId);
native bool:SetRequestsClientHeader(clientId, const name[], const value[]);
native bool:RemoveRequestsClientHeader(clientId, const name[]);

native Request(clientId, const path[], method, const callback[], const body[] = "", const headers[] = "");
native RequestJSON(clientId, const path[], method, const callback[], jsonNodeId = -1, const headers[] = "");

native bool:CancelRequest(requestId);
native RequestStatus(requestId);
native RequestHTTPStatus(requestId);
native RequestErrorCode(requestId);
native RequestErrorType(requestId, output[], outputSize);
native RequestResponse(requestId, output[], outputSize);
```

## WebSocket Client API

```pawn
native WebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);
native JsonWebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);
native bool:WebSocketSend(socketId, const data[]);
native bool:JsonWebSocketSend(socketId, nodeId);
native bool:WebSocketClose(socketId, status = 1000, const reason[] = "");
native bool:RemoveWebSocketClient(socketId);
native bool:IsWebSocketOpen(socketId);
```

## Helpers

```pawn
stock bool:RequestHeaders(output[], outputSize, const key[], const value[]);
stock bool:RequestHeadersAppend(headers[], outputSize, const key[], const value[]);
```

## CRC32 Utilities

```pawn
native PawnREST_VerifyCRC32(const filepath[], const expectedCrc[]);
native PawnREST_GetFileCRC32(const filepath[], output[], outputSize);
native PawnREST_CompareFiles(const path1[], const path2[]);
```
