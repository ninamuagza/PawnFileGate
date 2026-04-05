# Callbacks

## Incoming Upload

```pawn
forward OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], const filepath[], const crc32[], crcMatched);
forward OnFileFailedUpload(uploadId, const reason[], const crc32[]);
forward OnUploadProgress(uploadId, percent);
```

## Outgoing Upload

```pawn
forward OnFileUploadStarted(uploadId);
forward OnFileUploadProgress(uploadId, percent);
forward OnFileUploadCompleted(uploadId, httpStatus, const responseBody[], const crc32[]);
forward OnFileUploadFailed(uploadId, const errorMessage[]);
forward OnFileUploadFailure(uploadId, errorCode, const errorType[], const errorMessage[], httpStatus);
```

## Outbound HTTP Request

Per-request callback from `Request`:

```pawn
public YourTextCallback(requestId, httpStatus, const data[], dataLen)
```

Per-request callback from `RequestJSON`:

```pawn
public YourJsonCallback(requestId, httpStatus, nodeId)
```

Global failure callbacks:

```pawn
forward OnRequestFailure(requestId, errorCode, const errorMessage[], len);
forward OnRequestFailureDetailed(requestId, errorCode, const errorType[], const errorMessage[], httpStatus);
```

## WebSocket

Text websocket callback:

```pawn
public YourSocketTextCallback(socketId, const data[], dataLen)
```

JSON websocket callback:

```pawn
public YourSocketJsonCallback(socketId, nodeId)
```

Disconnect callback:

```pawn
forward OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode);
```
