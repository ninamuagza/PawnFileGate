#if defined _pawnrest_included
    #endinput
#endif
#define _pawnrest_included

// -----------------------------------------------------------------------------
// HTTP Methods
// -----------------------------------------------------------------------------

#define HTTP_METHOD_GET         0
#define HTTP_METHOD_POST        1
#define HTTP_METHOD_PUT         2
#define HTTP_METHOD_PATCH       3
#define HTTP_METHOD_DELETE      4
#define HTTP_METHOD_HEAD        5
#define HTTP_METHOD_OPTIONS     6


// -----------------------------------------------------------------------------
// Conflict Modes — behavior when an uploaded filename already exists
//
//   CONFLICT_RENAME    — auto-rename the new file (_1, _2, …)
//   CONFLICT_OVERWRITE — replace the existing file
//   CONFLICT_REJECT    — reject the upload entirely
// -----------------------------------------------------------------------------

#define CONFLICT_RENAME         0
#define CONFLICT_OVERWRITE      1
#define CONFLICT_REJECT         2


// -----------------------------------------------------------------------------
// Corrupt File Actions — behavior when CRC32 validation fails on upload
//
//   CORRUPT_DELETE     — delete the corrupted file
//   CORRUPT_QUARANTINE — move it to the quarantine folder
//   CORRUPT_KEEP       — keep the file regardless
// -----------------------------------------------------------------------------

#define CORRUPT_DELETE          0
#define CORRUPT_QUARANTINE      1
#define CORRUPT_KEEP            2


// -----------------------------------------------------------------------------
// Upload Modes — encoding used for outgoing file uploads
//
//   UPLOAD_MODE_MULTIPART — standard multipart/form-data
//   UPLOAD_MODE_RAW       — raw binary body
// -----------------------------------------------------------------------------

#define UPLOAD_MODE_MULTIPART   0
#define UPLOAD_MODE_RAW         1


// -----------------------------------------------------------------------------
// Upload Status — state of an outgoing file upload
// -----------------------------------------------------------------------------

#define UPLOAD_PENDING          0
#define UPLOAD_UPLOADING        1
#define UPLOAD_COMPLETED        2
#define UPLOAD_FAILED           3
#define UPLOAD_CANCELLED        4


// -----------------------------------------------------------------------------
// Request Status — state of an outbound HTTP request
// -----------------------------------------------------------------------------

#define REQUEST_PENDING         0
#define REQUEST_REQUESTING      1
#define REQUEST_COMPLETED       2
#define REQUEST_FAILED          3
#define REQUEST_CANCELLED       4


// -----------------------------------------------------------------------------
// Error Codes — structured codes for upload and request failures
// -----------------------------------------------------------------------------

#define PAWNREST_ERR_NONE               0
#define PAWNREST_ERR_INVALID_URL        1001
#define PAWNREST_ERR_UNSUPPORTED_SCHEME 1002
#define PAWNREST_ERR_TLS_UNAVAILABLE    1003
#define PAWNREST_ERR_TLS_INVALID_CERTS  1004
#define PAWNREST_ERR_FILE_NOT_FOUND     1005
#define PAWNREST_ERR_EMPTY_FILE         1006
#define PAWNREST_ERR_CANCELLED          1007
#define PAWNREST_ERR_HTTP_STATUS        1008
#define PAWNREST_ERR_NETWORK            1100
#define PAWNREST_ERR_TIMEOUT            1101
#define PAWNREST_ERR_TLS_HANDSHAKE      1102
#define PAWNREST_ERR_UNKNOWN            1199
#define PAWNREST_ERR_JSON_PARSE         1200
#define PAWNREST_ERR_WEBSOCKET          1201


// -----------------------------------------------------------------------------
// JSON Node Types
// -----------------------------------------------------------------------------

#define PAWNREST_NODE_NUMBER    0
#define PAWNREST_NODE_BOOLEAN   1
#define PAWNREST_NODE_STRING    2
#define PAWNREST_NODE_OBJECT    3
#define PAWNREST_NODE_ARRAY     4
#define PAWNREST_NODE_NULL      5

