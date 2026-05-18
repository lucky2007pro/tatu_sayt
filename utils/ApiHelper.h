#pragma once
#include <drogon/HttpClient.h>
#include <drogon/drogon.h>
#include <json/json.h>
#include <functional>
#include <string>
#include <map>
#include <vector>
#include <tuple>

// ── Tiplar ────────────────────────────────────────────────────────────────────
using Row     = std::map<std::string, std::string>;
using RowList = std::vector<Row>;

// ── API manzilini config'dan olish ───────────────────────────────────────────
inline std::string apiUrl() {
    const auto& cfg = drogon::app().getCustomConfig();
    if (cfg.isObject() && cfg.isMember("api_url") && !cfg["api_url"].isNull())
        return cfg["api_url"].asString();
    return "http://5.189.136.95:81";
}

inline std::string adminToken() {
    const auto& cfg = drogon::app().getCustomConfig();
    if (cfg.isObject() && cfg.isMember("admin_token") && !cfg["admin_token"].isNull())
        return cfg["admin_token"].asString();
    return "TATU2026";
}

// ── JSON → Row konvertatsiya ──────────────────────────────────────────────────
inline std::string jstr(const Json::Value& v, const char* key,
                        const std::string& def = "") {
    if (!v.isObject()) return def;          // massiv yoki null bo'lsa xato
    if (!v.isMember(key) || v[key].isNull()) return def;
    const auto& val = v[key];
    if (val.isString()) return val.asString();
    if (val.isInt())    return std::to_string(val.asInt());
    if (val.isInt64())  return std::to_string(val.asInt64());
    if (val.isDouble()) return std::to_string(val.asDouble());
    if (val.isBool())   return val.asBool() ? "True" : "False";
    return val.toStyledString();
}

// API javobidan "results" yoki to'g'ridan massivni olish
inline const Json::Value& extractArray(const Json::Value& json) {
    static const Json::Value empty(Json::arrayValue);
    if (json.isArray()) return json;
    if (json.isObject() && json.isMember("results") && json["results"].isArray())
        return json["results"];
    return empty;
}

inline RowList jsonArrayToRows(const Json::Value& arr,
    const std::vector<std::string>& fields)
{
    RowList rows;
    const Json::Value& realArr = arr.isArray() ? arr : extractArray(arr);
    if (!realArr.isArray()) return rows;
    for (const auto& item : realArr) {
        if (!item.isObject()) continue;
        Row row;
        for (const auto& f : fields)
            row[f] = jstr(item, f.c_str());
        rows.push_back(row);
    }
    return rows;
}

// Bitta JSON object'ni Row ga aylantiradi, fieldlar yo'q bo'lsa empty
inline Row jsonObjectToRow(const Json::Value& obj,
    const std::vector<std::string>& fields)
{
    Row row;
    for (const auto& f : fields)
        row[f] = jstr(obj, f.c_str());
    return row;
}

// Map'dan xavfsiz qiymat olish — kalit yo'q bo'lsa default qaytaradi
inline std::string getOr(const Row& r, const std::string& k,
                         const std::string& def = "") {
    auto it = r.find(k);
    return (it != r.end()) ? it->second : def;
}

// ── GET so'rovi ───────────────────────────────────────────────────────────────
inline void apiGet(const std::string& path,
                   std::function<void(bool ok, const Json::Value&)> cb,
                   const std::string& readerToken = "",
                   bool adminAuth = false)
{
    auto client = drogon::HttpClient::newHttpClient(apiUrl());
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath(path);
    if (!readerToken.empty())
        req->addHeader("X-Reader-Token", readerToken);
    if (adminAuth)
        req->addHeader("X-Admin-Token", adminToken());

    client->sendRequest(req,
        [cb, client](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp && resp->getJsonObject()) {
                cb(resp->statusCode() < 400, *resp->getJsonObject());
            } else {
                cb(false, Json::Value());
            }
        });
}

// ── POST so'rovi (JSON body) ──────────────────────────────────────────────────
inline void apiPost(const std::string& path,
                    const Json::Value& body,
                    std::function<void(int statusCode, const Json::Value&)> cb,
                    const std::string& readerToken = "",
                    bool adminAuth = false)
{
    auto client = drogon::HttpClient::newHttpClient(apiUrl());
    auto req = drogon::HttpRequest::newHttpJsonRequest(body);
    req->setMethod(drogon::Post);
    req->setPath(path);
    if (!readerToken.empty())
        req->addHeader("X-Reader-Token", readerToken);
    if (adminAuth)
        req->addHeader("X-Admin-Token", adminToken());

    client->sendRequest(req,
        [cb, client](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                Json::Value data;
                if (resp->getJsonObject()) data = *resp->getJsonObject();
                cb(resp->statusCode(), data);
            } else {
                Json::Value err;
                err["error"] = "API bilan bog'lanib bo'lmadi";
                cb(503, err);
            }
        });
}

