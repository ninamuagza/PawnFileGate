#if defined _pawnrest_included
    #endinput
#endif
#define _pawnrest_included

/**
 * PawnREST Plugin - HTTP File Receiver, Uploader & REST API Framework
 * Compatible Pawn include for open.mp / SA-MP
 */

// ─────────────────────────────────────────────────────────────────────────────
// HTTP Methods (for REST API)
// ─────────────────────────────────────────────────────────────────────────────

#define HTTP_METHOD_GET               0
#define HTTP_METHOD_POST              1
#define HTTP_METHOD_PUT               2
#define HTTP_METHOD_PATCH             3
#define HTTP_METHOD_DELETE            4
#define HTTP_METHOD_HEAD              5
#define HTTP_METHOD_OPTIONS           6

// ─────────────────────────────────────────────────────────────────────────────
// Conflict Modes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * If a file with the same name already exists:
 *
 * CONFLICT_RENAME    -> Rename the new file automatically (_1, _2, etc.)
 * CONFLICT_OVERWRITE -> Replace the existing file
 * CONFLICT_REJECT    -> Reject the upload
 */
#define CONFLICT_RENAME        0
#define CONFLICT_OVERWRITE     1
#define CONFLICT_REJECT        2

// ─────────────────────────────────────────────────────────────────────────────
// Corrupt File Actions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Action to take when uploaded file CRC validation fails.
 *
 * CORRUPT_DELETE     -> Delete the corrupted file
 * CORRUPT_QUARANTINE -> Move it into quarantine folder
 * CORRUPT_KEEP       -> Keep the file anyway
 */
#define CORRUPT_DELETE         0
#define CORRUPT_QUARANTINE     1
#define CORRUPT_KEEP           2

// ─────────────────────────────────────────────────────────────────────────────
// Upload Modes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Outgoing upload mode:
 *
 * UPLOAD_MODE_MULTIPART -> Standard multipart/form-data upload
 * UPLOAD_MODE_RAW       -> Raw body upload
 */
#define UPLOAD_MODE_MULTIPART  0
#define UPLOAD_MODE_RAW        1

// ─────────────────────────────────────────────────────────────────────────────
// Upload Status
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Status codes for outgoing uploads.
 */
#define UPLOAD_PENDING         0
#define UPLOAD_UPLOADING       1
#define UPLOAD_COMPLETED       2
#define UPLOAD_FAILED          3
#define UPLOAD_CANCELLED       4

// Outbound Request Status
#define REQUEST_PENDING        0
#define REQUEST_REQUESTING     1
#define REQUEST_COMPLETED      2
#define REQUEST_FAILED         3
#define REQUEST_CANCELLED      4

// ─────────────────────────────────────────────────────────────────────────────
// Outgoing Upload Error Codes
// ─────────────────────────────────────────────────────────────────────────────

#define PAWNREST_ERR_NONE                  0
#define PAWNREST_ERR_INVALID_URL           1001
#define PAWNREST_ERR_UNSUPPORTED_SCHEME    1002
#define PAWNREST_ERR_TLS_UNAVAILABLE       1003
#define PAWNREST_ERR_TLS_INVALID_CERTS     1004
#define PAWNREST_ERR_FILE_NOT_FOUND        1005
#define PAWNREST_ERR_EMPTY_FILE            1006
#define PAWNREST_ERR_CANCELLED             1007
#define PAWNREST_ERR_HTTP_STATUS           1008
#define PAWNREST_ERR_NETWORK               1100
#define PAWNREST_ERR_TIMEOUT               1101
#define PAWNREST_ERR_TLS_HANDSHAKE         1102
#define PAWNREST_ERR_UNKNOWN               1199
#define PAWNREST_ERR_JSON_PARSE            1200
#define PAWNREST_ERR_WEBSOCKET             1201

// ─────────────────────────────────────────────────────────────────────────────
// JSON Node Types
// ─────────────────────────────────────────────────────────────────────────────

#define PAWNREST_NODE_NUMBER               0
#define PAWNREST_NODE_BOOLEAN              1
#define PAWNREST_NODE_STRING               2
#define PAWNREST_NODE_OBJECT               3
#define PAWNREST_NODE_ARRAY                4
#define PAWNREST_NODE_NULL                 5

