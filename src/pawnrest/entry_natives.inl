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

cell AMX_NATIVE_CALL PawnREST_JsonObjectVariadic(AMX* amx, const cell* params)
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

cell AMX_NATIVE_CALL PawnREST_JsonArrayVariadic(AMX* amx, const cell* params)
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
SCRIPT_API(PawnREST_Start, bool(int port))
{
    auto c = GetComponent();
    if (!c) return false;
    if (port <= 0 || port > 65535) return false;
    return c->Start(port);
}

SCRIPT_API(PawnREST_StartTLS, bool(int port, const std::string& certPath, const std::string& keyPath))
{
    auto c = GetComponent();
    if (!c) return false;
    if (port <= 0 || port > 65535) return false;
    return c->StartTLS(port, certPath, keyPath);
}

SCRIPT_API(PawnREST_Stop, bool())
{
    auto c = GetComponent();
    if (!c) return false;
    return c->Stop();
}

SCRIPT_API(PawnREST_IsRunning, int())
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->IsRunning() ? 1 : 0;
}

SCRIPT_API(PawnREST_GetPort, int())
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetPort();
}

SCRIPT_API(PawnREST_IsTLSEnabled, int())
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->IsTLSEnabled() ? 1 : 0;
}

// Receive Routes
SCRIPT_API(PawnREST_RegisterRoute,
    int(const std::string& endpoint, const std::string& path,
        const std::string& allowedExts, int maxSizeMb))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->RegisterRoute(endpoint, path, allowedExts, maxSizeMb);
}

SCRIPT_API(PawnREST_AddKey, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->AddKeyToRoute(routeId, key);
}

SCRIPT_API(PawnREST_RemoveKey, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveKeyFromRoute(routeId, key);
}

SCRIPT_API(PawnREST_SetConflict, bool(int routeId, int mode))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetConflictMode(routeId, mode);
}

SCRIPT_API(PawnREST_SetCorruptAction, bool(int routeId, int action))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetCorruptAction(routeId, action);
}

SCRIPT_API(PawnREST_SetRequireCRC32, bool(int routeId, bool required))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetRequireCRC32(routeId, required);
}

SCRIPT_API(PawnREST_RemoveRoute, bool(int routeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveRoute(routeId);
}

// ═══════════════════════════════════════════════════════════════════════════
// REST API Natives
// ═══════════════════════════════════════════════════════════════════════════

SCRIPT_API(PawnREST_Route, int(int method, const std::string& endpoint, const std::string& callback))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->RegisterApiRoute(method, endpoint, callback);
}