// ── PATCH so'rovi ─────────────────────────────────────────────────────────────
inline void apiPatch(const std::string& path,
                     const Json::Value& body,
                     std::function<void(int statusCode, const Json::Value&)> cb,
                     const std::string& readerToken = "",
                     bool adminAuth = false)
{
    auto client = drogon::HttpClient::newHttpClient(apiUrl());
    auto req = drogon::HttpRequest::newHttpJsonRequest(body);
    req->setMethod(drogon::HttpMethod::Patch);
    req->setPath(path);
    if (!readerToken.empty())
        req->addHeader("X-Reader-Token", readerToken);
    if (adminAuth)
        req->addHeader("X-Admin-Token", adminToken());

    client->sendRequest(req,
        [cb, client](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                Json::Value data;
                if (resp->getJsonObject()) data = *resp->getJsonObject();
                cb(resp->statusCode(), data);
            } else {
                Json::Value err;
                err["error"] = "API bilan bog'lanib bo'lmadi";
                cb(503, err);
            }
        });
}

// ── DELETE so'rovi ────────────────────────────────────────────────────────────
inline void apiDelete(const std::string& path,
                      std::function<void(int statusCode)> cb,
                      const std::string& readerToken = "",
                      bool adminAuth = false)
{
    auto client = drogon::HttpClient::newHttpClient(apiUrl());
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Delete);
    req->setPath(path);
    if (!readerToken.empty())
        req->addHeader("X-Reader-Token", readerToken);
    if (adminAuth)
        req->addHeader("X-Admin-Token", adminToken());

    client->sendRequest(req,
        [cb, client](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) cb(resp->statusCode());
            else cb(503);
        });
}

// ── Fayl yuklash uchun tip: (filename, content_type, binary_data) ────────────
using FileData = std::tuple<std::string, std::string, std::string>;

inline std::string buildMultipartBody(
    const std::string& boundary,
    const std::map<std::string, std::string>& fields,
    const std::map<std::string, FileData>& files)
{
    std::string body;
    for (const auto& kv : fields) {
        if (kv.second.empty()) continue;
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"" + kv.first + "\"\r\n\r\n";
        body += kv.second + "\r\n";
    }
    for (const auto& kv : files) {
        const auto& fname = std::get<0>(kv.second);
        const auto& ct    = std::get<1>(kv.second);
        const auto& data  = std::get<2>(kv.second);
        if (data.empty()) continue;
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"" + kv.first + "\"; filename=\"" + fname + "\"\r\n";
        body += "Content-Type: " + ct + "\r\n\r\n";
        body += data + "\r\n";
    }
    body += "--" + boundary + "--\r\n";
    return body;
}

inline void apiPostMultipart(
    const std::string& path,
    const std::map<std::string, std::string>& fields,
    const std::map<std::string, FileData>& files,
    std::function<void(int, const Json::Value&)> cb,
    bool adminAuth = false)
{
    std::string bnd = "TATUBoundary" + drogon::utils::getUuid().substr(0, 16);
    std::string body = buildMultipartBody(bnd, fields, files);
    auto client = drogon::HttpClient::newHttpClient(apiUrl());
    auto r2 = drogon::HttpRequest::newHttpRequest();
    r2->setMethod(drogon::Post);
    r2->setPath(path);
    r2->setBody(body);
    r2->addHeader("Content-Type", "multipart/form-data; boundary=" + bnd);
    if (adminAuth) r2->addHeader("X-Admin-Token", adminToken());
    client->sendRequest(r2,
        [cb, client](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                Json::Value d;
                if (resp->getJsonObject()) d = *resp->getJsonObject();
                cb(resp->statusCode(), d);
            } else { Json::Value e; e["error"] = "API xato"; cb(503, e); }
        });
}

inline void apiPatchMultipart(
    const std::string& path,
    const std::map<std::string, std::string>& fields,
    const std::map<std::string, FileData>& files,
    std::function<void(int, const Json::Value&)> cb,
    bool adminAuth = false)
{
    std::string bnd = "TATUBoundary" + drogon::utils::getUuid().substr(0, 16);
    std::string body = buildMultipartBody(bnd, fields, files);
    auto client = drogon::HttpClient::newHttpClient(apiUrl());
    auto r2 = drogon::HttpRequest::newHttpRequest();
    r2->setMethod(drogon::HttpMethod::Patch);
    r2->setPath(path);
    r2->setBody(body);
    r2->addHeader("Content-Type", "multipart/form-data; boundary=" + bnd);
    if (adminAuth) r2->addHeader("X-Admin-Token", adminToken());
    client->sendRequest(r2,
        [cb, client](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                Json::Value d;
                if (resp->getJsonObject()) d = *resp->getJsonObject();
                cb(resp->statusCode(), d);
            } else { Json::Value e; e["error"] = "API xato"; cb(503, e); }
        });
}