// JSON node aliases
#define JSON_NODE_NUMBER                   PAWNREST_NODE_NUMBER
#define JSON_NODE_BOOLEAN                  PAWNREST_NODE_BOOLEAN
#define JSON_NODE_STRING                   PAWNREST_NODE_STRING
#define JSON_NODE_OBJECT                   PAWNREST_NODE_OBJECT
#define JSON_NODE_ARRAY                    PAWNREST_NODE_ARRAY
#define JSON_NODE_NULL                     PAWNREST_NODE_NULL

// ─────────────────────────────────────────────────────────────────────────────
// Server Control
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Starts the HTTP upload server.
 *
 * @param port The port to listen on
 * @return true on success, false on failure
 */
native bool:REST_Start(port);

/**
 * Starts HTTPS server with TLS certificates.
 *
 * @param port The port to listen on
 * @param certPath Path to certificate file (.pem/.crt)
 * @param keyPath Path to private key file (.pem/.key)
 * @return true on success, false on failure
 */
native bool:REST_StartTLS(port, const certPath[], const keyPath[]);

/**
 * Stops the HTTP upload server.
 *
 * @return true if stopped successfully
 */
native bool:REST_Stop();

/**
 * Checks whether the upload server is currently running.
 *
 * @return true if running, false otherwise
 */
native REST_IsRunning();

/**
 * Returns the currently active HTTP server port.
 *
 * @return active port, or 0 if server is not running
 */
native REST_GetPort();

/**
 * Returns whether the currently running server uses TLS/HTTPS.
 *
 * @return true if TLS is enabled, false otherwise
 */
native REST_IsTLSEnabled();

// ─────────────────────────────────────────────────────────────────────────────
// Incoming Upload Routes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Registers an HTTP POST endpoint for receiving uploaded files.
 *
 * Example:
 *     REST_RegisterRoute("/map", "scriptfiles/maps/", ".map,.json", 25);
 *
 * @param endpoint    HTTP route path (example: "/map")
 * @param path        Destination directory on server
 * @param allowedExts Comma-separated allowed file extensions
 * @param maxSizeMb   Maximum allowed file size in MB
 * @return routeId, or -1 on failure
 */
native REST_RegisterRoute(const endpoint[], const path[], const allowedExts[], maxSizeMb);

/**
 * Requires "Authorization: Bearer <key>" for a specific route.
 *
 * @param routeId Route ID
 * @param key Authorization key
 * @return true on success
 */
native bool:REST_AddKey(routeId, const key[]);

/**
 * Removes a previously allowed authorization key from a route.
 *
 * @param routeId Route ID
 * @param key Authorization key
 * @return true on success
 */
native bool:REST_RemoveKey(routeId, const key[]);

/**
 * Sets how filename conflicts should be handled for a route.
 *
 * Modes:
 *     CONFLICT_RENAME
 *     CONFLICT_OVERWRITE
 *     CONFLICT_REJECT
 *
 * @param routeId Route ID
 * @param mode Conflict mode
 * @return true on success
 */
native bool:REST_SetConflict(routeId, mode);

/**
 * Sets what to do if uploaded file CRC validation fails.
 *
 * Actions:
 *     CORRUPT_DELETE
 *     CORRUPT_QUARANTINE
 *     CORRUPT_KEEP
 *
 * @param routeId Route ID
 * @param action Corrupt file action
 * @return true on success
 */
native bool:REST_SetCorruptAction(routeId, action);

/**
 * Requires incoming uploads on this route to include X-File-CRC32 header.
 *
 * @param routeId Route ID
 * @param required true to require CRC32, false to disable requirement
 * @return true on success
 */
native bool:REST_SetRequireCRC32(routeId, bool:required);

/**
 * Removes an existing upload route.
 *
 * @param routeId Route ID
 * @return true on success
 */
native bool:REST_RemoveRoute(routeId);