SCRIPT_API(PawnREST_RemoveAPIRoute, bool(int routeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveApiRoute(routeId);
}

SCRIPT_API(PawnREST_SetRouteAuth, bool(int routeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetApiRouteAuth(routeId, key);
}

// Request data access
SCRIPT_API(PawnREST_GetRequestIP, int(int requestId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string ip = c->GetRequestIP(requestId);
    output = ip.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return ip.empty() ? 0 : 1;
}

SCRIPT_API(PawnREST_GetRequestMethod, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetRequestMethod(requestId);
}

SCRIPT_API(PawnREST_GetRequestPath, int(int requestId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string path = c->GetRequestPath(requestId);
    output = path.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return path.empty() ? 0 : 1;
}

SCRIPT_API(PawnREST_GetRequestBody, int(int requestId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string body = c->GetRequestBody(requestId);
    output = body.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return static_cast<int>(body.size());
}

SCRIPT_API(PawnREST_GetRequestBodyLength, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetRequestBodyLength(requestId);
}

// URL parameters
SCRIPT_API(PawnREST_GetParam, int(int requestId, const std::string& name, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->GetParam(requestId, name);
    output = val.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return val.empty() ? 0 : 1;
}

SCRIPT_API(PawnREST_GetParamInt, int(int requestId, const std::string& name))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetParamInt(requestId, name);
}

// Query string
SCRIPT_API(PawnREST_GetQuery, int(int requestId, const std::string& name, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->GetQuery(requestId, name);
    output = val.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return val.empty() ? 0 : 1;
}

SCRIPT_API(PawnREST_GetQueryInt, int(int requestId, const std::string& name, int defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->GetQueryInt(requestId, name, defaultValue);
}

// Headers
SCRIPT_API(PawnREST_GetHeader, int(int requestId, const std::string& name, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string val = c->GetHeader(requestId, name);
    output = val.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return val.empty() ? 0 : 1;
}

// JSON Node API
SCRIPT_API(PawnREST_JsonParseNode, int(const std::string& json))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->ParseJsonNode(json);
}

SCRIPT_API(PawnREST_RequestJsonNode, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->ParseRequestJsonNode(requestId);
}

SCRIPT_API(PawnREST_NodeType, int(int nodeId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeType(nodeId);
}

SCRIPT_API(PawnREST_NodeStringify, int(int nodeId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output.clear(); return 0; }
    return c->JsonNodeStringify(nodeId, output, outputSize) ? 1 : 0;
}

SCRIPT_API(PawnREST_NodeRelease, bool(int nodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->ReleaseJsonNode(nodeId);
}

SCRIPT_API(PawnREST_NodeObject, int())
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeObject();
}

SCRIPT_API(PawnREST_NodeArray, int())
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeArray();
}

SCRIPT_API(PawnREST_NodeString, int(const std::string& value))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeString(value);
}

SCRIPT_API(PawnREST_NodeInt, int(int value))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeInt(value);
}

SCRIPT_API(PawnREST_NodeFloat, int(float value))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeFloat(value);
}

SCRIPT_API(PawnREST_NodeBool, int(bool value))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeBool(value);
}

SCRIPT_API(PawnREST_NodeNull, int())
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeNull();
}

SCRIPT_API(PawnREST_NodeSet, bool(int objectNodeId, const std::string& key, int valueNodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSet(objectNodeId, key, valueNodeId);
}

SCRIPT_API(PawnREST_NodeSetString, bool(int objectNodeId, const std::string& key, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetString(objectNodeId, key, value);
}

SCRIPT_API(PawnREST_NodeSetInt, bool(int objectNodeId, const std::string& key, int value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetInt(objectNodeId, key, value);
}

SCRIPT_API(PawnREST_NodeSetFloat, bool(int objectNodeId, const std::string& key, float value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetFloat(objectNodeId, key, value);
}

SCRIPT_API(PawnREST_NodeSetBool, bool(int objectNodeId, const std::string& key, bool value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetBool(objectNodeId, key, value);
}

SCRIPT_API(PawnREST_NodeSetNull, bool(int objectNodeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeSetNull(objectNodeId, key);
}

SCRIPT_API(PawnREST_NodeHas, bool(int objectNodeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeHas(objectNodeId, key);
}

SCRIPT_API(PawnREST_NodeGet, int(int objectNodeId, const std::string& key))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeGet(objectNodeId, key);
}

SCRIPT_API(PawnREST_NodeGetString, int(int objectNodeId, const std::string& key, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output.clear(); return 0; }
    std::string value = c->JsonNodeGetString(objectNodeId, key, "");
    output = value.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return value.empty() ? 0 : 1;
}

SCRIPT_API(PawnREST_NodeGetInt, int(int objectNodeId, const std::string& key, int defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->JsonNodeGetInt(objectNodeId, key, defaultValue);
}

SCRIPT_API(PawnREST_NodeGetFloat, float(int objectNodeId, const std::string& key, float defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->JsonNodeGetFloat(objectNodeId, key, defaultValue);
}

SCRIPT_API(PawnREST_NodeGetBool, bool(int objectNodeId, const std::string& key, bool defaultValue))
{
    auto c = GetComponent();
    if (!c) return defaultValue;
    return c->JsonNodeGetBool(objectNodeId, key, defaultValue);
}

SCRIPT_API(PawnREST_NodeArrayLength, int(int arrayNodeId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeArrayLength(arrayNodeId);
}

SCRIPT_API(PawnREST_NodeArrayGet, int(int arrayNodeId, int index))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonNodeArrayGet(arrayNodeId, index);
}

SCRIPT_API(PawnREST_NodeArrayPush, bool(int arrayNodeId, int valueNodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPush(arrayNodeId, valueNodeId);
}

SCRIPT_API(PawnREST_NodeArrayPushString, bool(int arrayNodeId, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushString(arrayNodeId, value);
}

SCRIPT_API(PawnREST_NodeArrayPushInt, bool(int arrayNodeId, int value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushInt(arrayNodeId, value);
}

SCRIPT_API(PawnREST_NodeArrayPushFloat, bool(int arrayNodeId, float value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushFloat(arrayNodeId, value);
}

SCRIPT_API(PawnREST_NodeArrayPushBool, bool(int arrayNodeId, bool value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushBool(arrayNodeId, value);
}

SCRIPT_API(PawnREST_NodeArrayPushNull, bool(int arrayNodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->JsonNodeArrayPushNull(arrayNodeId);
}

SCRIPT_API(PawnREST_JsonAppend, int(int leftNodeId, int rightNodeId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->JsonAppend(leftNodeId, rightNodeId);
}

// Response methods
SCRIPT_API(PawnREST_Respond, bool(int requestId, int status, const std::string& body, const std::string& contentType))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->Respond(requestId, status, body, contentType);
}

SCRIPT_API(PawnREST_RespondJSON, bool(int requestId, int status, const std::string& json))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RespondJSON(requestId, status, json);
}

SCRIPT_API(PawnREST_RespondError, bool(int requestId, int status, const std::string& message))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RespondError(requestId, status, message);
}

SCRIPT_API(PawnREST_RespondNode, bool(int requestId, int status, int nodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RespondNode(requestId, status, nodeId);
}

SCRIPT_API(PawnREST_SetResponseHeader, bool(int requestId, const std::string& name, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetResponseHeader(requestId, name, value);
}

// File Route Permission Natives
SCRIPT_API(PawnREST_AllowList, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowList(routeId, allow);
}

SCRIPT_API(PawnREST_AllowDownload, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowDownload(routeId, allow);
}

SCRIPT_API(PawnREST_AllowDelete, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowDelete(routeId, allow);
}

SCRIPT_API(PawnREST_AllowInfo, bool(int routeId, bool allow))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetAllowInfo(routeId, allow);
}

// File Operation Natives
SCRIPT_API(PawnREST_GetFileCount, int(int routeId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetRouteFileCount(routeId);
}

SCRIPT_API(PawnREST_GetFileName, int(int routeId, int index, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) { output = ""; return 0; }
    std::string name = c->GetRouteFileName(routeId, index);
    output = name.substr(0, outputSize > 0 ? outputSize - 1 : 0);
    return name.empty() ? 0 : 1;
}

SCRIPT_API(PawnREST_DeleteFile, bool(int routeId, const std::string& filename))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->DeleteRouteFile(routeId, filename);
}

SCRIPT_API(PawnREST_GetFileSize, int(int routeId, const std::string& filename))
{
    auto c = GetComponent();
    if (!c) return 0;
    return static_cast<int>(c->GetRouteFileSize(routeId, filename));
}

// Upload (Client) Natives
// mode: 0 = multipart, 1 = raw
SCRIPT_API(PawnREST_UploadFile,
    int(const std::string& url, const std::string& filepath,
        const std::string& filename, const std::string& authKey,
        const std::string& customHeaders, int calculateCrc32, int mode, int verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->QueueUpload(url, filepath, filename, authKey, customHeaders, calculateCrc32 != 0, mode, verifyTls != 0);
}

SCRIPT_API(PawnREST_CreateUploadClient, int(const std::string& baseUrl, const std::string& defaultHeaders, bool verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->CreateUploadClient(baseUrl, defaultHeaders, verifyTls);
}

SCRIPT_API(PawnREST_RemoveUploadClient, bool(int clientId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveUploadClient(clientId);
}

SCRIPT_API(PawnREST_SetUploadClientHeader, bool(int clientId, const std::string& name, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetUploadClientHeader(clientId, name, value);
}

SCRIPT_API(PawnREST_RemoveUploadClientHeader, bool(int clientId, const std::string& name))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveUploadClientHeader(clientId, name);
}

SCRIPT_API(PawnREST_UploadFileWithClient,
    int(int clientId, const std::string& path, const std::string& filepath,
        const std::string& filename, const std::string& authKey,
        const std::string& customHeaders, int calculateCrc32, int mode))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->QueueUploadWithClient(clientId, path, filepath, filename, authKey, customHeaders, calculateCrc32 != 0, mode);
}

SCRIPT_API(PawnREST_CancelUpload, bool(int uploadId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->CancelUpload(uploadId);
}

SCRIPT_API(PawnREST_GetUploadStatus, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetUploadStatus(uploadId);
}

SCRIPT_API(PawnREST_GetUploadProgress, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetUploadProgress(uploadId);
}

SCRIPT_API(PawnREST_GetUploadResponse, int(int uploadId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) {
        output = "";
        return 0;
    }
    return c->GetUploadResponse(uploadId, output, outputSize) ? 1 : 0;
}

SCRIPT_API(PawnREST_GetUploadErrorCode, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetUploadErrorCode(uploadId);
}

SCRIPT_API(PawnREST_GetUploadErrorType, int(int uploadId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) {
        output.clear();
        return 0;
    }
    return c->GetUploadErrorType(uploadId, output, outputSize) ? 1 : 0;
}

SCRIPT_API(PawnREST_GetUploadHttpStatus, int(int uploadId))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetUploadHttpStatus(uploadId);
}

// Outbound Request (Requests-style) Natives
SCRIPT_API(PawnREST_RequestsClient, int(const std::string& endpoint, const std::string& defaultHeaders, int verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->CreateRequestClient(endpoint, defaultHeaders, verifyTls != 0);
}

SCRIPT_API(PawnREST_RemoveRequestsClient, bool(int clientId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveRequestClient(clientId);
}

SCRIPT_API(PawnREST_SetRequestsClientHeader, bool(int clientId, const std::string& name, const std::string& value))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->SetRequestClientHeader(clientId, name, value);
}

SCRIPT_API(PawnREST_RemoveRequestsClientHeader, bool(int clientId, const std::string& name))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveRequestClientHeader(clientId, name);
}

SCRIPT_API(PawnREST_Request,
    int(int clientId, const std::string& path, int method, const std::string& callback, const std::string& body, const std::string& customHeaders))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->QueueOutboundRequest(clientId, path, method, callback, body, customHeaders, false);
}

SCRIPT_API(PawnREST_RequestJSON,
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

SCRIPT_API(PawnREST_CancelRequest, bool(int requestId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->CancelOutboundRequest(requestId);
}

SCRIPT_API(PawnREST_GetRequestStatus, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetOutboundRequestStatus(requestId);
}

SCRIPT_API(PawnREST_GetRequestHttpStatus, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return 0;
    return c->GetOutboundRequestHttpStatus(requestId);
}

SCRIPT_API(PawnREST_GetRequestErrorCode, int(int requestId))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->GetOutboundRequestErrorCode(requestId);
}

SCRIPT_API(PawnREST_GetRequestErrorType, int(int requestId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) {
        output.clear();
        return 0;
    }
    return c->GetOutboundRequestErrorType(requestId, output, outputSize) ? 1 : 0;
}

SCRIPT_API(PawnREST_GetRequestResponse, int(int requestId, std::string& output, int outputSize))
{
    auto c = GetComponent();
    if (!c) {
        output.clear();
        return 0;
    }
    return c->GetOutboundRequestResponse(requestId, output, outputSize) ? 1 : 0;
}

// WebSocket Client Natives
SCRIPT_API(PawnREST_WebSocketClient, int(const std::string& address, const std::string& callback, const std::string& headers, int verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->ConnectWebSocketClient(address, callback, false, headers, verifyTls != 0);
}

SCRIPT_API(PawnREST_JsonWebSocketClient, int(const std::string& address, const std::string& callback, const std::string& headers, int verifyTls))
{
    auto c = GetComponent();
    if (!c) return -1;
    return c->ConnectWebSocketClient(address, callback, true, headers, verifyTls != 0);
}

SCRIPT_API(PawnREST_WebSocketSend, bool(int socketId, const std::string& data))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->WebSocketSendText(socketId, data);
}

SCRIPT_API(PawnREST_JsonWebSocketSend, bool(int socketId, int nodeId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->WebSocketSendJson(socketId, nodeId);
}

SCRIPT_API(PawnREST_WebSocketClose, bool(int socketId, int status, const std::string& reason))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->CloseWebSocketClient(socketId, status, reason);
}

SCRIPT_API(PawnREST_RemoveWebSocketClient, bool(int socketId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->RemoveWebSocketClient(socketId);
}

SCRIPT_API(PawnREST_IsWebSocketOpen, bool(int socketId))
{
    auto c = GetComponent();
    if (!c) return false;
    return c->IsWebSocketOpen(socketId);
}

// CRC32 Utilities
SCRIPT_API(PawnREST_VerifyCRC32, int(const std::string& filepath, const std::string& expectedCrc))
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

SCRIPT_API(PawnREST_GetFileCRC32, int(const std::string& filepath, std::string& output, int outputSize))
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

    if (outputSize > 0) {
        size_t copyLen = std::min(static_cast<size_t>(outputSize - 1), hex.length());
        output.assign(hex.c_str(), copyLen);
    } else {
        output = "";
    }
    return 1;
}

SCRIPT_API(PawnREST_CompareFiles, int(const std::string& path1, const std::string& path2))
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
