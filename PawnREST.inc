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

#define HTTP_GET               0
#define HTTP_POST              1
#define HTTP_PUT               2
#define HTTP_PATCH             3
#define HTTP_DELETE            4
#define HTTP_HEAD              5
#define HTTP_OPTIONS           6

// pawn-requests-style aliases
#define HTTP_METHOD_GET        HTTP_GET
#define HTTP_METHOD_POST       HTTP_POST
#define HTTP_METHOD_PUT        HTTP_PUT
#define HTTP_METHOD_PATCH      HTTP_PATCH
#define HTTP_METHOD_DELETE     HTTP_DELETE
#define HTTP_METHOD_HEAD       HTTP_HEAD
#define HTTP_METHOD_OPTIONS    HTTP_OPTIONS

// ─────────────────────────────────────────────────────────────────────────────
// Preferred REST_* Function Prefix
// ─────────────────────────────────────────────────────────────────────────────
//
// All REST_* names are aliases of PawnREST_* natives for shorter usage while
// keeping backward compatibility with existing PawnREST_* scripts.

#define REST_Start                         PawnREST_Start
#define REST_StartTLS                      PawnREST_StartTLS
#define REST_Stop                          PawnREST_Stop
#define REST_IsRunning                     PawnREST_IsRunning
#define REST_GetPort                       PawnREST_GetPort
#define REST_IsTLSEnabled                  PawnREST_IsTLSEnabled

#define REST_RegisterRoute                 PawnREST_RegisterRoute
#define REST_AddKey                        PawnREST_AddKey
#define REST_RemoveKey                     PawnREST_RemoveKey
#define REST_SetConflict                   PawnREST_SetConflict
#define REST_SetCorruptAction              PawnREST_SetCorruptAction
#define REST_SetRequireCRC32               PawnREST_SetRequireCRC32
#define REST_RemoveRoute                   PawnREST_RemoveRoute

#define REST_Route                         PawnREST_Route
#define REST_RemoveAPIRoute                PawnREST_RemoveAPIRoute
#define REST_SetRouteAuth                  PawnREST_SetRouteAuth

#define REST_GetRequestIP                  PawnREST_GetRequestIP
#define REST_GetRequestMethod              PawnREST_GetRequestMethod
#define REST_GetRequestPath                PawnREST_GetRequestPath
#define REST_GetRequestBody                PawnREST_GetRequestBody
#define REST_GetRequestBodyLength          PawnREST_GetRequestBodyLength
#define REST_GetParam                      PawnREST_GetParam
#define REST_GetParamInt                   PawnREST_GetParamInt
#define REST_GetQuery                      PawnREST_GetQuery
#define REST_GetQueryInt                   PawnREST_GetQueryInt
#define REST_GetHeader                     PawnREST_GetHeader

#define REST_AllowList                     PawnREST_AllowList
#define REST_AllowDownload                 PawnREST_AllowDownload
#define REST_AllowDelete                   PawnREST_AllowDelete
#define REST_AllowInfo                     PawnREST_AllowInfo

#define REST_GetFileCount                  PawnREST_GetFileCount
#define REST_GetFileName                   PawnREST_GetFileName
#define REST_DeleteFile                    PawnREST_DeleteFile
#define REST_GetFileSize                   PawnREST_GetFileSize

#define REST_UploadFile                    PawnREST_UploadFile
#define REST_CreateUploadClient            PawnREST_CreateUploadClient
#define REST_RemoveUploadClient            PawnREST_RemoveUploadClient
#define REST_SetUploadClientHeader         PawnREST_SetUploadClientHeader
#define REST_RemoveUploadClientHeader      PawnREST_RemoveUploadClientHeader
#define REST_UploadFileWithClient          PawnREST_UploadFileWithClient
#define REST_CancelUpload                  PawnREST_CancelUpload
#define REST_GetUploadStatus               PawnREST_GetUploadStatus
#define REST_GetUploadProgress             PawnREST_GetUploadProgress
#define REST_GetUploadResponse             PawnREST_GetUploadResponse
#define REST_GetUploadErrorCode            PawnREST_GetUploadErrorCode
#define REST_GetUploadErrorType            PawnREST_GetUploadErrorType
#define REST_GetUploadHttpStatus           PawnREST_GetUploadHttpStatus

