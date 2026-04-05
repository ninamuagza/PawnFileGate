# Outbound HTTP Guide (Requests-style)

This guide focuses on outbound APIs with usage patterns similar to `pawn-requests`.

## 1. Create a client

```pawn
new g_API;

public OnGameModeInit()
{
    new headers[256];
    RequestHeaders(headers, sizeof(headers), "Authorization", "Bearer token-123");
    RequestHeadersAppend(headers, sizeof(headers), "X-Server", "my-openmp");

    g_API = RequestsClient("https://api.example.com", headers, true);
    return 1;
}
```

## 2. Send a text request

```pawn
Request(g_API, "/status", HTTP_METHOD_GET, "OnStatus");

public OnStatus(requestId, httpStatus, const data[], dataLen)
{
    printf("request=%d status=%d body=%s", requestId, httpStatus, data);
    return 1;
}
```

## 3. Send a JSON request

```pawn
new payload = JsonObject();
JsonSetString(payload, "event", "player_join");
JsonSetInt(payload, "playerId", 7);

RequestJSON(g_API, "/events", HTTP_METHOD_POST, "OnEventPosted", payload);
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

## 4. Error callbacks

```pawn
forward OnRequestFailure(requestId, errorCode, const errorMessage[], len);
forward OnRequestFailureDetailed(requestId, errorCode, const errorType[], const errorMessage[], httpStatus);
```

Use `errorCode` (`PAWNREST_ERR_*`) for quick classification:
- timeout/network/tls
- invalid url / unsupported scheme
- json parse error (for `RequestJSON`)

## 5. Optional status polling

```pawn
new status = RequestStatus(requestId);      // REQUEST_*
new code = RequestErrorCode(requestId);
new http = RequestHTTPStatus(requestId);
```

## 6. Header format

`Request` / `RequestJSON` accepts a header string with this format:

```text
Key: Value|Key2: Value2|Key3: Value3
```