// ─────────────────────────────────────────────────────────────────────────────
// REST API Routes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Registers a custom REST API endpoint.
 *
 * Example:
 *     REST_Route(HTTP_METHOD_GET, "/api/players", "OnGetPlayers");
 *     REST_Route(HTTP_METHOD_POST, "/api/ban", "OnPostBan");
 *     REST_Route(HTTP_METHOD_GET, "/api/player/{id}", "OnGetPlayer");
 *
 * @param method HTTP method (HTTP_METHOD_GET, HTTP_METHOD_POST, HTTP_METHOD_PUT, HTTP_METHOD_PATCH, HTTP_METHOD_DELETE, HTTP_METHOD_HEAD, HTTP_METHOD_OPTIONS)
 * @param endpoint Route path (supports {param} placeholders)
 * @param callback Pawn callback function name
 * @return routeId, or -1 on failure
 */
native REST_Route(method, const endpoint[], const callback[]);

/**
 * Removes a custom API route.
 *
 * @param routeId Route ID
 * @return true on success
 */
native bool:REST_RemoveAPIRoute(routeId);

/**
 * Sets authorization key for an API route.
 * Requires "Authorization: Bearer <key>" header.
 *
 * @param routeId Route ID
 * @param key Authorization key
 * @return true on success
 */
native bool:REST_SetRouteAuth(routeId, const key[]);

// ─────────────────────────────────────────────────────────────────────────────
// REST API - Request Data
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Gets the client IP address.
 *
 * @param requestId Request ID
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native REST_GetRequestIP(requestId, output[], outputSize);

/**
 * Gets the HTTP method of the request.
 *
 * @param requestId Request ID
 * @return HTTP method constant (HTTP_METHOD_GET, HTTP_METHOD_POST, etc.) or -1 on error
 */
native REST_GetRequestMethod(requestId);

/**
 * Gets the request path.
 *
 * @param requestId Request ID
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native REST_GetRequestPath(requestId, output[], outputSize);

/**
 * Gets the raw request body.
 *
 * @param requestId Request ID
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return Body length
 */
native REST_GetRequestBody(requestId, output[], outputSize);

/**
 * Gets the length of the request body.
 *
 * @param requestId Request ID
 * @return Body length in bytes
 */
native REST_GetRequestBodyLength(requestId);

/**
 * Gets a URL parameter value (e.g., {id} from /api/player/{id}).
 *
 * @param requestId Request ID
 * @param name Parameter name
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native REST_GetParam(requestId, const name[], output[], outputSize);

/**
 * Gets a URL parameter as integer.
 *
 * @param requestId Request ID
 * @param name Parameter name
 * @return Parameter value or 0 if not found
 */
native REST_GetParamInt(requestId, const name[]);

/**
 * Gets a query string parameter (e.g., ?page=1&limit=10).
 *
 * @param requestId Request ID
 * @param name Query parameter name
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native REST_GetQuery(requestId, const name[], output[], outputSize);

/**
 * Gets a query string parameter as integer.
 *
 * @param requestId Request ID
 * @param name Query parameter name
 * @param defaultValue Default value if not found
 * @return Parameter value or defaultValue
 */
native REST_GetQueryInt(requestId, const name[], defaultValue = 0);

/**
 * Gets a request header value.
 *
 * @param requestId Request ID
 * @param name Header name
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native REST_GetHeader(requestId, const name[], output[], outputSize);

// ─────────────────────────────────────────────────────────────────────────────
// JSON API (node-only, concise naming style)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Parse raw JSON or request JSON body into node tree handles.
 */
native JsonParse(const json[]) = REST_JsonParseNode;
native RequestJson(requestId) = REST_RequestJsonNode;

/**
 * Node metadata and lifecycle.
 */
native JsonNodeType(nodeId) = REST_NodeType;
native JsonStringify(nodeId, output[], outputSize) = REST_NodeStringify;
native bool:JsonCleanup(nodeId) = REST_NodeRelease;

/**
 * Node constructors.
 */
native JsonObject(...) = REST_JsonObject;
native JsonArray(...) = REST_JsonArray;
native JsonString(const value[]) = REST_NodeString;
native JsonInt(value) = REST_NodeInt;
native Float:JsonFloat(Float:value) = REST_NodeFloat;
native JsonBool(bool:value) = REST_NodeBool;
native JsonNull() = REST_NodeNull;
native JsonAppend(leftNodeId, rightNodeId) = REST_JsonAppend;