#define REST_RequestsClient                PawnREST_RequestsClient
#define REST_RemoveRequestsClient          PawnREST_RemoveRequestsClient
#define REST_SetRequestsClientHeader       PawnREST_SetRequestsClientHeader
#define REST_RemoveRequestsClientHeader    PawnREST_RemoveRequestsClientHeader
#define REST_Request                       PawnREST_Request
#define REST_RequestJSON                   PawnREST_RequestJSON
#define REST_CancelRequest                 PawnREST_CancelRequest
#define REST_GetRequestStatus              PawnREST_GetRequestStatus
#define REST_GetRequestHttpStatus          PawnREST_GetRequestHttpStatus
#define REST_GetRequestErrorCode           PawnREST_GetRequestErrorCode
#define REST_GetRequestErrorType           PawnREST_GetRequestErrorType
#define REST_GetRequestResponse            PawnREST_GetRequestResponse

#define REST_WebSocketClient               PawnREST_WebSocketClient
#define REST_JsonWebSocketClient           PawnREST_JsonWebSocketClient
#define REST_WebSocketSend                 PawnREST_WebSocketSend
#define REST_JsonWebSocketSend             PawnREST_JsonWebSocketSend
#define REST_WebSocketClose                PawnREST_WebSocketClose
#define REST_RemoveWebSocketClient         PawnREST_RemoveWebSocketClient
#define REST_IsWebSocketOpen               PawnREST_IsWebSocketOpen

#define REST_VerifyCRC32                   PawnREST_VerifyCRC32
#define REST_GetFileCRC32                  PawnREST_GetFileCRC32
#define REST_CompareFiles                  PawnREST_CompareFiles

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

// Concise aliases (pawn-requests style)
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
native bool:PawnREST_Start(port);

/**
 * Starts HTTPS server with TLS certificates.
 *
 * @param port The port to listen on
 * @param certPath Path to certificate file (.pem/.crt)
 * @param keyPath Path to private key file (.pem/.key)
 * @return true on success, false on failure
 */
native bool:PawnREST_StartTLS(port, const certPath[], const keyPath[]);

/**
 * Stops the HTTP upload server.
 *
 * @return true if stopped successfully
 */
native bool:PawnREST_Stop();

/**
 * Checks whether the upload server is currently running.
 *
 * @return true if running, false otherwise
 */
native PawnREST_IsRunning();

/**
 * Returns the currently active HTTP server port.
 *
 * @return active port, or 0 if server is not running
 */
native PawnREST_GetPort();

/**
 * Returns whether the currently running server uses TLS/HTTPS.
 *
 * @return true if TLS is enabled, false otherwise
 */
native PawnREST_IsTLSEnabled();

// ─────────────────────────────────────────────────────────────────────────────
// Incoming Upload Routes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Registers an HTTP POST endpoint for receiving uploaded files.
 *
 * Example:
 *     PawnREST_RegisterRoute("/map", "scriptfiles/maps/", ".map,.json", 25);
 *
 * @param endpoint    HTTP route path (example: "/map")
 * @param path        Destination directory on server
 * @param allowedExts Comma-separated allowed file extensions
 * @param maxSizeMb   Maximum allowed file size in MB
 * @return routeId, or -1 on failure
 */
native PawnREST_RegisterRoute(const endpoint[], const path[], const allowedExts[], maxSizeMb);

/**
 * Requires "Authorization: Bearer <key>" for a specific route.
 *
 * @param routeId Route ID
 * @param key Authorization key
 * @return true on success
 */
native bool:PawnREST_AddKey(routeId, const key[]);

/**
 * Removes a previously allowed authorization key from a route.
 *
 * @param routeId Route ID
 * @param key Authorization key
 * @return true on success
 */
native bool:PawnREST_RemoveKey(routeId, const key[]);

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
native bool:PawnREST_SetConflict(routeId, mode);

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
native bool:PawnREST_SetCorruptAction(routeId, action);

