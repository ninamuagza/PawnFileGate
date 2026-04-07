# Outbound HTTP Guide

Use this guide to integrate external HTTP services (auth, leaderboard, billing, telemetry, etc.) from Pawn.

## 1. Create a Reusable Client

```pawn
new g_API = -1;

public OnGameModeInit()
{
    new headers[256];
    REST_RequestHeaders(headers, sizeof(headers), "Authorization", "Bearer token-123");
    REST_RequestHeadersAppend(headers, sizeof(headers), "X-Server", "my-openmp");

    g_API = REST_CreateRequestClient("https://api.example.com", headers, true);
    REST_SetRequestsClientHeader(g_API, "User-Agent", "PawnREST/1.0");
    return 1;
}
```

## 2. Send a Text Request

```pawn
new requestId = REST_Request(g_API, "/status", HTTP_METHOD_GET, "OnStatus");

public OnStatus(requestId, httpStatus, const data[], dataLen)
{
    printf("request=%d status=%d len=%d body=%s", requestId, httpStatus, dataLen, data);
    return 1;
}
```

Callback signature for `REST_Request`:

```pawn
public YourTextCallback(requestId, httpStatus, const data[], dataLen)
```

## 3. Send a JSON Request

```pawn
new payload = JsonObject(
    "event", JsonString("player_join"),
    "playerId", JsonInt(7)
);

REST_RequestJSON(g_API, "/events", HTTP_METHOD_POST, "OnEventPosted", payload);
JsonCleanup(payload);

public OnEventPosted(requestId, httpStatus, nodeId)
{
    if (nodeId != -1)
    {
        new ok = JsonGetInt(nodeId, "ok", 0);
        printf("status=%d ok=%d", httpStatus, ok);
        JsonCleanup(nodeId);
    }
    return 1;
}
```

Callback signature for `REST_RequestJSON`:

```pawn
public YourJsonCallback(requestId, httpStatus, nodeId)
```

## 4. Error Callbacks

Transport/internal failures are emitted globally:

```pawn
forward OnRequestFailure(requestId, errorCode, const errorType[], const errorMessage[], httpStatus);
```

Typical `PAWNREST_ERR_*` categories:

- invalid URL or unsupported scheme
- TLS and certificate errors
- timeout/network failures
- JSON parse failures in JSON request/response path

## 5. Request State and Cancellation

```pawn
new bool:cancelled = REST_CancelRequest(requestId);
new status = REST_GetRequestStatus(requestId);          // REQUEST_*
new httpStatus = REST_GetRequestHttpStatus(requestId);
new errorCode = REST_GetRequestErrorCode(requestId);    // PAWNREST_ERR_*
```

For completed requests, you can also retrieve the raw response body:

```pawn
new buffer[1024];
if (REST_GetRequestResponse(requestId, buffer, sizeof(buffer)))
{
    printf("cached-response=%s", buffer);
}
```

## 6. Per-Request Extra Headers

`REST_Request` and `REST_RequestJSON` accept `headers` as:

```text
Key: Value|Key2: Value2|Key3: Value3
```

Use helper stocks when possible:

```pawn
new headers[256];
REST_RequestHeaders(headers, sizeof(headers), "X-Trace-Id", "abc-123");
REST_RequestHeadersAppend(headers, sizeof(headers), "X-Region", "ap-southeast");
```

## 7. Cleanup

```pawn
public OnGameModeExit()
{
    if (g_API != -1)
    {
        REST_RemoveRequestsClient(g_API);
    }
    return 1;
}
```

## 8. Operational Recommendations

1. Keep one client per external service and reuse it.
2. Use short callback names scoped by domain (`OnAuthResponse`, `OnBillingResponse`, etc.).
3. Always implement `OnRequestFailure` in production.
4. Prefer JSON request/response for typed payloads and future compatibility.
