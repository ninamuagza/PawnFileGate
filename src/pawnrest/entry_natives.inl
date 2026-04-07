COMPONENT_ENTRY_POINT()
{
    return new PawnRESTComponent();
}

// ═══════════════════════════════════════════════════════════════════════════
// PAWN NATIVES
// ═══════════════════════════════════════════════════════════════════════════

static bool ReadAmxStringArg(AMX* amx, cell arg, std::string& out)
{
    out.clear();
    if (!amx || arg == 0) return false;

    cell* phys = nullptr;
    if (amx_GetAddr(amx, arg, &phys) != AMX_ERR_NONE || !phys) return false;

    int length = 0;
    if (amx_StrLen(phys, &length) != AMX_ERR_NONE) return false;
    if (length <= 0) return true;

    std::string value(static_cast<size_t>(length) + 1, '\0');
    if (amx_GetString(value.data(), phys, 0, static_cast<size_t>(length + 1)) != AMX_ERR_NONE) {
        return false;
    }
    value.resize(static_cast<size_t>(length));
    out = std::move(value);
    return true;
}

cell AMX_NATIVE_CALL REST_JsonObjectVariadic(AMX* amx, const cell* params)
{
    auto c = GetComponent();
    if (!c) return -1;

    int objectId = c->JsonNodeObject();
    if (objectId < 0) return -1;
    if (!params) return objectId;

    int argc = static_cast<int>(params[0] / static_cast<cell>(sizeof(cell)));
    if (argc == 0) return objectId;
    if ((argc % 2) != 0) {
        c->ReleaseJsonNode(objectId);
        return -1;
    }

    std::vector<int> consumed;
    consumed.reserve(static_cast<size_t>(argc / 2));

    for (int i = 1; i <= argc; i += 2) {
        std::string key;
        if (!ReadAmxStringArg(amx, params[i], key) || key.empty()) {
            for (int nodeId : consumed) c->ReleaseJsonNode(nodeId);
            c->ReleaseJsonNode(objectId);
            return -1;
        }

        int valueNodeId = static_cast<int>(params[i + 1]);
        consumed.push_back(valueNodeId);
        if (!c->JsonNodeSet(objectId, key, valueNodeId)) {
            for (int nodeId : consumed) c->ReleaseJsonNode(nodeId);
            c->ReleaseJsonNode(objectId);
            return -1;
        }
    }

    for (int nodeId : consumed) c->ReleaseJsonNode(nodeId);
    return objectId;
}

cell AMX_NATIVE_CALL REST_JsonArrayVariadic(AMX* amx, const cell* params)
{
    (void)amx;

    auto c = GetComponent();
    if (!c) return -1;

    int arrayId = c->JsonNodeArray();
    if (arrayId < 0) return -1;
    if (!params) return arrayId;

    int argc = static_cast<int>(params[0] / static_cast<cell>(sizeof(cell)));
    if (argc == 0) return arrayId;

    std::vector<int> consumed;
    consumed.reserve(static_cast<size_t>(argc));

    for (int i = 1; i <= argc; ++i) {
        int valueNodeId = static_cast<int>(params[i]);
        consumed.push_back(valueNodeId);
        if (!c->JsonNodeArrayPush(arrayId, valueNodeId)) {
            for (int nodeId : consumed) c->ReleaseJsonNode(nodeId);
            c->ReleaseJsonNode(arrayId);
            return -1;
        }
    }

    for (int nodeId : consumed) c->ReleaseJsonNode(nodeId);
    return arrayId;
}

// Server Control
SCRIPT_API(REST_Start, bool(int port))
{
    auto c = GetComponent();
    if (!c) return false;
    if (port <= 0 || port > 65535) return false;
    return c->Start(port);
}

SCRIPT_API(REST_StartTLS, bool(int port, const std::string& certPath, const std::string& keyPath))
{
    auto c = GetComponent();
    if (!c) return false;
    if (port <= 0 || port > 65535) return false;
    return c->StartTLS(port, certPath, keyPath);
}

SCRIPT_API(REST_Stop, bool())
{
    auto c = GetComponent();
    if (!c) return false;
    return c->Stop();
}

SCRIPT_API(REST_IsRunning, int())
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->IsRunning() ? 1 : 0;
}

SCRIPT_API(REST_GetPort, int())
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetPort();
}