/**
 * Requires incoming uploads on this route to include X-File-CRC32 header.
 *
 * @param routeId Route ID
 * @param required true to require CRC32, false to disable requirement
 * @return true on success
 */
native bool:PawnREST_SetRequireCRC32(routeId, bool:required);

/**
 * Removes an existing upload route.
 *
 * @param routeId Route ID
 * @return true on success
 */
native bool:PawnREST_RemoveRoute(routeId);

// ─────────────────────────────────────────────────────────────────────────────
// REST API Routes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Registers a custom REST API endpoint.
 *
 * Example:
 *     PawnREST_Route(HTTP_GET, "/api/players", "OnGetPlayers");
 *     PawnREST_Route(HTTP_POST, "/api/ban", "OnPostBan");
 *     PawnREST_Route(HTTP_GET, "/api/player/{id}", "OnGetPlayer");
 *
 * @param method HTTP method (HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE, HTTP_HEAD, HTTP_OPTIONS)
 * @param endpoint Route path (supports {param} placeholders)
 * @param callback Pawn callback function name
 * @return routeId, or -1 on failure
 */
native PawnREST_Route(method, const endpoint[], const callback[]);

/**
 * Removes a custom API route.
 *
 * @param routeId Route ID
 * @return true on success
 */
native bool:PawnREST_RemoveAPIRoute(routeId);

/**
 * Sets authorization key for an API route.
 * Requires "Authorization: Bearer <key>" header.
 *
 * @param routeId Route ID
 * @param key Authorization key
 * @return true on success
 */
native bool:PawnREST_SetRouteAuth(routeId, const key[]);

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
native PawnREST_GetRequestIP(requestId, output[], outputSize);

/**
 * Gets the HTTP method of the request.
 *
 * @param requestId Request ID
 * @return HTTP method constant (HTTP_GET, HTTP_POST, etc.) or -1 on error
 */
native PawnREST_GetRequestMethod(requestId);

/**
 * Gets the request path.
 *
 * @param requestId Request ID
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native PawnREST_GetRequestPath(requestId, output[], outputSize);

/**
 * Gets the raw request body.
 *
 * @param requestId Request ID
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return Body length
 */
native PawnREST_GetRequestBody(requestId, output[], outputSize);

/**
 * Gets the length of the request body.
 *
 * @param requestId Request ID
 * @return Body length in bytes
 */
native PawnREST_GetRequestBodyLength(requestId);

/**
 * Gets a URL parameter value (e.g., {id} from /api/player/{id}).
 *
 * @param requestId Request ID
 * @param name Parameter name
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native PawnREST_GetParam(requestId, const name[], output[], outputSize);

/**
 * Gets a URL parameter as integer.
 *
 * @param requestId Request ID
 * @param name Parameter name
 * @return Parameter value or 0 if not found
 */
native PawnREST_GetParamInt(requestId, const name[]);

/**
 * Gets a query string parameter (e.g., ?page=1&limit=10).
 *
 * @param requestId Request ID
 * @param name Query parameter name
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native PawnREST_GetQuery(requestId, const name[], output[], outputSize);

/**
 * Gets a query string parameter as integer.
 *
 * @param requestId Request ID
 * @param name Query parameter name
 * @param defaultValue Default value if not found
 * @return Parameter value or defaultValue
 */
native PawnREST_GetQueryInt(requestId, const name[], defaultValue = 0);

/**
 * Gets a request header value.
 *
 * @param requestId Request ID
 * @param name Header name
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native PawnREST_GetHeader(requestId, const name[], output[], outputSize);

// ─────────────────────────────────────────────────────────────────────────────
// JSON API (node-only, concise naming style)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Parse raw JSON or request JSON body into node tree handles.
 */
native JsonParse(const json[]) = PawnREST_JsonParseNode;
native RequestJson(requestId) = PawnREST_RequestJsonNode;

/**
 * Node metadata and lifecycle.
 */
native JsonNodeType(nodeId) = PawnREST_NodeType;
native JsonStringify(nodeId, output[], outputSize) = PawnREST_NodeStringify;
native bool:JsonCleanup(nodeId) = PawnREST_NodeRelease;