/**
 * Object operations.
 */
native bool:JsonSetObject(objectNodeId, const key[], valueNodeId) = REST_NodeSet;
native bool:JsonSetString(objectNodeId, const key[], const value[]) = REST_NodeSetString;
native bool:JsonSetInt(objectNodeId, const key[], value) = REST_NodeSetInt;
native bool:JsonSetFloat(objectNodeId, const key[], Float:value) = REST_NodeSetFloat;
native bool:JsonSetBool(objectNodeId, const key[], bool:value) = REST_NodeSetBool;
native bool:JsonSetNull(objectNodeId, const key[]) = REST_NodeSetNull;
native bool:JsonHas(objectNodeId, const key[]) = REST_NodeHas;
native JsonGetObject(objectNodeId, const key[]) = REST_NodeGet;
native JsonGetString(objectNodeId, const key[], output[], outputSize) = REST_NodeGetString;
native JsonGetInt(objectNodeId, const key[], defaultValue = 0) = REST_NodeGetInt;
native Float:JsonGetFloat(objectNodeId, const key[], Float:defaultValue = 0.0) = REST_NodeGetFloat;
native bool:JsonGetBool(objectNodeId, const key[], bool:defaultValue = false) = REST_NodeGetBool;

/**
 * Array operations.
 */
native JsonArrayLength(arrayNodeId) = REST_NodeArrayLength;
native JsonArrayObject(arrayNodeId, index) = REST_NodeArrayGet;
native bool:JsonArrayAppend(arrayNodeId, valueNodeId) = REST_NodeArrayPush;
native bool:JsonArrayAppendString(arrayNodeId, const value[]) = REST_NodeArrayPushString;
native bool:JsonArrayAppendInt(arrayNodeId, value) = REST_NodeArrayPushInt;
native bool:JsonArrayAppendFloat(arrayNodeId, Float:value) = REST_NodeArrayPushFloat;
native bool:JsonArrayAppendBool(arrayNodeId, bool:value) = REST_NodeArrayPushBool;
native bool:JsonArrayAppendNull(arrayNodeId) = REST_NodeArrayPushNull;

// ─────────────────────────────────────────────────────────────────────────────
// Response Helpers (concise naming style)
// ─────────────────────────────────────────────────────────────────────────────

native bool:Respond(requestId, status, const body[], const contentType[] = "application/json") = REST_Respond;
native bool:RespondJSON(requestId, status, const json[]) = REST_RespondJSON;
native bool:RespondNode(requestId, status, nodeId) = REST_RespondNode;
native bool:RespondError(requestId, status, const message[]) = REST_RespondError;
native bool:SetResponseHeader(requestId, const name[], const value[]) = REST_SetResponseHeader;

// ─────────────────────────────────────────────────────────────────────────────
// File Route Permissions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Allow listing files via REST API.
 * Endpoint: GET {route}/files
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:REST_AllowList(routeId, bool:allow);

/**
 * Allow downloading files via REST API.
 * Endpoint: GET {route}/files/{filename}
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:REST_AllowDownload(routeId, bool:allow);

/**
 * Allow deleting files via REST API.
 * Endpoint: DELETE {route}/files/{filename}
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:REST_AllowDelete(routeId, bool:allow);

/**
 * Allow getting file info via REST API.
 * Endpoint: GET {route}/files/{filename}/info
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:REST_AllowInfo(routeId, bool:allow);

// ─────────────────────────────────────────────────────────────────────────────
// File Operations (Pawn-side)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Gets the number of files in a route's directory.
 *
 * @param routeId Route ID
 * @return Number of files, or -1 on error
 */
native REST_GetFileCount(routeId);

/**
 * Gets a filename by index from a route's directory.
 *
 * @param routeId Route ID
 * @param index File index (0-based)
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native REST_GetFileName(routeId, index, output[], outputSize);

/**
 * Deletes a file from a route's directory.
 *
 * @param routeId Route ID
 * @param filename Filename to delete
 * @return true on success
 */
native bool:REST_DeleteFile(routeId, const filename[]);