// ── Raw multipart proxy: butun body'ni Django ga yo'naltiradi ───────────────
inline void apiPostMultipartRaw(
    const std::string& path,
    const std::string& rawBody,
    const std::string& contentType,
    std::function<void(int, const Json::Value&)> cb,
    bool adminAuth = false)
{
    auto client = drogon::HttpClient::newHttpClient(apiUrl());
    auto r2 = drogon::HttpRequest::newHttpRequest();
    r2->setMethod(drogon::Post);
    r2->setPath(path);
    r2->setBody(rawBody);
    r2->addHeader("Content-Type", contentType);
    if (adminAuth) r2->addHeader("X-Admin-Token", adminToken());
    client->sendRequest(r2,
        [cb, client](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                Json::Value d;
                if (resp->getJsonObject()) d = *resp->getJsonObject();
                cb(resp->statusCode(), d);
            } else { Json::Value e; e["error"] = "API xato"; cb(503, e); }
        });
}

inline void apiPatchMultipartRaw(
    const std::string& path,
    const std::string& rawBody,
    const std::string& contentType,
    std::function<void(int, const Json::Value&)> cb,
    bool adminAuth = false)
{
    auto client = drogon::HttpClient::newHttpClient(apiUrl());
    auto r2 = drogon::HttpRequest::newHttpRequest();
    r2->setMethod(drogon::HttpMethod::Patch);
    r2->setPath(path);
    r2->setBody(rawBody);
    r2->addHeader("Content-Type", contentType);
    if (adminAuth) r2->addHeader("X-Admin-Token", adminToken());
    client->sendRequest(r2,
        [cb, client](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                Json::Value d;
                if (resp->getJsonObject()) d = *resp->getJsonObject();
                cb(resp->statusCode(), d);
            } else { Json::Value e; e["error"] = "API xato"; cb(503, e); }
        });
}

// ── Session yordamchilari ─────────────────────────────────────────────────────
inline bool isLoggedIn(const drogon::HttpRequestPtr& req) {
    return req->session()->find("reader_token");
}

inline std::string sessionStr(const drogon::HttpRequestPtr& req,
                               const char* key,
                               const std::string& def = "") {
    if (req->session()->find(key))
        return req->session()->get<std::string>(key);
    return def;
}

inline bool sessionBool(const drogon::HttpRequestPtr& req, const char* key) {
    if (req->session()->find(key))
        return req->session()->get<bool>(key);
    return false;
}

// ── CSRF ──────────────────────────────────────────────────────────────────────
inline std::string ensureCsrf(const drogon::HttpRequestPtr& req) {
    auto s = req->session();
    if (!s->find("csrf_token"))
        s->insert("csrf_token", drogon::utils::getUuid());
    return s->get<std::string>("csrf_token");
}

// multipart/form-data body'dan matn maydonini ajratib olish
inline std::string extractMultipartField(const std::string& body,
                                          const std::string& fieldName) {
    std::string marker = "name=\"" + fieldName + "\"";
    auto pos = body.find(marker);
    if (pos == std::string::npos) return "";
    // Sarlavhalar tugashi: \r\n\r\n
    auto start = body.find("\r\n\r\n", pos);
    if (start == std::string::npos) return "";
    start += 4;
    // Qiymat tugashi: keyingi boundary oldidagi \r\n--
    auto end = body.find("\r\n--", start);
    if (end == std::string::npos) return "";
    return body.substr(start, end - start);
}

inline bool checkCsrf(const drogon::HttpRequestPtr& req) {
    auto s = req->session();
    if (!s->find("csrf_token")) return false;

    // 1-urinish: application/x-www-form-urlencoded
    std::string token = req->getParameter("csrf_token");

    // 2-urinish: HTTP sarlavhasidan (X-CSRF-Token)
    if (token.empty()) {
        token = req->getHeader("X-CSRF-Token");
        if (token.empty()) token = req->getHeader("x-csrf-token");
    }

    // 3-urinish: JSON body'dan
    if (token.empty()) {
        auto json = req->getJsonObject();
        if (json && (*json)["csrf_token"].isString()) {
            token = (*json)["csrf_token"].asString();
        }
    }

    // 4-urinish: multipart/form-data body'dan
    if (token.empty()) {
        const auto& bv = req->getBody();
        if (bv.size() > 0)
            token = extractMultipartField(std::string(bv.data(), bv.size()),
                                          "csrf_token");
    }

    if (token.empty()) return false;
    return token == s->get<std::string>("csrf_token");
}

// ── Xato javobi ───────────────────────────────────────────────────────────────
inline drogon::HttpResponsePtr errResp(int code, const std::string& msg) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(code));
    resp->setBody("<h2>" + msg + "</h2>");
    return resp;
}

// ── Redirect ──────────────────────────────────────────────────────────────────
inline drogon::HttpResponsePtr redirect(const std::string& url) {
    return drogon::HttpResponse::newRedirectionResponse(url);
}
