#define NOMINMAX
#include "AIController.h"
#include "ApiHelper.h"
#include <drogon/HttpViewData.h>

// ── Maslahatchi sahifasi (chat UI) ───────────────────────────────────────────
void AIController::advisor(const HttpRequestPtr& req,
                           std::function<void(const HttpResponsePtr&)>&& cb)
{
    bool loggedIn    = isLoggedIn(req);
    std::string name = sessionStr(req, "reader_name", "Mehmon");
    bool approved    = sessionBool(req, "reader_approved");
    std::string csrf = ensureCsrf(req);

    HttpViewData data;
    data.insert("is_logged_in",   loggedIn);
    data.insert("reader_name",    name);
    data.insert("reader_approved",approved);
    data.insert("csrf_token",     csrf);
    cb(HttpResponse::newHttpViewResponse("AIAdvisor", data));
}

// ── Chat proxy: brauzer → Drogon → Django Gemini ─────────────────────────────
void AIController::chatProxy(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!checkCsrf(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        resp->setBody("{\"error\":\"CSRF xato\"}");
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        cb(resp);
        return;
    }

    // JSON body kelishi kutiladi: {"message": "...", "history": [...]}
    auto json = req->getJsonObject();
    Json::Value body;
    if (json) {
        body = *json;
    } else {
        // Yoki form parametri
        body["message"] = req->getParameter("message");
    }

    std::string token = sessionStr(req, "reader_token");

    apiPost("/api/ai/chat/", body,
        [cb = std::move(cb)](int statusCode, const Json::Value& data) mutable {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(static_cast<drogon::HttpStatusCode>(statusCode));
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            Json::FastWriter w;
            resp->setBody(w.write(data));
            cb(resp);
        }, token);
}

// ── Status: AI sozlanganmi? ──────────────────────────────────────────────────
void AIController::statusJson(const HttpRequestPtr& req,
                              std::function<void(const HttpResponsePtr&)>&& cb)
{
    apiGet("/api/ai/status/",
        [cb = std::move(cb)](bool, const Json::Value& data) mutable {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            Json::FastWriter w;
            resp->setBody(w.write(data));
            cb(resp);
        });
}