// Aliases for convenience
#define JSON_NODE_NUMBER        PAWNREST_NODE_NUMBER
#define JSON_NODE_BOOLEAN       PAWNREST_NODE_BOOLEAN
#define JSON_NODE_STRING        PAWNREST_NODE_STRING
#define JSON_NODE_OBJECT        PAWNREST_NODE_OBJECT
#define JSON_NODE_ARRAY         PAWNREST_NODE_ARRAY
#define JSON_NODE_NULL          PAWNREST_NODE_NULL


// =============================================================================
// Server Control
// =============================================================================

// Start the HTTP server on the given port.
// Returns true on success.
native bool:REST_Start(port);

// Start the HTTPS server with TLS. certPath and keyPath must point to valid
// .pem/.crt and .pem/.key files respectively.
native bool:REST_StartTLS(port, const certPath[], const keyPath[]);

// Stop the running server. Returns true if stopped successfully.
native bool:REST_Stop();

// Returns true if the server is currently running.
native REST_IsRunning();

// Returns the active port, or 0 if the server is not running.
native REST_GetPort();

// Returns true if the running server has TLS enabled.
native REST_IsTLSEnabled();


// =============================================================================
// Incoming File Upload Routes
// =============================================================================

// Register an HTTP POST endpoint to receive uploaded files.
//
//   endpoint    — route path, e.g. "/map"
//   path        — destination directory, e.g. "scriptfiles/maps/"
//   allowedExts — comma-separated extensions, e.g. ".map,.json"
//   maxSizeMb   — maximum file size in megabytes
//
// Returns a routeId, or -1 on failure.
native FILE_RegisterRoute(const endpoint[], const path[], const allowedExts[], maxSizeMb) = REST_RegisterRoute;

// Require "Authorization: Bearer <key>" on the given route.
native bool:FILE_AddAuthKey(routeId, const key[]) = REST_AddKey;

// Remove a previously added authorization key from a route.
native bool:FILE_RemoveAuthKey(routeId, const key[]) = REST_RemoveKey;

// Set the filename conflict mode for a route.
// Use one of: CONFLICT_RENAME, CONFLICT_OVERWRITE, CONFLICT_REJECT
native bool:FILE_SetConflict(routeId, mode) = REST_SetConflict;

// Set the action to take when an uploaded file fails CRC32 validation.
// Use one of: CORRUPT_DELETE, CORRUPT_QUARANTINE, CORRUPT_KEEP
native bool:FILE_SetCorruptAction(routeId, action) = REST_SetCorruptAction;

// When required = true, uploads must include the X-File-CRC32 header.
native bool:FILE_SetRequireCRC32(routeId, bool:required) = REST_SetRequireCRC32;

// Unregister an upload route.
native bool:FILE_RemoveRoute(routeId) = REST_RemoveRoute;


// =============================================================================
// File Route Permissions
// =============================================================================

// Allow or deny listing files via GET {route}/files
native bool:FILE_AllowList(routeId, bool:allow) = REST_AllowList;

// Allow or deny downloading files via GET {route}/files/{filename}
native bool:FILE_AllowDownload(routeId, bool:allow) = REST_AllowDownload;

// Allow or deny deleting files via DELETE {route}/files/{filename}
native bool:FILE_AllowDelete(routeId, bool:allow) = REST_AllowDelete;

// Allow or deny file info via GET {route}/files/{filename}/info
native bool:FILE_AllowInfo(routeId, bool:allow) = REST_AllowInfo;


// =============================================================================
// File Operations (Pawn-side)
// =============================================================================

// Returns the number of files in a route's directory, or -1 on error.
native FILE_GetCount(routeId) = REST_GetFileCount;

// Writes the filename at the given index into output. Returns 1 on success.
native FILE_GetName(routeId, index, output[], outputSize) = REST_GetFileName;

// Delete a file from a route's directory. Returns true on success.
native bool:FILE_Delete(routeId, const filename[]) = REST_DeleteFile;

// Returns the file size in bytes, or 0 if not found.
native FILE_GetSize(routeId, const filename[]) = REST_GetFileSize;


// =============================================================================
// REST API Routes
// =============================================================================

