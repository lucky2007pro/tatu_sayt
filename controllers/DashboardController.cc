#define NOMINMAX
#include "DashboardController.h"
#include "ApiHelper.h"
#include <drogon/HttpViewData.h>

void DashboardController::index(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isLoggedIn(req)) { cb(redirect("/kirish")); return; }

    std::string token = sessionStr(req, "reader_token");
    std::string name  = sessionStr(req, "reader_name");
    std::string cardId= sessionStr(req, "reader_card_id");

    // Real vaqtda /me/ dan yangilash — Reader.is_active = active/blok
    apiGet("/api/readers/me/",
        [req, token, name, cardId, cb = std::move(cb)](bool ok, const Json::Value& me) mutable
    {
        if (ok && me.isObject()) {
            bool active = me["is_active"].isBool() ? me["is_active"].asBool() : true;
            req->session()->insert("reader_approved", active);
            if (me.isMember("fullname") && me["fullname"].isString())
                req->session()->insert("reader_name", me["fullname"].asString());
            if (me.isMember("card_id") && me["card_id"].isString())
                req->session()->insert("reader_card_id", me["card_id"].asString());
        }

        bool approved = sessionBool(req, "reader_approved");
        std::string updatedName = sessionStr(req, "reader_name", name);
        std::string csrf = ensureCsrf(req);

        std::string flashMsg  = sessionStr(req, "flash_msg");
        std::string flashType = sessionStr(req, "flash_type");
        if (!flashMsg.empty()) {
            req->session()->erase("flash_msg");
            req->session()->erase("flash_type");
        }

        // 1) Bronlar
        apiGet("/api/reservations/?mine=1",
            [updatedName, cardId, approved, csrf, flashMsg, flashType, token,
             cb = std::move(cb)](bool, const Json::Value& resJson) mutable
        {
            RowList reservations = jsonArrayToRows(resJson,
                {"id","book","book_title","reserved_at","note"});

            // 2) O'qish tarixi (Issues)
            apiGet("/api/issues/?mine=1",
                [updatedName, cardId, approved, csrf, flashMsg, flashType, token,
                 cb = std::move(cb), reservations](bool, const Json::Value& isJson) mutable
            {
                RowList issues = jsonArrayToRows(isJson,
                    {"id","book","book_title","issue_date","return_date"});

                // 3) Navbat ro'yxati
                apiGet("/api/waitlist/?mine=1",
                    [updatedName, cardId, approved, csrf, flashMsg, flashType, token,
                     cb = std::move(cb), reservations, issues](bool, const Json::Value& wJson) mutable
                {
                    RowList waitlist = jsonArrayToRows(wJson,
                        {"id","book","book_title","position","created_at"});

                    HttpViewData data;
                    data.insert("reader_name",    updatedName);
                    data.insert("reader_card_id", cardId);
                    data.insert("reader_approved",approved);
                    data.insert("reservations",   reservations);
                    data.insert("issues",         issues);
                    data.insert("waitlist",       waitlist);
                    data.insert("is_logged_in",   true);
                    data.insert("flash_msg",      flashMsg);
                    data.insert("flash_type",     flashType);
                    data.insert("csrf_token",     csrf);

                    cb(HttpResponse::newHttpViewResponse("Dashboard", data));
                }, token);
            }, token);
        }, token);
    }, token);
}

void DashboardController::cancel(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 int id)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isLoggedIn(req)) { cb(redirect("/kirish")); return; }

    std::string token = sessionStr(req, "reader_token");
    std::string path  = "/api/reservations/" + std::to_string(id) + "/";

    // Django: DELETE /api/reservations/<id>/ — token egasi yoki admin
    apiDelete(path, [req, cb = std::move(cb)](int status) mutable {
        if (status >= 200 && status < 300) {
            req->session()->insert("flash_msg",  std::string("Bron bekor qilindi."));
            req->session()->insert("flash_type", std::string("success"));
        } else if (status == 403) {
            req->session()->insert("flash_msg",  std::string("Bu bronni bekor qilishga ruxsatingiz yo'q."));
            req->session()->insert("flash_type", std::string("error"));
        } else {
            req->session()->insert("flash_msg",  std::string("Bronni bekor qilib bo'lmadi."));
            req->session()->insert("flash_type", std::string("error"));
        }
        cb(redirect("/kabinet"));
    }, token);
}
