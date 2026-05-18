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
                {"id","book","book_title","reserved_at","note","expires_at","days_remaining","is_expired"});

            // 2) O'qish tarixi (Issues)
            apiGet("/api/issues/?mine=1",
                [updatedName, cardId, approved, csrf, flashMsg, flashType, token,
                 cb = std::move(cb), reservations](bool, const Json::Value& isJson) mutable
            {
                RowList issues = jsonArrayToRows(isJson,
                    {"id","book","book_title","issue_date","return_date","is_returned"});

                // 3) Tavsiyalar (Sizga yoqishi mumkin)
                apiGet("/api/books/recommended-for-me/?limit=8",
                    [updatedName, cardId, approved, csrf, flashMsg, flashType, token,
                     cb = std::move(cb), reservations, issues](bool, const Json::Value& recJson) mutable
                {
                    RowList recommended = jsonArrayToRows(recJson,
                        {"id","title","author_name","cover_image","library_name",
                         "average_rating","availability_status"});

                    // 4) Statistika (gamification)
                    apiGet("/api/readers/my-stats/",
                        [updatedName, cardId, approved, csrf, flashMsg, flashType,
                         cb = std::move(cb), reservations, issues, recommended]
                        (bool, const Json::Value& stats) mutable
                    {
                        // Statistik maydonlarni stringlashtiramiz
                        std::string sLevel, sLevelTier, sLevelNext;
                        std::string sTotalIssues, sReturnedIssues, sActiveIssues,
                                    sMonthIssues, sRecentIssues,
                                    sTotalRes, sActiveRes, sTotalFav, sTotalRatings;
                        Json::Value badges;
                        if (stats.isObject()) {
                            const auto& lvl = stats["level"];
                            if (lvl.isObject()) {
                                if (lvl["name"].isString())  sLevel     = lvl["name"].asString();
                                if (lvl["tier"].isInt())     sLevelTier = std::to_string(lvl["tier"].asInt());
                                if (lvl["next_at"].isInt())  sLevelNext = std::to_string(lvl["next_at"].asInt());
                            }
                            const auto& m = stats["metrics"];
                            if (m.isObject()) {
                                auto si = [&](const char* k) { return m[k].isInt() ? std::to_string(m[k].asInt()) : std::string("0"); };
                                sTotalIssues    = si("total_issues");
                                sReturnedIssues = si("returned_issues");
                                sActiveIssues   = si("active_issues");
                                sMonthIssues    = si("month_issues");
                                sRecentIssues   = si("recent_issues");
                                sTotalRes       = si("total_reservations");
                                sActiveRes      = si("active_reservations");
                                sTotalFav       = si("total_favourites");
                                sTotalRatings   = si("total_ratings");
                            }
                            if (stats["badges"].isArray()) badges = stats["badges"];
                        }
                        RowList badgeRows = jsonArrayToRows(badges, {"code","name","icon","description"});

                        HttpViewData data;
                        data.insert("reader_name",    updatedName);
                        data.insert("reader_card_id", cardId);
                        data.insert("reader_approved",approved);
                        data.insert("reservations",   reservations);
                        data.insert("issues",         issues);
                        data.insert("recommended",    recommended);
                        data.insert("badges",         badgeRows);
                        data.insert("level_name",     sLevel.empty() ? std::string("Yangi") : sLevel);
                        data.insert("level_tier",     sLevelTier.empty() ? std::string("1") : sLevelTier);
                        data.insert("level_next",     sLevelNext);
                        data.insert("total_issues",   sTotalIssues.empty() ? std::string("0") : sTotalIssues);
                        data.insert("returned_issues",sReturnedIssues);
                        data.insert("active_issues",  sActiveIssues);
                        data.insert("month_issues",   sMonthIssues);
                        data.insert("recent_issues",  sRecentIssues);
                        data.insert("total_res",      sTotalRes);
                        data.insert("active_res",     sActiveRes);
                        data.insert("total_fav",      sTotalFav);
                        data.insert("total_ratings",  sTotalRatings);
                        data.insert("is_logged_in",   true);
                        data.insert("flash_msg",      flashMsg);
                        data.insert("flash_type",     flashType);
                        data.insert("csrf_token",     csrf);

                        cb(HttpResponse::newHttpViewResponse("Dashboard", data));
                    }, token);
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
