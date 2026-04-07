# Callbacks

PawnREST uses callback-driven flow for upload events, outbound requests, and websocket clients.

## Behavior Notes

1. Declare callback signatures in your Pawn code (typically as `forward` for global callbacks).
2. Per-request callbacks are named dynamically when you call `REST_Request`, `REST_RequestJSON`, `REST_WebSocketClient`, or `REST_JsonWebSocketClient`.
3. In most gamemode setups, callbacks should return `1`.

## Incoming Upload Callbacks

### Fired on successful inbound upload

```pawn
forward OnIncomingUploadCompleted(
    uploadId,
    routeId,
    const endpoint[],
    const filename[],
    const filepath[],
    const crc32[],
    crcMatched
);
```

### Fired on inbound upload failure

```pawn
forward OnIncomingUploadFailed(uploadId, const reason[], const crc32[]);
```

### Fired during inbound upload progress

```pawn
forward OnIncomingUploadProgress(uploadId, percent);
```

## Outgoing Upload Callbacks

```pawn
forward OnOutgoingUploadStarted(uploadId);
forward OnOutgoingUploadProgress(uploadId, percent);
forward OnOutgoingUploadCompleted(uploadId, httpStatus, const responseBody[], const crc32[]);
forward OnOutgoingUploadFailed(uploadId, errorCode, const errorType[], const errorMessage[], httpStatus);
```

`OnOutgoingUploadFailureDetailed` is the structured variant with typed metadata (`PAWNREST_ERR_*`, error type string, and HTTP status when available).

## Outbound HTTP Request Callbacks

### Per-request text callback (`REST_Request`)

```pawn
public YourTextCallback(requestId, httpStatus, const data[], dataLen)
```

### Per-request JSON callback (`REST_RequestJSON`)

```pawn
public YourJsonCallback(requestId, httpStatus, nodeId)
```

### Global failure callbacks

```pawn
forward OnRequestFailure(requestId, errorCode, const errorType[], const errorMessage[], httpStatus);
```

## WebSocket Callbacks

### Per-socket text callback

```pawn
public YourSocketTextCallback(socketId, const data[], dataLen)
```

### Per-socket JSON callback

```pawn
public YourSocketJsonCallback(socketId, nodeId)
```

### Global disconnect callback

```pawn
forward OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode);
```

`status` is the websocket close code; `errorCode` maps to `PAWNREST_ERR_*` for transport/parser failures.