// Register a custom REST API endpoint.
// Supports path parameters, e.g. "/api/player/{id}"
//
//   method   — one of the HTTP_METHOD_* constants
//   endpoint — route path
//   callback — Pawn callback function to invoke on request
//
// Returns a routeId, or -1 on failure.
native REST_RegisterAPIRoute(method, const endpoint[], const callback[]) = REST_Route;

// Unregister a custom API route.
native bool:REST_RemoveAPIRoute(routeId);

// Require "Authorization: Bearer <key>" on an API route.
native bool:REST_SetRouteAuthKey(routeId, const key[]);


// =============================================================================
// REST API — Reading Incoming Request Data
// =============================================================================

// Write the client IP address into output. Returns 1 on success.
native REST_GetRequestIP(requestId, output[], outputSize);

// Returns the HTTP method constant (HTTP_METHOD_*), or -1 on error.
native REST_GetRequestMethod(requestId);

// Write the request path into output. Returns 1 on success.
native REST_GetRequestPath(requestId, output[], outputSize);

// Write the raw request body into output. Returns the body length.
native REST_GetRequestBody(requestId, output[], outputSize);

// Returns the length of the request body in bytes.
native REST_GetRequestBodyLength(requestId);

// Write a URL path parameter (e.g. {id} from /api/player/{id}) into output.
// Returns 1 on success, 0 if not found.
native REST_GetParam(requestId, const name[], output[], outputSize);

// Returns a URL path parameter as an integer, or 0 if not found.
native REST_GetParamInt(requestId, const name[]);

// Write a query string parameter (e.g. ?page=1) into output.
// Returns 1 on success, 0 if not found.
native REST_GetQuery(requestId, const name[], output[], outputSize);

// Returns a query string parameter as an integer, or defaultValue if not found.
native REST_GetQueryInt(requestId, const name[], defaultValue = 0);

// Write a request header value into output. Returns 1 on success.
native REST_GetHeader(requestId, const name[], output[], outputSize);


// =============================================================================
// JSON API
// =============================================================================
// All nodes are referenced by integer handles. Nodes created but no longer
// needed must be freed manually with JsonCleanup to avoid memory leaks.
// =============================================================================

// Parse a raw JSON string into a node tree. Returns the root nodeId.
native JsonParse(const json[]) = REST_JsonParseNode;

// Parse the JSON body of an incoming request. Returns the root nodeId.
native GetRequestJsonNode(requestId) = REST_RequestJsonNode;

// Returns the type of a node (one of PAWNREST_NODE_* / JSON_NODE_*).
native JsonNodeType(nodeId) = REST_NodeType;

// Serialize a node tree back to a JSON string.
native JsonStringify(nodeId, output[], outputSize) = REST_NodeStringify;

// Free a node and all its children. Always call this when done with a node.
native bool:JsonCleanup(nodeId) = REST_NodeRelease;

// --- Node Constructors -------------------------------------------------------

native JsonObject(...) = REST_JsonObject;
native JsonArray(...) = REST_JsonArray;
native JsonString(const value[]) = REST_NodeString;
native JsonInt(value) = REST_NodeInt;
native Float:JsonFloat(Float:value) = REST_NodeFloat;
native JsonBool(bool:value) = REST_NodeBool;
native JsonNull() = REST_NodeNull;

// Merge two nodes (append rightNodeId into leftNodeId). Returns the result nodeId.
native JsonAppend(leftNodeId, rightNodeId) = REST_JsonAppend;

// --- Object Operations -------------------------------------------------------

native bool:JsonSetObject(objectNodeId, const key[], valueNodeId) = REST_NodeSet;
native bool:JsonSetString(objectNodeId, const key[], const value[]) = REST_NodeSetString;
native bool:JsonSetInt(objectNodeId, const key[], value) = REST_NodeSetInt;
native bool:JsonSetFloat(objectNodeId, const key[], Float:value) = REST_NodeSetFloat;
native bool:JsonSetBool(objectNodeId, const key[], bool:value) = REST_NodeSetBool;
native bool:JsonSetNull(objectNodeId, const key[]) = REST_NodeSetNull;

// Returns true if the object contains the given key.
native bool:JsonHas(objectNodeId, const key[]) = REST_NodeHas;

// Returns the child nodeId for key, or -1 if not found.
native JsonGetNode(objectNodeId, const key[]) = REST_NodeGet;