/**
 * Gets the size of a file in a route's directory.
 *
 * @param routeId Route ID
 * @param filename Filename
 * @return File size in bytes, or 0 if not found
 */
native REST_GetFileSize(routeId, const filename[]);

// ─────────────────────────────────────────────────────────────────────────────
// Outgoing Upload Client
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Uploads a local file from this server to another HTTP endpoint.
 *
 * @param url            Target upload URL
 * @param filepath       Local file path
 * @param filename       Optional override filename (leave empty to use original)
 * @param authKey        Optional Bearer token
 * @param customHeaders  Optional extra headers in format:
 *                       "Key: Value|Key2: Value2"
 * @param calculateCrc32 true to calculate and send CRC32 automatically
 * @param mode           UPLOAD_MODE_MULTIPART or UPLOAD_MODE_RAW
 * @param verifyTls      true to verify TLS certificate for https:// targets
 * @return uploadId, or -1 on failure
 */
native REST_UploadFile(
    const url[],
    const filepath[],
    const filename[] = "",
    const authKey[] = "",
    const customHeaders[] = "",
    calculateCrc32 = 1,
    mode = UPLOAD_MODE_MULTIPART,
    bool:verifyTls = true
);

/**
 * Creates a reusable upload client (base URL + default headers).
 *
 * @param baseUrl Base URL (http://host:port or https://host:port)
 * @param defaultHeaders Default headers in format: "Key: Value|Key2: Value2"
 * @param verifyTls true to verify remote TLS certificate by default
 * @return clientId, or -1 on failure
 */
native REST_CreateUploadClient(const baseUrl[], const defaultHeaders[] = "", bool:verifyTls = true);

/**
 * Removes an upload client.
 *
 * @param clientId Upload client ID
 * @return true on success
 */
native bool:REST_RemoveUploadClient(clientId);

/**
 * Sets or overwrites a default header on an upload client.
 *
 * @param clientId Upload client ID
 * @param name Header name
 * @param value Header value
 * @return true on success
 */
native bool:REST_SetUploadClientHeader(clientId, const name[], const value[]);

/**
 * Removes a default header from an upload client.
 *
 * @param clientId Upload client ID
 * @param name Header name
 * @return true on success
 */
native bool:REST_RemoveUploadClientHeader(clientId, const name[]);

/**
 * Uploads a file using a pre-configured upload client.
 *
 * @param clientId Upload client ID
 * @param path Relative path appended to client base URL
 * @param filepath Local file path
 * @param filename Optional override filename
 * @param authKey Optional Bearer token
 * @param customHeaders Optional request headers ("Key: Value|Key2: Value2")
 * @param calculateCrc32 true to calculate and send CRC32 automatically
 * @param mode UPLOAD_MODE_MULTIPART or UPLOAD_MODE_RAW
 * @return uploadId, or -1 on failure
 */
native REST_UploadFileWithClient(
    clientId,
    const path[],
    const filepath[],
    const filename[] = "",
    const authKey[] = "",
    const customHeaders[] = "",
    calculateCrc32 = 1,
    mode = UPLOAD_MODE_MULTIPART
);

/**
 * Cancels a pending or active outgoing upload.
 *
 * @param uploadId Upload ID
 * @return true on success
 */
native bool:REST_CancelUpload(uploadId);

/**
 * Returns the current status of an outgoing upload.
 *
 * Possible values:
 *     UPLOAD_PENDING
 *     UPLOAD_UPLOADING
 *     UPLOAD_COMPLETED
 *     UPLOAD_FAILED
 *     UPLOAD_CANCELLED
 *
 * @param uploadId Upload ID
 * @return Upload status code
 */
native REST_GetUploadStatus(uploadId);

/**
 * Returns the current progress percentage of an outgoing upload.
 *
 * @param uploadId Upload ID
 * @return Progress from 0 to 100
 */
native REST_GetUploadProgress(uploadId);

/**
 * Returns the response body of a completed outgoing upload.
 *
 * Note:
 * This only returns true if the upload finished successfully.
 *
 * @param uploadId Upload ID
 * @param output Destination buffer
 * @param outputSize Buffer size
 * @return 1 if upload completed successfully, 0 otherwise
 */