/**
 * Node constructors.
 */
native JsonObject(...) = PawnREST_JsonObject;
native JsonArray(...) = PawnREST_JsonArray;
native JsonString(const value[]) = PawnREST_NodeString;
native JsonInt(value) = PawnREST_NodeInt;
native JsonFloat(Float:value) = PawnREST_NodeFloat;
native JsonBool(bool:value) = PawnREST_NodeBool;
native JsonNull() = PawnREST_NodeNull;
native JsonAppend(leftNodeId, rightNodeId) = PawnREST_JsonAppend;

/**
 * Object operations.
 */
native bool:JsonSetObject(objectNodeId, const key[], valueNodeId) = PawnREST_NodeSet;
native bool:JsonSetString(objectNodeId, const key[], const value[]) = PawnREST_NodeSetString;
native bool:JsonSetInt(objectNodeId, const key[], value) = PawnREST_NodeSetInt;
native bool:JsonSetFloat(objectNodeId, const key[], Float:value) = PawnREST_NodeSetFloat;
native bool:JsonSetBool(objectNodeId, const key[], bool:value) = PawnREST_NodeSetBool;
native bool:JsonSetNull(objectNodeId, const key[]) = PawnREST_NodeSetNull;
native bool:JsonHas(objectNodeId, const key[]) = PawnREST_NodeHas;
native JsonGetObject(objectNodeId, const key[]) = PawnREST_NodeGet;
native JsonGetString(objectNodeId, const key[], output[], outputSize) = PawnREST_NodeGetString;
native JsonGetInt(objectNodeId, const key[], defaultValue = 0) = PawnREST_NodeGetInt;
native Float:JsonGetFloat(objectNodeId, const key[], Float:defaultValue = 0.0) = PawnREST_NodeGetFloat;
native bool:JsonGetBool(objectNodeId, const key[], bool:defaultValue = false) = PawnREST_NodeGetBool;

/**
 * Array operations.
 */
native JsonArrayLength(arrayNodeId) = PawnREST_NodeArrayLength;
native JsonArrayObject(arrayNodeId, index) = PawnREST_NodeArrayGet;
native bool:JsonArrayAppend(arrayNodeId, valueNodeId) = PawnREST_NodeArrayPush;
native bool:JsonArrayAppendString(arrayNodeId, const value[]) = PawnREST_NodeArrayPushString;
native bool:JsonArrayAppendInt(arrayNodeId, value) = PawnREST_NodeArrayPushInt;
native bool:JsonArrayAppendFloat(arrayNodeId, Float:value) = PawnREST_NodeArrayPushFloat;
native bool:JsonArrayAppendBool(arrayNodeId, bool:value) = PawnREST_NodeArrayPushBool;
native bool:JsonArrayAppendNull(arrayNodeId) = PawnREST_NodeArrayPushNull;

// ─────────────────────────────────────────────────────────────────────────────
// Response Helpers (concise naming style)
// ─────────────────────────────────────────────────────────────────────────────

native bool:Respond(requestId, status, const body[], const contentType[] = "application/json") = PawnREST_Respond;
native bool:RespondJSON(requestId, status, const json[]) = PawnREST_RespondJSON;
native bool:RespondNode(requestId, status, nodeId) = PawnREST_RespondNode;
native bool:RespondError(requestId, status, const message[]) = PawnREST_RespondError;
native bool:SetResponseHeader(requestId, const name[], const value[]) = PawnREST_SetResponseHeader;

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
native bool:PawnREST_AllowList(routeId, bool:allow);

/**
 * Allow downloading files via REST API.
 * Endpoint: GET {route}/files/{filename}
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:PawnREST_AllowDownload(routeId, bool:allow);

/**
 * Allow deleting files via REST API.
 * Endpoint: DELETE {route}/files/{filename}
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:PawnREST_AllowDelete(routeId, bool:allow);

/**
 * Allow getting file info via REST API.
 * Endpoint: GET {route}/files/{filename}/info
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:PawnREST_AllowInfo(routeId, bool:allow);

// ─────────────────────────────────────────────────────────────────────────────
// File Operations (Pawn-side)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Gets the number of files in a route's directory.
 *
 * @param routeId Route ID
 * @return Number of files, or -1 on error
 */