native JsonGetString(objectNodeId, const key[], output[], outputSize) = REST_NodeGetString;
native JsonGetInt(objectNodeId, const key[], defaultValue = 0) = REST_NodeGetInt;
native Float:JsonGetFloat(objectNodeId, const key[], Float:defaultValue = 0.0) = REST_NodeGetFloat;
native bool:JsonGetBool(objectNodeId, const key[], bool:defaultValue = false) = REST_NodeGetBool;

// --- Array Operations --------------------------------------------------------

// Returns the number of elements in the array.
native JsonArrayLength(arrayNodeId) = REST_NodeArrayLength;

// Returns the nodeId at the given index, or -1 if out of bounds.
native JsonArrayGetNode(arrayNodeId, index) = REST_NodeArrayGet;

native bool:JsonArrayAppend(arrayNodeId, valueNodeId) = REST_NodeArrayPush;
native bool:JsonArrayAppendString(arrayNodeId, const value[]) = REST_NodeArrayPushString;
native bool:JsonArrayAppendInt(arrayNodeId, value) = REST_NodeArrayPushInt;
native bool:JsonArrayAppendFloat(arrayNodeId, Float:value) = REST_NodeArrayPushFloat;
native bool:JsonArrayAppendBool(arrayNodeId, bool:value) = REST_NodeArrayPushBool;
native bool:JsonArrayAppendNull(arrayNodeId) = REST_NodeArrayPushNull;


// =============================================================================
// Response Helpers
// =============================================================================

// Send a plain response with a custom Content-Type (default: application/json).
native bool:Respond(requestId, status, const body[], const contentType[] = "application/json") = REST_Respond;

// Send a raw JSON string as the response body.
native bool:RespondJSON(requestId, status, const json[]) = REST_RespondJSON;

// Serialize a JSON node and send it as the response body.
native bool:RespondNode(requestId, status, nodeId) = REST_RespondNode;

// Send a standard error response: {"error": "<message>"}.
native bool:RespondError(requestId, status, const message[]) = REST_RespondError;

// Set a custom response header before calling any Respond* function.
native bool:SetResponseHeader(requestId, const name[], const value[]) = REST_SetResponseHeader;


// =============================================================================
// Outgoing File Upload Client
// =============================================================================

// Upload a local file to a remote HTTP endpoint.
//
//   url            — target URL
//   filepath       — path to the local file
//   filename       — override filename sent to the server (empty = use original)
//   authKey        — Bearer token (empty = no auth)
//   customHeaders  — extra headers in "Key: Value|Key2: Value2" format
//   calculateCrc32 — 1 to auto-calculate and send X-File-CRC32
//   mode           — UPLOAD_MODE_MULTIPART or UPLOAD_MODE_RAW
//   verifyTls      — true to verify the remote TLS certificate
//
// Returns an uploadId, or -1 on failure.
native FILE_Upload(
    const url[],
    const filepath[],
    const filename[]      = "",
    const authKey[]       = "",
    const customHeaders[] = "",
    calculateCrc32        = 1,
    mode                  = UPLOAD_MODE_MULTIPART,
    bool:verifyTls        = true
) = REST_UploadFile;

// Create a reusable upload client with a shared base URL and default headers.
// Returns a clientId, or -1 on failure.
native FILE_CreateUploadClient(const baseUrl[], const defaultHeaders[] = "", bool:verifyTls = true) = REST_CreateUploadClient;

// Destroy an upload client.
native bool:FILE_RemoveUploadClient(clientId) = REST_RemoveUploadClient;

// Set or overwrite a default header on an upload client.
native bool:FILE_SetUploadClientHeader(clientId, const name[], const value[]) = REST_SetUploadClientHeader;

// Remove a default header from an upload client.
native bool:FILE_RemoveUploadClientHeader(clientId, const name[]) = REST_RemoveUploadClientHeader;

// Upload a file using a pre-configured client. path is appended to the client's
// base URL. customHeaders override (not replace) the client's default headers.
// Returns an uploadId, or -1 on failure.
native FILE_UploadWithClient(
    clientId,
    const path[],
    const filepath[],
    const filename[]      = "",
    const authKey[]       = "",
    const customHeaders[] = "",
    calculateCrc32        = 1,
    mode                  = UPLOAD_MODE_MULTIPART
) = REST_UploadFileWithClient;