native REST_GetUploadResponse(uploadId, output[], outputSize);

/**
 * Returns structured error code for an outgoing upload.
 *
 * @param uploadId Upload ID
 * @return PAWNREST_ERR_* code, or -1 if upload not found
 */
native REST_GetUploadErrorCode(uploadId);

/**
 * Returns structured error type string for an outgoing upload.
 *
 * @param uploadId Upload ID
 * @param output Destination buffer
 * @param outputSize Buffer size
 * @return 1 if available, 0 otherwise
 */
native REST_GetUploadErrorType(uploadId, output[], outputSize);

/**
 * Returns last HTTP status code for an outgoing upload.
 *
 * @param uploadId Upload ID
 * @return HTTP status or 0 if unavailable
 */
native REST_GetUploadHttpStatus(uploadId);

// ─────────────────────────────────────────────────────────────────────────────
// Outbound Requests
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Creates a reusable outbound HTTP client (base URL + default headers).
 */
native REST_RequestsClient(const endpoint[], const defaultHeaders[] = "", bool:verifyTls = true);
native bool:REST_RemoveRequestsClient(clientId);
native bool:REST_SetRequestsClientHeader(clientId, const name[], const value[]);
native bool:REST_RemoveRequestsClientHeader(clientId, const name[]);

/**
 * Sends a text request asynchronously.
 * Callback signature: callback(requestId, httpStatus, const data[], dataLen)
 */
native REST_Request(
    clientId,
    const path[],
    method,
    const callback[],
    const body[] = "",
    const headers[] = ""
);

/**
 * Sends a JSON request asynchronously.
 * Callback signature: callback(requestId, httpStatus, nodeId)
 *
 * @param jsonNodeId Optional JSON node for request body (-1 for empty body)
 */
native REST_RequestJSON(
    clientId,
    const path[],
    method,
    const callback[],
    jsonNodeId = -1,
    const headers[] = ""
);

native bool:REST_CancelRequest(requestId);
native REST_GetRequestStatus(requestId);
native REST_GetRequestHttpStatus(requestId);
native REST_GetRequestErrorCode(requestId);
native REST_GetRequestErrorType(requestId, output[], outputSize);
native REST_GetRequestResponse(requestId, output[], outputSize);

// ─────────────────────────────────────────────────────────────────────────────
// WebSocket Client
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Text websocket.
 * Callback signature: callback(socketId, const data[], dataLen)
 */
native REST_WebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);

/**
 * JSON websocket.
 * Callback signature: callback(socketId, nodeId)
 */
native REST_JsonWebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);

native bool:REST_WebSocketSend(socketId, const data[]);
native bool:REST_JsonWebSocketSend(socketId, nodeId);
native bool:REST_WebSocketClose(socketId, status = 1000, const reason[] = "");
native bool:REST_RemoveWebSocketClient(socketId);
native bool:REST_IsWebSocketOpen(socketId);

// ─────────────────────────────────────────────────────────────────────────────
// CRC32 Utilities
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Verifies a local file against a given CRC32 checksum.
 *
 * @param filepath Local file path
 * @param expectedCrc Expected CRC32 string
 * @return 1 if match, 0 if mismatch, -1 on error
 */
native REST_VerifyCRC32(const filepath[], const expectedCrc[]);

/**
 * Calculates the CRC32 checksum of a local file.
 *
 * @param filepath Local file path
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 on failure
 */
native REST_GetFileCRC32(const filepath[], output[], outputSize);

/**
 * Compares two local files using CRC32.
 *
 * @param path1 First file path
 * @param path2 Second file path
 * @return 1 if identical, 0 if different, -1 on error
 */
native REST_CompareFiles(const path1[], const path2[]);

// ─────────────────────────────────────────────────────────────────────────────
// Incoming Callbacks
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Called when an incoming file upload has been successfully received and saved.
 *
 * @param uploadId Upload ID
 * @param routeId Route ID
 * @param endpoint Endpoint path
 * @param filename Final stored filename
 * @param filepath Final stored file path
 * @param crc32 Calculated CRC32 of uploaded file
 * @param crcMatched 1 if CRC matched expected value, 0 otherwise
 */
