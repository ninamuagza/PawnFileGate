#if defined _filegate_included
    #endinput
#endif
#define _filegate_included

/**
 * FileGate Plugin - HTTP File Receiver, Uploader & REST API Framework
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

// ─────────────────────────────────────────────────────────────────────────────
// Server Control
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Starts the HTTP upload server.
 *
 * @param port The port to listen on
 * @return true on success, false on failure
 */
native bool:FileGate_Start(port);

/**
 * Stops the HTTP upload server.
 *
 * @return true if stopped successfully
 */
native bool:FileGate_Stop();

/**
 * Checks whether the upload server is currently running.
 *
 * @return true if running, false otherwise
 */
native FileGate_IsRunning();

/**
 * Returns the currently active HTTP server port.
 *
 * @return active port, or 0 if server is not running
 */
native FileGate_GetPort();

// ─────────────────────────────────────────────────────────────────────────────
// Incoming Upload Routes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Registers an HTTP POST endpoint for receiving uploaded files.
 *
 * Example:
 *     FileGate_RegisterRoute("/map", "scriptfiles/maps/", ".map,.json", 25);
 *
 * @param endpoint    HTTP route path (example: "/map")
 * @param path        Destination directory on server
 * @param allowedExts Comma-separated allowed file extensions
 * @param maxSizeMb   Maximum allowed file size in MB
 * @return routeId, or -1 on failure
 */
native FileGate_RegisterRoute(const endpoint[], const path[], const allowedExts[], maxSizeMb);

/**
 * Requires "Authorization: Bearer <key>" for a specific route.
 *
 * @param routeId Route ID
 * @param key Authorization key
 * @return true on success
 */
native bool:FileGate_AddKey(routeId, const key[]);

/**
 * Removes a previously allowed authorization key from a route.
 *
 * @param routeId Route ID
 * @param key Authorization key
 * @return true on success
 */
native bool:FileGate_RemoveKey(routeId, const key[]);

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
native bool:FileGate_SetConflict(routeId, mode);

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
native bool:FileGate_SetCorruptAction(routeId, action);

/**
 * Requires incoming uploads on this route to include X-File-CRC32 header.
 *
 * @param routeId Route ID
 * @param required true to require CRC32, false to disable requirement
 * @return true on success
 */
native bool:FileGate_SetRequireCRC32(routeId, bool:required);

/**
 * Removes an existing upload route.
 *
 * @param routeId Route ID
 * @return true on success
 */
native bool:FileGate_RemoveRoute(routeId);

// ─────────────────────────────────────────────────────────────────────────────
// REST API Routes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Registers a custom REST API endpoint.
 *
 * Example:
 *     FileGate_Route(HTTP_GET, "/api/players", "OnGetPlayers");
 *     FileGate_Route(HTTP_POST, "/api/ban", "OnPostBan");
 *     FileGate_Route(HTTP_GET, "/api/player/{id}", "OnGetPlayer");
 *
 * @param method HTTP method (HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE)
 * @param endpoint Route path (supports {param} placeholders)
 * @param callback Pawn callback function name
 * @return routeId, or -1 on failure
 */
native FileGate_Route(method, const endpoint[], const callback[]);

/**
 * Removes a custom API route.
 *
 * @param routeId Route ID
 * @return true on success
 */
native bool:FileGate_RemoveAPIRoute(routeId);

/**
 * Sets authorization key for an API route.
 * Requires "Authorization: Bearer <key>" header.
 *
 * @param routeId Route ID
 * @param key Authorization key
 * @return true on success
 */
native bool:FileGate_SetRouteAuth(routeId, const key[]);

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
native FileGate_GetRequestIP(requestId, output[], outputSize);

/**
 * Gets the HTTP method of the request.
 *
 * @param requestId Request ID
 * @return HTTP method constant (HTTP_GET, HTTP_POST, etc.) or -1 on error
 */
native FileGate_GetRequestMethod(requestId);

/**
 * Gets the request path.
 *
 * @param requestId Request ID
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native FileGate_GetRequestPath(requestId, output[], outputSize);

/**
 * Gets the raw request body.
 *
 * @param requestId Request ID
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return Body length
 */
native FileGate_GetRequestBody(requestId, output[], outputSize);

/**
 * Gets the length of the request body.
 *
 * @param requestId Request ID
 * @return Body length in bytes
 */
native FileGate_GetRequestBodyLength(requestId);

/**
 * Gets a URL parameter value (e.g., {id} from /api/player/{id}).
 *
 * @param requestId Request ID
 * @param name Parameter name
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native FileGate_GetParam(requestId, const name[], output[], outputSize);

/**
 * Gets a URL parameter as integer.
 *
 * @param requestId Request ID
 * @param name Parameter name
 * @return Parameter value or 0 if not found
 */