// Cancel a pending or active outgoing upload.
native bool:FILE_CancelUpload(uploadId) = REST_CancelUpload;

// Returns the current upload state (one of the UPLOAD_* constants).
native FILE_GetUploadStatus(uploadId) = REST_GetUploadStatus;

// Returns upload progress as a percentage (0–100).
native FILE_GetUploadProgress(uploadId) = REST_GetUploadProgress;

// Write the server's response body into output after a completed upload.
// Returns 1 only when status is UPLOAD_COMPLETED.
native FILE_GetUploadResponse(uploadId, output[], outputSize) = REST_GetUploadResponse;

// Returns the structured error code (PAWNREST_ERR_*), or -1 if not found.
native FILE_GetUploadErrorCode(uploadId) = REST_GetUploadErrorCode;

// Write the error type string (e.g. "timeout", "tls") into output.
native FILE_GetUploadErrorType(uploadId, output[], outputSize) = REST_GetUploadErrorType;

// Returns the last HTTP status code from the upload response, or 0 if unavailable.
native FILE_GetUploadHttpStatus(uploadId) = REST_GetUploadHttpStatus;


// =============================================================================
// Outbound HTTP Request Client
// =============================================================================

// Create a reusable HTTP client with a shared base URL and default headers.
// Returns a clientId, or -1 on failure.
native REST_CreateRequestClient(const baseUrl[], const defaultHeaders[] = "", bool:verifyTls = true) = REST_RequestsClient;

// Destroy a request client.
native bool:REST_RemoveRequestsClient(clientId);

// Set or overwrite a default header on a request client.
native bool:REST_SetRequestsClientHeader(clientId, const name[], const value[]);

// Remove a default header from a request client.
native bool:REST_RemoveRequestsClientHeader(clientId, const name[]);

// Send an asynchronous HTTP request. path is appended to the client's base URL.
// Callback signature: callback(requestId, httpStatus, const data[], dataLen)
native REST_Request(
    clientId,
    const path[],
    method,
    const callback[],
    const body[]    = "",
    const headers[] = ""
);

// Send an asynchronous HTTP request with a JSON body and auto-parsed response.
// jsonNodeId — JSON node to send as the body, or -1 for an empty body.
// Callback signature: callback(requestId, httpStatus, nodeId)
native REST_RequestJSON(
    clientId,
    const path[],
    method,
    const callback[],
    jsonNodeId      = -1,
    const headers[] = ""
);

// Cancel a pending or active outbound request.
native bool:REST_CancelRequest(requestId);

// Returns the current request state (one of the REQUEST_* constants).
native REST_GetRequestStatus(requestId);

// Returns the last HTTP status code from the request response.
native REST_GetRequestHttpStatus(requestId);

// Returns the structured error code (PAWNREST_ERR_*).
native REST_GetRequestErrorCode(requestId);

// Write the error type string into output.
native REST_GetRequestErrorType(requestId, output[], outputSize);

// Write the response body into output.
native REST_GetRequestResponse(requestId, output[], outputSize);


// =============================================================================
// WebSocket Client
// =============================================================================

// Open a text WebSocket connection.
// Callback signature: callback(socketId, const data[], dataLen)
native REST_WebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);

// Open a JSON WebSocket connection. Incoming messages are auto-parsed.
// Callback signature: callback(socketId, nodeId)
native REST_JsonWebSocketClient(const address[], const callback[], const headers[] = "", bool:verifyTls = true);

// Send a text message over an open WebSocket.
native bool:REST_WebSocketSend(socketId, const data[]);

// Send a JSON node as a message over an open WebSocket.
native bool:REST_JsonWebSocketSend(socketId, nodeId);

// Initiate a clean close handshake with an optional status code and reason.
native bool:REST_WebSocketClose(socketId, status = 1000, const reason[] = "");

// Remove and clean up a WebSocket client handle.
native bool:REST_RemoveWebSocketClient(socketId);

// Returns true if the WebSocket connection is currently open.
native bool:REST_IsWebSocketOpen(socketId);


// =============================================================================
// CRC32 Utilities
// =============================================================================