forward OnFileUploaded(
    uploadId,
    routeId,
    const endpoint[],
    const filename[],
    const filepath[],
    const crc32[],
    crcMatched
);

/**
 * Called when an incoming upload fails.
 *
 * Example failure reasons:
 * - file too large
 * - invalid extension
 * - unauthorized
 * - malformed upload
 * - CRC mismatch
 *
 * @param uploadId Upload ID
 * @param reason Failure reason
 * @param crc32 Calculated CRC32 if available, otherwise empty
 */
forward OnFileFailedUpload(
    uploadId,
    const reason[],
    const crc32[]
);

/**
 * Called periodically during an incoming upload.
 *
 * @param uploadId Upload ID
 * @param percent Upload progress (0-100)
 */
forward OnUploadProgress(uploadId, percent);

// ─────────────────────────────────────────────────────────────────────────────
// Outgoing Callbacks
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Called when an outgoing upload starts.
 *
 * @param uploadId Upload ID
 */
forward OnFileUploadStarted(uploadId);

/**
 * Called periodically during an outgoing upload.
 *
 * @param uploadId Upload ID
 * @param percent Upload progress (0-100)
 */
forward OnFileUploadProgress(uploadId, percent);

/**
 * Called when an outgoing upload completes successfully.
 *
 * @param uploadId Upload ID
 * @param httpStatus HTTP response status code
 * @param responseBody Response body from remote server
 * @param crc32 CRC32 of uploaded file
 */
forward OnFileUploadCompleted(
    uploadId,
    httpStatus,
    const responseBody[],
    const crc32[]
);

/**
 * Called when an outgoing upload fails.
 *
 * @param uploadId Upload ID
 * @param errorMessage Failure reason
 */
forward OnFileUploadFailed(uploadId, const errorMessage[]);

/**
 * Called when an outgoing upload fails with structured error metadata.
 *
 * @param uploadId Upload ID
 * @param errorCode PAWNREST_ERR_* code
 * @param errorType Error type category (e.g. "timeout", "network", "tls")
 * @param errorMessage Detailed failure reason
 * @param httpStatus HTTP status code if available
 */
forward OnFileUploadFailure(uploadId, errorCode, const errorType[], const errorMessage[], httpStatus);

/**
 * Called when outbound Request/RequestJSON fails due to transport/internal errors.
 * (HTTP response codes still go to the per-request callback.)
 *
 * Signature is designed for global request failure handling.
 */
forward OnRequestFailure(requestId, errorCode, const errorMessage[], len);

/**
 * Extended request failure callback with error type and HTTP status metadata.
 */
forward OnRequestFailureDetailed(requestId, errorCode, const errorType[], const errorMessage[], httpStatus);

/**
 * Called when a websocket disconnects or is closed.
 *
 * @param socketId WebSocket handle
 * @param isJson 1 for JSON websocket, 0 for text websocket
 * @param status Close status code
 * @param reason Close reason
 * @param reasonLen Reason length
 * @param errorCode PAWNREST_ERR_* code for transport/parser issues
 */
forward OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode);

// ─────────────────────────────────────────────────────────────────────────────
// Header Helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Initializes a header string in "Key: Value" format.
 *
 * @param output Destination buffer
 * @param outputSize Buffer size
 * @param key Header name
 * @param value Header value
 * @return true on success
 */
stock bool:REST_RequestHeaders(output[], outputSize, const key[], const value[])
{
    if (outputSize <= 0) return false;
    format(output, outputSize, "%s: %s", key, value);
    return true;
}

/**
 * Appends header pair using "Key: Value|Key2: Value2" format.
 *
 * @param headers Existing header buffer
 * @param outputSize Buffer size
 * @param key Header name
 * @param value Header value
 * @return true on success
 */
stock bool:REST_RequestHeadersAppend(headers[], outputSize, const key[], const value[])
{
    if (outputSize <= 0) return false;
    if (strlen(headers) > 0) {
        format(headers, outputSize, "%s|%s: %s", headers, key, value);
    } else {
        format(headers, outputSize, "%s: %s", key, value);
    }
    return true;
}