SCRIPT_API(REST_IsTLSEnabled, int())
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->IsTLSEnabled() ? 1 : 0;
}

// Receive Routes
SCRIPT_API(REST_RegisterRoute,
    int(const std::string& endpoint, const std::string& path,
        const std::string& allowedExts, int maxSizeMb))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->RegisterRoute(endpoint, path, allowedExts, maxSizeMb);
}

SCRIPT_API(REST_AddKey, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->AddKeyToRoute(routeId, key);
}

SCRIPT_API(REST_RemoveKey, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveKeyFromRoute(routeId, key);
}

SCRIPT_API(REST_SetConflict, bool(int routeId, int mode))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetConflictMode(routeId, mode);
}

SCRIPT_API(REST_SetCorruptAction, bool(int routeId, int action))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetCorruptAction(routeId, action);
}

SCRIPT_API(REST_SetRequireCRC32, bool(int routeId, bool required))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetRequireCRC32(routeId, required);
}

SCRIPT_API(REST_RemoveRoute, bool(int routeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveRoute(routeId);
}

// ═══════════════════════════════════════════════════════════════════════════
// REST API Natives
// ═══════════════════════════════════════════════════════════════════════════

SCRIPT_API(REST_Route, int(int method, const std::string& endpoint, const std::string& callback))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->RegisterApiRoute(method, endpoint, callback);
}

SCRIPT_API(REST_RemoveAPIRoute, bool(int routeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveApiRoute(routeId);
}

SCRIPT_API(REST_SetRouteAuthKey, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetApiRouteAuth(routeId, key);
}

// Request data access
SCRIPT_API(REST_GetRequestIP, int(int requestId, std::string& output))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string ip = c->GetRequestIP(requestId);
    output = ip;
    return ip.empty() ? 0 : 1;
}

SCRIPT_API(REST_GetRequestMethod, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetRequestMethod(requestId);
}

SCRIPT_API(REST_GetRequestPath, int(int requestId, std::string& output))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string path = c->GetRequestPath(requestId);
    output = path;
    return path.empty() ? 0 : 1;
}

SCRIPT_API(REST_GetRequestBody, int(int requestId, std::string& output))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string body = c->GetRequestBody(requestId);
    output = body;
    return static_cast<int>(body.size());
}

SCRIPT_API(REST_GetRequestBodyLength, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetRequestBodyLength(requestId);
}

// URL parameters
SCRIPT_API(REST_GetParam, int(int requestId, const std::string& name, std::string& output))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->GetParam(requestId, name);
    output = val;
    return val.empty() ? 0 : 1;
}

SCRIPT_API(REST_GetParamInt, int(int requestId, const std::string& name))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetParamInt(requestId, name);
}

// Query string
SCRIPT_API(REST_GetQuery, int(int requestId, const std::string& name, std::string& output))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->GetQuery(requestId, name);
    output = val;
    return val.empty() ? 0 : 1;
}

SCRIPT_API(REST_GetQueryInt, int(int requestId, const std::string& name, int defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->GetQueryInt(requestId, name, defaultValue);
}

// Headers
SCRIPT_API(REST_GetHeader, int(int requestId, const std::string& name, std::string& output))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->GetHeader(requestId, name);
    output = val;
    return val.empty() ? 0 : 1;
}

// JSON Node API
SCRIPT_API(REST_JsonParseNode, int(const std::string& json))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->ParseJsonNode(json);
}

SCRIPT_API(REST_RequestJsonNode, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->ParseRequestJsonNode(requestId);
}

SCRIPT_API(REST_NodeType, int(int nodeId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeType(nodeId);
}

SCRIPT_API(REST_NodeStringify, int(int nodeId, std::string& output))
{
    auto c = GetComponent();
    if (!c) { output.clear(); return 0; }
    return c->JsonNodeStringify(nodeId, output, INT_MAX) ? 1 : 0;
}

SCRIPT_API(REST_NodeRelease, bool(int nodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->ReleaseJsonNode(nodeId);
}

SCRIPT_API(REST_NodeObject, int())
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeObject();
}

SCRIPT_API(REST_NodeArray, int())
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeArray();
}

SCRIPT_API(REST_NodeString, int(const std::string& value))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeString(value);
}

SCRIPT_API(REST_NodeInt, int(int value))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeInt(value);
}

SCRIPT_API(REST_NodeFloat, int(float value))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeFloat(value);
}