native PawnREST_GetFileCount(routeId);

/**
 * Gets a filename by index from a route's directory.
 *
 * @param routeId Route ID
 * @param index File index (0-based)
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native PawnREST_GetFileName(routeId, index, output[], outputSize);

/**
 * Deletes a file from a route's directory.
 *
 * @param routeId Route ID
 * @param filename Filename to delete
 * @return true on success
 */
native bool:PawnREST_DeleteFile(routeId, const filename[]);

/**
 * Gets the size of a file in a route's directory.
 *
 * @param routeId Route ID
 * @param filename Filename
 * @return File size in bytes, or 0 if not found
 */
native PawnREST_GetFileSize(routeId, const filename[]);

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
native PawnREST_UploadFile(
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
native PawnREST_CreateUploadClient(const baseUrl[], const defaultHeaders[] = "", bool:verifyTls = true);

/**
 * Removes an upload client.
 *
 * @param clientId Upload client ID
 * @return true on success
 */
native bool:PawnREST_RemoveUploadClient(clientId);

/**
 * Sets or overwrites a default header on an upload client.
 *
 * @param clientId Upload client ID
 * @param name Header name
 * @param value Header value
 * @return true on success
 */
native bool:PawnREST_SetUploadClientHeader(clientId, const name[], const value[]);

/**
 * Removes a default header from an upload client.
 *
 * @param clientId Upload client ID
 * @param name Header name
 * @return true on success
 */
native bool:PawnREST_RemoveUploadClientHeader(clientId, const name[]);

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
native PawnREST_UploadFileWithClient(
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
native bool:PawnREST_CancelUpload(uploadId);

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
native PawnREST_GetUploadStatus(uploadId);

/**
 * Returns the current progress percentage of an outgoing upload.
 *
 * @param uploadId Upload ID
 * @return Progress from 0 to 100
 */
native PawnREST_GetUploadProgress(uploadId);

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
native PawnREST_GetUploadResponse(uploadId, output[], outputSize);

/**
 * Returns structured error code for an outgoing upload.
 *
 * @param uploadId Upload ID
 * @return PAWNREST_ERR_* code, or -1 if upload not found
 */
native PawnREST_GetUploadErrorCode(uploadId);

/**
 * Returns structured error type string for an outgoing upload.
 *
 * @param uploadId Upload ID
 * @param output Destination buffer
 * @param outputSize Buffer size
 * @return 1 if available, 0 otherwise
 */
native PawnREST_GetUploadErrorType(uploadId, output[], outputSize);

/**
 * Returns last HTTP status code for an outgoing upload.
 *
 * @param uploadId Upload ID
 * @return HTTP status or 0 if unavailable
 */
native PawnREST_GetUploadHttpStatus(uploadId);

// ─────────────────────────────────────────────────────────────────────────────
// Outbound Requests (pawn-requests style)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Creates a reusable outbound HTTP client (base URL + default headers).
 */
native PawnREST_RequestsClient(const endpoint[], const defaultHeaders[] = "", bool:verifyTls = true);
native bool:PawnREST_RemoveRequestsClient(clientId);
native bool:PawnREST_SetRequestsClientHeader(clientId, const name[], const value[]);
native bool:PawnREST_RemoveRequestsClientHeader(clientId, const name[]);

/**
 * Sends a text request asynchronously.
 * Callback signature: callback(requestId, httpStatus, const data[], dataLen)
 */
native PawnREST_Request(
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
native PawnREST_RequestJSON(
    clientId,
    const path[],
    method,
    const callback[],
    jsonNodeId = -1,
    const headers[] = ""
);

native bool:PawnREST_CancelRequest(requestId);
native PawnREST_GetRequestStatus(requestId);
native PawnREST_GetRequestHttpStatus(requestId);
native PawnREST_GetRequestErrorCode(requestId);
native PawnREST_GetRequestErrorType(requestId, output[], outputSize);
native PawnREST_GetRequestResponse(requestId, output[], outputSize);

// Concise aliases
native RequestsClient(const endpoint[], const defaultHeaders[] = "", bool:verifyTls = true) = PawnREST_RequestsClient;
native bool:RemoveRequestsClient(clientId) = PawnREST_RemoveRequestsClient;
native bool:SetRequestsClientHeader(clientId, const name[], const value[]) = PawnREST_SetRequestsClientHeader;
native bool:RemoveRequestsClientHeader(clientId, const name[]) = PawnREST_RemoveRequestsClientHeader;
native Request(clientId, const path[], method, const callback[], const body[] = "", const headers[] = "") = PawnREST_Request;
native RequestJSON(clientId, const path[], method, const callback[], jsonNodeId = -1, const headers[] = "") = PawnREST_RequestJSON;
native bool:CancelRequest(requestId) = PawnREST_CancelRequest;
native RequestStatus(requestId) = PawnREST_GetRequestStatus;
native RequestHTTPStatus(requestId) = PawnREST_GetRequestHttpStatus;
native RequestErrorCode(requestId) = PawnREST_GetRequestErrorCode;
native RequestErrorType(requestId, output[], outputSize) = PawnREST_GetRequestErrorType;
native RequestResponse(requestId, output[], outputSize) = PawnREST_GetRequestResponse;

// ─────────────────────────────────────────────────────────────────────────────
// WebSocket Client (pawn-requests style)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Text websocket.
 * Callback signature: callback(socketId, const data[], dataLen)
 */
native PawnREST_WebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);

/**
 * JSON websocket.
 * Callback signature: callback(socketId, nodeId)
 */
native PawnREST_JsonWebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);

native bool:PawnREST_WebSocketSend(socketId, const data[]);
native bool:PawnREST_JsonWebSocketSend(socketId, nodeId);
native bool:PawnREST_WebSocketClose(socketId, status = 1000, const reason[] = "");
native bool:PawnREST_RemoveWebSocketClient(socketId);
native bool:PawnREST_IsWebSocketOpen(socketId);

// Concise aliases
native WebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true) = PawnREST_WebSocketClient;
native JsonWebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true) = PawnREST_JsonWebSocketClient;
native bool:WebSocketSend(socketId, const data[]) = PawnREST_WebSocketSend;
native bool:JsonWebSocketSend(socketId, nodeId) = PawnREST_JsonWebSocketSend;
native bool:WebSocketClose(socketId, status = 1000, const reason[] = "") = PawnREST_WebSocketClose;
native bool:RemoveWebSocketClient(socketId) = PawnREST_RemoveWebSocketClient;
native bool:IsWebSocketOpen(socketId) = PawnREST_IsWebSocketOpen;

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
native PawnREST_VerifyCRC32(const filepath[], const expectedCrc[]);

