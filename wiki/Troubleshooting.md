# Troubleshooting

Use this page when route behavior looks inconsistent or request values appear missing.

## 1. Endpoint returns `404` unexpectedly

**Symptom**
- Valid routes intermittently return `{"error":"endpoint not found"}`.

**Most common cause**
- Plugin architecture mismatch on Linux (deploying 64-bit `pawnrest.so` into a 32-bit runtime, or vice versa).

**Fix**
```bash
file components/pawnrest.so
```

Expected architecture must match your runtime:
- `ELF 32-bit ... Intel i386`
- or `ELF 64-bit ... x86-64`

If you need a 32-bit build:
```bash
cmake -S . -B build-32 -G Ninja \
  -DCMAKE_C_FLAGS=-m32 \
  -DCMAKE_CXX_FLAGS=-m32 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-32 --parallel
```

## 2. File route REST endpoints return `404`

**Symptom**
- `GET /route/files` returns `{"error":"endpoint not found"}` even though `FILE_AllowList` was called.

**Cause (older versions)**
- File ops endpoints were only registered at server startup, before routes existed.

**Fix**
- Update to the latest PawnREST build. File ops routes (`/files`, `/files/{name}`, etc.) are now registered dynamically when `FILE_RegisterRoute` is called.

## 3. Required values look empty even when sent

**Symptom**
- Handlers report missing params (`discord_id required`, etc.) while client sends body/query/header.

**Checks**
1. Ensure you are running the latest plugin build (keep `.so` and `PawnREST.inc` in sync).
2. Confirm request keys are exact (`discord_id`, etc.).
3. URL-encode query values containing reserved characters.

**Request parsing notes**
- `REST_GetQuery*` reads from request URL target query string.
- `REST_GetHeader` is case-insensitive (`X-Token` == `x-token`).

## 4. Mixed responses between valid and stale behavior

**Symptom**
- Same endpoint sometimes behaves differently without code changes.

**Cause**
- Multiple server processes bound/restarted incorrectly, or old process still handling requests.

**Fix**
1. Stop old server process cleanly.
2. Start one server instance.
3. Re-test routes after startup finishes.

## 5. File list returns array of strings, not objects

**Note**
- `GET {route}/files` returns `{ "files": ["a.map", "b.json"] }` — an array of filenames (strings), not objects with size/modified info.
- Use `GET {route}/files/{name}/info` to get metadata for a specific file.

## 6. Quick API sanity checks

```bash
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/stats
```

If these fail, verify server startup and plugin load before testing custom routes.