SCRIPT_API(REST_NodeBool, int(bool value))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeBool(value);
}

SCRIPT_API(REST_NodeNull, int())
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeNull();
}

SCRIPT_API(REST_NodeSet, bool(int objectNodeId, const std::string& key, int valueNodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSet(objectNodeId, key, valueNodeId);
}

SCRIPT_API(REST_NodeSetString, bool(int objectNodeId, const std::string& key, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetString(objectNodeId, key, value);
}

SCRIPT_API(REST_NodeSetInt, bool(int objectNodeId, const std::string& key, int value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetInt(objectNodeId, key, value);
}

SCRIPT_API(REST_NodeSetFloat, bool(int objectNodeId, const std::string& key, float value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetFloat(objectNodeId, key, value);
}

SCRIPT_API(REST_NodeSetBool, bool(int objectNodeId, const std::string& key, bool value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetBool(objectNodeId, key, value);
}

SCRIPT_API(REST_NodeSetNull, bool(int objectNodeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetNull(objectNodeId, key);
}

SCRIPT_API(REST_NodeHas, bool(int objectNodeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeHas(objectNodeId, key);
}

SCRIPT_API(REST_NodeGet, int(int objectNodeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeGet(objectNodeId, key);
}

SCRIPT_API(REST_NodeGetString, int(int objectNodeId, const std::string& key, std::string& output))
{
    auto c = GetComponent();
    if (!c) { output.clear(); return 0; }
    std::string value = c->JsonNodeGetString(objectNodeId, key, "");
    output = value;
    return value.empty() ? 0 : 1;
}

SCRIPT_API(REST_NodeGetInt, int(int objectNodeId, const std::string& key, int defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->JsonNodeGetInt(objectNodeId, key, defaultValue);
}

SCRIPT_API(REST_NodeGetFloat, float(int objectNodeId, const std::string& key, float defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->JsonNodeGetFloat(objectNodeId, key, defaultValue);
}

SCRIPT_API(REST_NodeGetBool, bool(int objectNodeId, const std::string& key, bool defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->JsonNodeGetBool(objectNodeId, key, defaultValue);
}

SCRIPT_API(REST_NodeArrayLength, int(int arrayNodeId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeArrayLength(arrayNodeId);
}

SCRIPT_API(REST_NodeArrayGet, int(int arrayNodeId, int index))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeArrayGet(arrayNodeId, index);
}

SCRIPT_API(REST_NodeArrayPush, bool(int arrayNodeId, int valueNodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPush(arrayNodeId, valueNodeId);
}

SCRIPT_API(REST_NodeArrayPushString, bool(int arrayNodeId, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushString(arrayNodeId, value);
}

SCRIPT_API(REST_NodeArrayPushInt, bool(int arrayNodeId, int value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushInt(arrayNodeId, value);
}

SCRIPT_API(REST_NodeArrayPushFloat, bool(int arrayNodeId, float value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushFloat(arrayNodeId, value);
}

SCRIPT_API(REST_NodeArrayPushBool, bool(int arrayNodeId, bool value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushBool(arrayNodeId, value);
}

SCRIPT_API(REST_NodeArrayPushNull, bool(int arrayNodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushNull(arrayNodeId);
}

SCRIPT_API(REST_JsonAppend, int(int leftNodeId, int rightNodeId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonAppend(leftNodeId, rightNodeId);
}

// Response methods
SCRIPT_API(REST_Respond, bool(int requestId, int status, const std::string& body, const std::string& contentType))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->Respond(requestId, status, body, contentType);
}

SCRIPT_API(REST_RespondJSON, bool(int requestId, int status, const std::string& json))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RespondJSON(requestId, status, json);
}

SCRIPT_API(REST_RespondError, bool(int requestId, int status, const std::string& message))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RespondError(requestId, status, message);
}

SCRIPT_API(REST_RespondNode, bool(int requestId, int status, int nodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RespondNode(requestId, status, nodeId);
}

SCRIPT_API(REST_SetResponseHeader, bool(int requestId, const std::string& name, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetResponseHeader(requestId, name, value);
}

// File Route Permission Natives
SCRIPT_API(REST_AllowList, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowList(routeId, allow);
}

SCRIPT_API(REST_AllowDownload, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowDownload(routeId, allow);
}

SCRIPT_API(REST_AllowDelete, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowDelete(routeId, allow);
}

SCRIPT_API(REST_AllowInfo, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowInfo(routeId, allow);
}

// File Operation Natives
SCRIPT_API(REST_GetFileCount, int(int routeId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetRouteFileCount(routeId);
}

SCRIPT_API(REST_GetFileName, int(int routeId, int index, std::string& output))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string name = c->GetRouteFileName(routeId, index);
    output = name;
    return name.empty() ? 0 : 1;
}

SCRIPT_API(REST_DeleteFile, bool(int routeId, const std::string& filename))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->DeleteRouteFile(routeId, filename);
}

SCRIPT_API(REST_GetFileSize, int(int routeId, const std::string& filename))
{
    auto c = GetComponent();
    if (!c) return 0;
    return static_cast<int>(c->GetRouteFileSize(routeId, filename));
}

// Upload (Client) Natives
// mode: 0 = multipart, 1 = raw
SCRIPT_API(REST_UploadFile,
    int(const std::string& url, const std::string& filepath,
        const std::string& filename, const std::string& authKey,
        const std::string& customHeaders, int calculateCrc32, int mode, int verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->QueueUpload(url, filepath, filename, authKey, customHeaders, calculateCrc32 != 0, mode, verifyTls != 0);
}

SCRIPT_API(REST_CreateUploadClient, int(const std::string& baseUrl, const std::string& defaultHeaders, bool verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->CreateUploadClient(baseUrl, defaultHeaders, verifyTls);
}

SCRIPT_API(REST_RemoveUploadClient, bool(int clientId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveUploadClient(clientId);
}

SCRIPT_API(REST_SetUploadClientHeader, bool(int clientId, const std::string& name, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetUploadClientHeader(clientId, name, value);
}

SCRIPT_API(REST_RemoveUploadClientHeader, bool(int clientId, const std::string& name))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveUploadClientHeader(clientId, name);
}

SCRIPT_API(REST_UploadFileWithClient,
    int(int clientId, const std::string& path, const std::string& filepath,
        const std::string& filename, const std::string& authKey,
        const std::string& customHeaders, int calculateCrc32, int mode))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->QueueUploadWithClient(clientId, path, filepath, filename, authKey, customHeaders, calculateCrc32 != 0, mode);
}

SCRIPT_API(REST_CancelUpload, bool(int uploadId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->CancelUpload(uploadId);
}

SCRIPT_API(REST_GetUploadStatus, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetUploadStatus(uploadId);
}

SCRIPT_API(REST_GetUploadProgress, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetUploadProgress(uploadId);
}

SCRIPT_API(REST_GetUploadResponse, int(int uploadId, std::string& output))
{
    auto c = GetComponent();
    if (!c) {
        output = "";
        return 0;
    }
    return c->GetUploadResponse(uploadId, output, INT_MAX) ? 1 : 0;
}

SCRIPT_API(REST_GetUploadErrorCode, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetUploadErrorCode(uploadId);
}

SCRIPT_API(REST_GetUploadErrorType, int(int uploadId, std::string& output))
{
    auto c = GetComponent();
    if (!c) {
        output.clear();
        return 0;
    }
    return c->GetUploadErrorType(uploadId, output, INT_MAX) ? 1 : 0;
}

SCRIPT_API(REST_GetUploadHttpStatus, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetUploadHttpStatus(uploadId);
}

// Outbound Request (Requests-style) Natives
SCRIPT_API(REST_RequestsClient, int(const std::string& endpoint, const std::string& defaultHeaders, int verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->CreateRequestClient(endpoint, defaultHeaders, verifyTls != 0);
}

SCRIPT_API(REST_RemoveRequestsClient, bool(int clientId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveRequestClient(clientId);
}

SCRIPT_API(REST_SetRequestsClientHeader, bool(int clientId, const std::string& name, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetRequestClientHeader(clientId, name, value);
}

SCRIPT_API(REST_RemoveRequestsClientHeader, bool(int clientId, const std::string& name))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveRequestClientHeader(clientId, name);
}

SCRIPT_API(REST_Request,
    int(int clientId, const std::string& path, int method, const std::string& callback, const std::string& body, const std::string& customHeaders))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->QueueOutboundRequest(clientId, path, method, callback, body, customHeaders, false);
}

SCRIPT_API(REST_RequestJSON,
    int(int clientId, const std::string& path, int method, const std::string& callback, int jsonNodeId, const std::string& customHeaders))
{
    auto c = GetComponent();
    if (!c) return -1;

    std::string body;
    if (jsonNodeId >= 0) {
        if (!c->JsonNodeStringify(jsonNodeId, body, static_cast<int>(INT_MAX))) {
            return -1;
        }
    }

    return c->QueueOutboundRequest(clientId, path, method, callback, body, customHeaders, true);
}

SCRIPT_API(REST_CancelRequest, bool(int requestId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->CancelOutboundRequest(requestId);
}

SCRIPT_API(REST_GetRequestStatus, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetOutboundRequestStatus(requestId);
}

SCRIPT_API(REST_GetRequestHttpStatus, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetOutboundRequestHttpStatus(requestId);
}

SCRIPT_API(REST_GetRequestErrorCode, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetOutboundRequestErrorCode(requestId);
}

SCRIPT_API(REST_GetRequestErrorType, int(int requestId, std::string& output))
{
    auto c = GetComponent();
    if (!c) {
        output.clear();
        return 0;
    }
    return c->GetOutboundRequestErrorType(requestId, output, INT_MAX) ? 1 : 0;
}

SCRIPT_API(REST_GetRequestResponse, int(int requestId, std::string& output))
{
    auto c = GetComponent();
    if (!c) {
        output.clear();
        return 0;
    }
    return c->GetOutboundRequestResponse(requestId, output, INT_MAX) ? 1 : 0;
}

// WebSocket Client Natives
SCRIPT_API(REST_WebSocketClient, int(const std::string& address, const std::string& callback, const std::string& headers, int verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->ConnectWebSocketClient(address, callback, false, headers, verifyTls != 0);
}

SCRIPT_API(REST_JsonWebSocketClient, int(const std::string& address, const std::string& callback, const std::string& headers, int verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->ConnectWebSocketClient(address, callback, true, headers, verifyTls != 0);
}

SCRIPT_API(REST_WebSocketSend, bool(int socketId, const std::string& data))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->WebSocketSendText(socketId, data);
}

SCRIPT_API(REST_JsonWebSocketSend, bool(int socketId, int nodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->WebSocketSendJson(socketId, nodeId);
}

SCRIPT_API(REST_WebSocketClose, bool(int socketId, int status, const std::string& reason))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->CloseWebSocketClient(socketId, status, reason);
}

SCRIPT_API(REST_RemoveWebSocketClient, bool(int socketId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveWebSocketClient(socketId);
}

SCRIPT_API(REST_IsWebSocketOpen, bool(int socketId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->IsWebSocketOpen(socketId);
}

// CRC32 Utilities
SCRIPT_API(REST_VerifyCRC32, int(const std::string& filepath, const std::string& expectedCrc))
{
    auto c = GetComponent();
    if (!c) return -1;

    std::string base = FileUtils::GetCurrentWorkingDirectory();
    std::string fullPath = base + filepath;

    if (!FileUtils::FileExists(fullPath)) return -1;

    uint32_t calculated = CRC32::fileChecksum(fullPath);
    uint32_t expected = CRC32::fromHex(expectedCrc);

    return (calculated == expected) ? 1 : 0;
}

SCRIPT_API(REST_GetFileCRC32, int(const std::string& filepath, std::string& output))
{
    auto c = GetComponent();
    if (!c) {
        output = "0";
        return 0;
    }

    std::string base = FileUtils::GetCurrentWorkingDirectory();
    std::string fullPath = base + filepath;

    if (!FileUtils::FileExists(fullPath)) {
        output = "0";
        return 0;
    }

    uint32_t crc = CRC32::fileChecksum(fullPath);
    std::string hex = CRC32::toHex(crc);

    output = hex;
    return 1;
}

SCRIPT_API(REST_CompareFiles, int(const std::string& path1, const std::string& path2))
{
    auto c = GetComponent();
    if (!c) return -1;

    std::string base = FileUtils::GetCurrentWorkingDirectory();
    std::string fullPath1 = base + path1;
    std::string fullPath2 = base + path2;

    if (!FileUtils::FileExists(fullPath1) || !FileUtils::FileExists(fullPath2)) return -1;

    uint32_t crc1 = CRC32::fileChecksum(fullPath1);
    uint32_t crc2 = CRC32::fileChecksum(fullPath2);

    if (crc1 == 0 || crc2 == 0) return -1;
    return (crc1 == crc2) ? 1 : 0;
}