native FileGate_GetParamInt(requestId, const name[]);

/**
 * Gets a query string parameter (e.g., ?page=1&limit=10).
 *
 * @param requestId Request ID
 * @param name Query parameter name
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native FileGate_GetQuery(requestId, const name[], output[], outputSize);

/**
 * Gets a query string parameter as integer.
 *
 * @param requestId Request ID
 * @param name Query parameter name
 * @param defaultValue Default value if not found
 * @return Parameter value or defaultValue
 */
native FileGate_GetQueryInt(requestId, const name[], defaultValue = 0);

/**
 * Gets a request header value.
 *
 * @param requestId Request ID
 * @param name Header name
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native FileGate_GetHeader(requestId, const name[], output[], outputSize);

// ─────────────────────────────────────────────────────────────────────────────
// REST API - JSON Parsing (Request Body)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Gets a string value from JSON request body.
 *
 * @param requestId Request ID
 * @param key JSON key
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native FileGate_JsonGetString(requestId, const key[], output[], outputSize);

/**
 * Gets an integer value from JSON request body.
 *
 * @param requestId Request ID
 * @param key JSON key
 * @param defaultValue Default value if not found
 * @return Value or defaultValue
 */
native FileGate_JsonGetInt(requestId, const key[], defaultValue = 0);

/**
 * Gets a float value from JSON request body.
 *
 * @param requestId Request ID
 * @param key JSON key
 * @param defaultValue Default value if not found
 * @return Value or defaultValue
 */
native Float:FileGate_JsonGetFloat(requestId, const key[], Float:defaultValue = 0.0);

/**
 * Gets a boolean value from JSON request body.
 *
 * @param requestId Request ID
 * @param key JSON key
 * @param defaultValue Default value if not found
 * @return Value or defaultValue
 */
native bool:FileGate_JsonGetBool(requestId, const key[], bool:defaultValue = false);

/**
 * Checks if a key exists in JSON request body.
 *
 * @param requestId Request ID
 * @param key JSON key
 * @return true if key exists
 */
native bool:FileGate_JsonHasKey(requestId, const key[]);

/**
 * Gets the length of a JSON array.
 *
 * @param requestId Request ID
 * @param key JSON key (array key, or empty for root array)
 * @return Array length, or -1 if not an array
 */
native FileGate_JsonArrayLength(requestId, const key[]);

/**
 * Gets a nested string value using dot notation.
 * Supports: "user.profile.name" or "items[0].id"
 *
 * @param requestId Request ID
 * @param path Dot-notation path
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native FileGate_JsonGetNested(requestId, const path[], output[], outputSize);

/**
 * Gets a nested integer value using dot notation.
 *
 * @param requestId Request ID
 * @param path Dot-notation path
 * @param defaultValue Default value if not found
 * @return Value or defaultValue
 */
native FileGate_JsonGetNestedInt(requestId, const path[], defaultValue = 0);

// ─────────────────────────────────────────────────────────────────────────────
// REST API - Response
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Sends a response to the client.
 *
 * @param requestId Request ID
 * @param status HTTP status code
 * @param body Response body
 * @param contentType Content-Type header (default: "application/json")
 * @return true on success
 */
native bool:FileGate_Respond(requestId, status, const body[], const contentType[] = "application/json");

/**
 * Sends a JSON response.
 *
 * @param requestId Request ID
 * @param status HTTP status code
 * @param json JSON string
 * @return true on success
 */
native bool:FileGate_RespondJSON(requestId, status, const json[]);

/**
 * Sends an error response.
 *
 * @param requestId Request ID
 * @param status HTTP status code
 * @param message Error message
 * @return true on success
 */
native bool:FileGate_RespondError(requestId, status, const message[]);

/**
 * Sets a response header (call before Respond).
 *
 * @param requestId Request ID
 * @param name Header name
 * @param value Header value
 * @return true on success
 */
native bool:FileGate_SetResponseHeader(requestId, const name[], const value[]);

// ─────────────────────────────────────────────────────────────────────────────
// REST API - JSON Builder (Response)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Starts building a JSON response.
 *
 * @param requestId Request ID
 * @return true on success
 */
native bool:FileGate_JsonStart(requestId);

/**
 * Adds a string field to JSON response.
 *
 * @param requestId Request ID
 * @param key Field key
 * @param value Field value
 * @return true on success
 */
native bool:FileGate_JsonAddString(requestId, const key[], const value[]);