// Verify a local file against an expected CRC32 string.
// Returns 1 on match, 0 on mismatch, -1 on error.
native FILE_VerifyCRC32(const filepath[], const expectedCrc[]) = REST_VerifyCRC32;

// Write the CRC32 checksum of a local file into output.
// Returns 1 on success, 0 on failure.
native FILE_GetCRC32(const filepath[], output[], outputSize) = REST_GetFileCRC32;

// Compare two local files using CRC32.
// Returns 1 if identical, 0 if different, -1 on error.
native FILE_Compare(const path1[], const path2[]) = REST_CompareFiles;


// =============================================================================
// Incoming Callbacks
// =============================================================================

// Called when an incoming upload is successfully received and saved.
//
//   uploadId   — upload identifier
//   routeId    — route that handled the upload
//   endpoint   — route path
//   filename   — final stored filename (may differ from original if renamed)
//   filepath   — full path to the stored file
//   crc32      — calculated CRC32 of the received file
//   crcMatched — 1 if CRC matched the X-File-CRC32 header, 0 otherwise
forward OnIncomingUploadCompleted(
    uploadId,
    routeId,
    const endpoint[],
    const filename[],
    const filepath[],
    const crc32[],
    crcMatched
);

// Called when an incoming upload fails (e.g. oversized, bad extension,
// unauthorized, malformed, or CRC mismatch).
//
//   reason — human-readable failure description
//   crc32  — calculated CRC32 if available, empty string otherwise
forward OnIncomingUploadFailed(uploadId, const reason[], const crc32[]);

// Called periodically while an upload is being received.
forward OnIncomingUploadProgress(uploadId, percent);


// =============================================================================
// Outgoing Callbacks
// =============================================================================

// Called when an outgoing upload begins transferring.
forward OnOutgoingUploadStarted(uploadId);

// Called periodically while an outgoing upload is in progress.
forward OnOutgoingUploadProgress(uploadId, percent);

// Called when an outgoing upload completes successfully.
//
//   httpStatus   — HTTP response status from the remote server
//   responseBody — response body from the remote server
//   crc32        — CRC32 of the uploaded file
forward OnOutgoingUploadCompleted(uploadId, httpStatus, const responseBody[], const crc32[]);

// Called when an outgoing upload fails.
//
//   errorCode    — PAWNREST_ERR_* code
//   errorType    — category string (e.g. "timeout", "network", "tls")
//   errorMessage — detailed failure description
//   httpStatus   — HTTP status code if available, 0 otherwise
forward OnOutgoingUploadFailed(uploadId, errorCode, const errorType[], const errorMessage[], httpStatus);

// Called when an outbound request fails at the transport/internal level.
// HTTP error responses (4xx, 5xx) are still delivered to the per-request callback.
//
//   errorCode    — PAWNREST_ERR_* code
//   errorType    — category string
//   errorMessage — detailed failure description
//   httpStatus   — HTTP status code if available, 0 otherwise
forward OnRequestFailure(requestId, errorCode, const errorType[], const errorMessage[], httpStatus);

// Called when a WebSocket connection is closed or drops unexpectedly.
//
//   isJson    — true for JSON WebSocket, false for text WebSocket
//   status    — WebSocket close status code
//   reason    — close reason string
//   reasonLen — length of reason
//   errorCode — PAWNREST_ERR_* code for transport/parser failures
forward OnWebSocketDisconnect(socketId, bool:isJson, status, const reason[], reasonLen, errorCode);


// =============================================================================
// Header Builder Utilities (stock helpers, no native call)
// =============================================================================

// Initialize a header buffer with a single "Key: Value" entry.
stock bool:REST_RequestHeaders(output[], outputSize, const key[], const value[])
{
    if (outputSize <= 0)
        return false;

    format(output, outputSize, "%s: %s", key, value);
    return true;
}

// Append a "Key: Value" entry to an existing header buffer, separated by "|".
stock bool:REST_RequestHeadersAppend(headers[], outputSize, const key[], const value[])
{
    if (outputSize <= 0)
        return false;

    if (strlen(headers) > 0)
        format(headers, outputSize, "%s|%s: %s", headers, key, value);
    else
        format(headers, outputSize, "%s: %s", key, value);

    return true;
}