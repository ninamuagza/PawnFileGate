# Use Cases

## 1. Admin Dashboard (Web + REST)

**Flow**
1. Dashboard call `GET /api/players`.
2. PawnREST route callback reads game data.
3. Return the response using `RespondNode`.

**Good for**
- player monitoring
- kick/ban tools
- server settings panel

## 2. File Distribution Hub

**Flow**
1. Client uploads files to route `/maps`.
2. Validate extension + size + optional CRC32.
3. Route exposes `GET /maps/files` for listing/download.

**Good for**
- map/mod pack distribution
- patch file management

## 3. External API integration (Outbound HTTP)

**Flow**
1. Create `RequestsClient("https://api.example.com")`.
2. Send `Request` / `RequestJSON`.
3. Handle callback + `OnRequestFailure`.

**Good for**
- auth service
- leaderboard service
- billing/entitlement check

## 4. Event Streaming (WebSocket client)

**Flow**
1. Game server connects to a WS broker.
2. Receive real-time events via text/json callback.
3. Trigger actions in the gamemode.

**Good for**
- live moderation events
- cross-server sync signal
- in-game notification pipeline

## 5. Hybrid Internal API Layer

**Flow**
1. Inbound REST for panel/admin.
2. Outbound REST for microservices.
3. Node-based JSON as a unified data format.

**Good for**
- service-oriented architecture
- moving heavy logic to external services

## 6. Migration from separate plugins to one stack

**Flow**
1. Replace inbound API with PawnREST route API.
2. Replace outbound requests with `RequestsClient` + `RequestJSON`.
3. Replace websocket clients with `WebSocketClient` / `JsonWebSocketClient`.

**Benefit**
- fewer dependencies
- consistent JSON patterns
- simpler operations
