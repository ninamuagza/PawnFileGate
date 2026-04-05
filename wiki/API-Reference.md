# API Reference

Quick reference for the main API surface in `PawnREST.inc`.

Use `REST_*` for core server, route, upload, outbound request, and websocket functions.

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
native bool:REST_Start(port);
native bool:REST_StartTLS(port, const certPath[], const keyPath[]);
native bool:REST_Stop();
native REST_IsRunning();
native REST_GetPort();
native REST_IsTLSEnabled();
```

## Inbound Upload Routes

```pawn
native REST_RegisterRoute(const endpoint[], const path[], const allowedExts[], maxSizeMb);
native bool:REST_AddKey(routeId, const key[]);
native bool:REST_RemoveKey(routeId, const key[]);
native bool:REST_SetConflict(routeId, mode);
native bool:REST_SetCorruptAction(routeId, action);
native bool:REST_SetRequireCRC32(routeId, bool:required);
native bool:REST_RemoveRoute(routeId);
```

## REST API Routes

```pawn
native REST_Route(method, const endpoint[], const callback[]);
native bool:REST_RemoveAPIRoute(routeId);
native bool:REST_SetRouteAuth(routeId, const key[]);
```

## Request Data Access

```pawn
native REST_GetRequestIP(requestId, output[], outputSize);
native REST_GetRequestMethod(requestId);
native REST_GetRequestPath(requestId, output[], outputSize);
native REST_GetRequestBody(requestId, output[], outputSize);
native REST_GetRequestBodyLength(requestId);
native REST_GetParam(requestId, const name[], output[], outputSize);
native REST_GetParamInt(requestId, const name[]);
native REST_GetQuery(requestId, const name[], output[], outputSize);
native REST_GetQueryInt(requestId, const name[], defaultValue = 0);
native REST_GetHeader(requestId, const name[], output[], outputSize);
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
native bool:REST_AllowList(routeId, bool:allow);
native bool:REST_AllowDownload(routeId, bool:allow);
native bool:REST_AllowDelete(routeId, bool:allow);
native bool:REST_AllowInfo(routeId, bool:allow);

native REST_GetFileCount(routeId);
native REST_GetFileName(routeId, index, output[], outputSize);
native bool:REST_DeleteFile(routeId, const filename[]);
native REST_GetFileSize(routeId, const filename[]);
```

## Outgoing File Upload API

```pawn
native REST_UploadFile(const url[], const filepath[], const filename[] = "", const authKey[] = "", const customHeaders[] = "", calculateCrc32 = 1, mode = UPLOAD_MODE_MULTIPART, bool:verifyTls = true);
native REST_CreateUploadClient(const baseUrl[], const defaultHeaders[] = "", bool:verifyTls = true);
native bool:REST_RemoveUploadClient(clientId);
native bool:REST_SetUploadClientHeader(clientId, const name[], const value[]);
native bool:REST_RemoveUploadClientHeader(clientId, const name[]);
native REST_UploadFileWithClient(clientId, const path[], const filepath[], const filename[] = "", const authKey[] = "", const customHeaders[] = "", calculateCrc32 = 1, mode = UPLOAD_MODE_MULTIPART);
native bool:REST_CancelUpload(uploadId);
native REST_GetUploadStatus(uploadId);
native REST_GetUploadProgress(uploadId);
native REST_GetUploadResponse(uploadId, output[], outputSize);
native REST_GetUploadErrorCode(uploadId);
native REST_GetUploadErrorType(uploadId, output[], outputSize);
native REST_GetUploadHttpStatus(uploadId);
```

## Outbound HTTP API

```pawn
native REST_RequestsClient(const endpoint[], const defaultHeaders[] = "", bool:verifyTls = true);
native bool:REST_RemoveRequestsClient(clientId);
native bool:REST_SetRequestsClientHeader(clientId, const name[], const value[]);
native bool:REST_RemoveRequestsClientHeader(clientId, const name[]);

native REST_Request(clientId, const path[], method, const callback[], const body[] = "", const headers[] = "");
native REST_RequestJSON(clientId, const path[], method, const callback[], jsonNodeId = -1, const headers[] = "");

native bool:REST_CancelRequest(requestId);
native REST_GetRequestStatus(requestId);
native REST_GetRequestHttpStatus(requestId);
native REST_GetRequestErrorCode(requestId);
native REST_GetRequestErrorType(requestId, output[], outputSize);
native REST_GetRequestResponse(requestId, output[], outputSize);
```

## WebSocket Client API

```pawn
native REST_WebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);
native REST_JsonWebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);
native bool:REST_WebSocketSend(socketId, const data[]);
native bool:REST_JsonWebSocketSend(socketId, nodeId);
native bool:REST_WebSocketClose(socketId, status = 1000, const reason[] = "");
native bool:REST_RemoveWebSocketClient(socketId);
native bool:REST_IsWebSocketOpen(socketId);
```

## Helpers

```pawn
stock bool:REST_RequestHeaders(output[], outputSize, const key[], const value[]);
stock bool:REST_RequestHeadersAppend(headers[], outputSize, const key[], const value[]);
```

## CRC32 Utilities

```pawn
native REST_VerifyCRC32(const filepath[], const expectedCrc[]);
native REST_GetFileCRC32(const filepath[], output[], outputSize);
native REST_CompareFiles(const path1[], const path2[]);
```