/**
 * Calculates the CRC32 checksum of a local file.
 *
 * @param filepath Local file path
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 on failure
 */
native PawnREST_GetFileCRC32(const filepath[], output[], outputSize);

/**
 * Compares two local files using CRC32.
 *
 * @param path1 First file path
 * @param path2 Second file path
 * @return 1 if identical, 0 if different, -1 on error
 */
native PawnREST_CompareFiles(const path1[], const path2[]);

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
 * Signature follows pawn-requests style.
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
// Header Helpers (RequestsClient-style)
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
stock bool:PawnREST_RequestHeaders(output[], outputSize, const key[], const value[])
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
stock bool:PawnREST_RequestHeadersAppend(headers[], outputSize, const key[], const value[])
{
    if (outputSize <= 0) return false;
    if (strlen(headers) > 0) {
        format(headers, outputSize, "%s|%s: %s", headers, key, value);
    } else {
        format(headers, outputSize, "%s: %s", key, value);
    }
    return true;
}

stock bool:RequestHeaders(output[], outputSize, const key[], const value[])
{
    return PawnREST_RequestHeaders(output, outputSize, key, value);
}

stock bool:RequestHeadersAppend(headers[], outputSize, const key[], const value[])
{
    return PawnREST_RequestHeadersAppend(headers, outputSize, key, value);
}