/**
 * Adds an integer field to JSON response.
 *
 * @param requestId Request ID
 * @param key Field key
 * @param value Field value
 * @return true on success
 */
native bool:FileGate_JsonAddInt(requestId, const key[], value);

/**
 * Adds a float field to JSON response.
 *
 * @param requestId Request ID
 * @param key Field key
 * @param value Field value
 * @return true on success
 */
native bool:FileGate_JsonAddFloat(requestId, const key[], Float:value);

/**
 * Adds a boolean field to JSON response.
 *
 * @param requestId Request ID
 * @param key Field key
 * @param value Field value
 * @return true on success
 */
native bool:FileGate_JsonAddBool(requestId, const key[], bool:value);

/**
 * Adds a null field to JSON response.
 *
 * @param requestId Request ID
 * @param key Field key
 * @return true on success
 */
native bool:FileGate_JsonAddNull(requestId, const key[]);

/**
 * Starts a nested JSON object.
 *
 * @param requestId Request ID
 * @param key Object key (empty for array element)
 * @return true on success
 */
native bool:FileGate_JsonStartObject(requestId, const key[]);

/**
 * Ends a nested JSON object.
 *
 * @param requestId Request ID
 * @return true on success
 */
native bool:FileGate_JsonEndObject(requestId);

/**
 * Starts a JSON array.
 *
 * @param requestId Request ID
 * @param key Array key
 * @return true on success
 */
native bool:FileGate_JsonStartArray(requestId, const key[]);

/**
 * Ends a JSON array.
 *
 * @param requestId Request ID
 * @return true on success
 */
native bool:FileGate_JsonEndArray(requestId);

/**
 * Finalizes and sends the JSON response.
 *
 * @param requestId Request ID
 * @param status HTTP status code (default: 200)
 * @return true on success
 */
native bool:FileGate_JsonSend(requestId, status = 200);

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
native bool:FileGate_AllowList(routeId, bool:allow);

/**
 * Allow downloading files via REST API.
 * Endpoint: GET {route}/files/{filename}
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:FileGate_AllowDownload(routeId, bool:allow);

/**
 * Allow deleting files via REST API.
 * Endpoint: DELETE {route}/files/{filename}
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:FileGate_AllowDelete(routeId, bool:allow);

/**
 * Allow getting file info via REST API.
 * Endpoint: GET {route}/files/{filename}/info
 *
 * @param routeId Route ID
 * @param allow true to allow, false to deny
 * @return true on success
 */
native bool:FileGate_AllowInfo(routeId, bool:allow);

// ─────────────────────────────────────────────────────────────────────────────
// File Operations (Pawn-side)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Gets the number of files in a route's directory.
 *
 * @param routeId Route ID
 * @return Number of files, or -1 on error
 */
native FileGate_GetFileCount(routeId);

/**
 * Gets a filename by index from a route's directory.
 *
 * @param routeId Route ID
 * @param index File index (0-based)
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 if not found
 */
native FileGate_GetFileName(routeId, index, output[], outputSize);

/**
 * Deletes a file from a route's directory.
 *
 * @param routeId Route ID
 * @param filename Filename to delete
 * @return true on success
 */
native bool:FileGate_DeleteFile(routeId, const filename[]);

/**
 * Gets the size of a file in a route's directory.
 *
 * @param routeId Route ID
 * @param filename Filename
 * @return File size in bytes, or 0 if not found
 */
native FileGate_GetFileSize(routeId, const filename[]);

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
 * @return uploadId, or -1 on failure
 */
native FileGate_UploadFile(
    const url[],
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
native bool:FileGate_CancelUpload(uploadId);

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
native FileGate_GetUploadStatus(uploadId);

/**
 * Returns the current progress percentage of an outgoing upload.
 *
 * @param uploadId Upload ID
 * @return Progress from 0 to 100
 */
native FileGate_GetUploadProgress(uploadId);

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
native FileGate_GetUploadResponse(uploadId, output[], outputSize);

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
native FileGate_VerifyCRC32(const filepath[], const expectedCrc[]);

/**
 * Calculates the CRC32 checksum of a local file.
 *
 * @param filepath Local file path
 * @param output Output buffer
 * @param outputSize Buffer size
 * @return 1 on success, 0 on failure
 */
native FileGate_GetFileCRC32(const filepath[], output[], outputSize);

/**
 * Compares two local files using CRC32.
 *
 * @param path1 First file path
 * @param path2 Second file path
 * @return 1 if identical, 0 if different, -1 on error
 */
native FileGate_CompareFiles(const path1[], const path2[]);

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
