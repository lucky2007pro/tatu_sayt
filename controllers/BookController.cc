#define NOMINMAX
#include "BookController.h"
#include "ApiHelper.h"
#include <drogon/HttpViewData.h>

// ── Kitob batafsil ────────────────────────────────────────────────────────────
void BookController::detail(const HttpRequestPtr& req,
                            std::function<void(const HttpResponsePtr&)>&& cb,
                            int id)
{
    bool loggedIn = isLoggedIn(req);
    std::string token    = sessionStr(req, "reader_token");
    std::string name     = sessionStr(req, "reader_name", "Mehmon");
    bool approved        = sessionBool(req, "reader_approved");
    std::string csrf     = ensureCsrf(req);
    std::string flashMsg = sessionStr(req, "flash_msg");
    std::string flashType= sessionStr(req, "flash_type");
    if (!flashMsg.empty()) {
        req->session()->erase("flash_msg");
        req->session()->erase("flash_type");
    }

    std::string path = "/api/books/" + std::to_string(id) + "/";
    apiGet(path, [=, cb = std::move(cb)](bool ok, const Json::Value& bookJson) mutable {
        if (!ok || !bookJson.isObject()) {
            cb(redirect("/?error=kitob_topilmadi"));
            return;
        }
        Row book = jsonObjectToRow(bookJson, {
            "id", "title", "author_name", "library", "library_name", "section_name",
            "is_available", "availability_status", "cover_image", "description", "average_rating",
            "ratings_count", "view_count", "ebook_file",
            "published_date", "isbn", "shelf", "row",
            "library_latitude", "library_longitude"
        });

        // Kutubxona to'liq ma'lumotlari (manzil + telefon)
        if (bookJson["library"].isInt()) {
            book["library_id"] = std::to_string(bookJson["library"].asInt());
        }

        // Izohlar (ratings + review)
        std::string ratingsPath = "/api/books/" + std::to_string(id) + "/ratings/";
        apiGet(ratingsPath,
            [id, book, loggedIn, token, name, approved, csrf, flashMsg, flashType,
             cb = std::move(cb)](bool, const Json::Value& rj) mutable
        {
            RowList ratings = jsonArrayToRows(rj,
                {"id","reader_name","rating","review","updated_at"});

            // O'xshash kitoblar — xuddi shu bo'lim
            std::string sectionId = book.count("section_name") ? "" : "";
            std::string similarPath = "/api/books/?page_size=6&exclude=" + std::to_string(id);
            if (book.count("section_name") && !book.at("section_name").empty())
                similarPath += "&section_name=" + drogon::utils::urlEncode(book.at("section_name"));
            apiGet(similarPath,
                [book, ratings, loggedIn, name, approved, csrf, flashMsg, flashType,
                 id, cb = std::move(cb)](bool, const Json::Value& simJ) mutable
            {
                RowList similar;
                const Json::Value& arr = simJ.isObject() ? simJ["results"] : simJ;
                if (arr.isArray()) {
                    for (const auto& s : arr) {
                        if (s.isObject()) {
                            std::string sid = s.isMember("id") ? std::to_string(s["id"].asInt()) : "";
                            if (sid == std::to_string(id)) continue;
                            Row r;
                            r["id"]     = sid;
                            r["title"]  = s.isMember("title")       ? s["title"].asString()       : "";
                            r["author_name"] = s.isMember("author_name") ? s["author_name"].asString() : "";
                            r["cover_image"] = s.isMember("cover_image") ? s["cover_image"].asString() : "";
                            r["average_rating"] = s.isMember("average_rating") ? s["average_rating"].asString() : "";
                            similar.push_back(r);
                            if (similar.size() >= 5) break;
                        }
                    }
                }

                HttpViewData data;
                data.insert("book",           book);
                data.insert("ratings",        ratings);
                data.insert("similar",        similar);
                data.insert("is_logged_in",   loggedIn);
                data.insert("reader_name",    name);
                data.insert("reader_approved",approved);
                data.insert("csrf_token",     csrf);
                data.insert("flash_msg",      flashMsg);
                data.insert("flash_type",     flashType);

                cb(HttpResponse::newHttpViewResponse("BookDetail", data));
            }, token);
        }, token);
    }, token);
}

// ── Band qilish ───────────────────────────────────────────────────────────────
void BookController::reserve(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             int id)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isLoggedIn(req)) { cb(redirect("/kirish")); return; }

    std::string token = sessionStr(req, "reader_token");
    std::string backUrl = "/kitob/" + std::to_string(id);

    if (!sessionBool(req, "reader_approved")) {
        req->session()->insert("flash_msg",
            std::string("Hisobingiz admin tomonidan tasdiqlanmagan."));
        req->session()->insert("flash_type", std::string("error"));
        cb(redirect(backUrl));
        return;
    }

    Json::Value body;
    body["book"] = id;

    apiPost("/api/reservations/", body,
        [req, cb = std::move(cb), backUrl](int status, const Json::Value& data) mutable {
            if (status == 200 || status == 201) {
                req->session()->insert("flash_msg",
                    std::string("Kitob muvaffaqiyatli band qilindi!"));
                req->session()->insert("flash_type", std::string("success"));
            } else {
                std::string err = "Band qilishda xato.";
                if (data.isObject()) {
                    if (data.isMember("detail") && data["detail"].isString())
                        err = data["detail"].asString();
                    else if (data.isMember("error") && data["error"].isString())
                        err = data["error"].asString();
                    else {
                        for (const auto& key : data.getMemberNames()) {
                            const auto& v = data[key];
                            if (v.isString() && !v.asString().empty()) { err = v.asString(); break; }
                            if (v.isArray() && !v.empty() && v[0].isString()) { err = v[0].asString(); break; }
                        }
                    }
                }
                req->session()->insert("flash_msg",  err);
                req->session()->insert("flash_type", std::string("error"));
            }
            cb(redirect(backUrl));
        },
        token);
}

// ── Baholash ──────────────────────────────────────────────────────────────────
void BookController::rateBook(const HttpRequestPtr& req,
                              std::function<void(const HttpResponsePtr&)>&& cb,
                              int id)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isLoggedIn(req)) { cb(redirect("/kirish")); return; }

    std::string token  = sessionStr(req, "reader_token");
    std::string score  = req->getParameter("score");
    std::string review = req->getParameter("review");
    std::string backUrl = "/kitob/" + std::to_string(id);

    int rating = 5;
    if (!score.empty()) {
        try { rating = std::stoi(score); } catch (...) { rating = 5; }
        if (rating < 1) rating = 1;
        if (rating > 5) rating = 5;
    }
    Json::Value body;
    body["rating"] = rating;
    body["review"] = review;

    // Django: POST /api/books/<id>/rate/  body: {rating: 1..5, review: ""}
    apiPost("/api/books/" + std::to_string(id) + "/rate/", body,
        [req, cb = std::move(cb), backUrl](int status, const Json::Value& data) mutable {
            if (status == 200 || status == 201) {
                req->session()->insert("flash_msg",  std::string("Baholash saqlandi!"));
                req->session()->insert("flash_type", std::string("success"));
            } else {
                std::string err = "Baholashda xato.";
                if (data.isObject()) {
                    if (data.isMember("detail") && data["detail"].isString())
                        err = data["detail"].asString();
                    else if (data.isMember("error") && data["error"].isString())
                        err = data["error"].asString();
                    else {
                        for (const auto& key : data.getMemberNames()) {
                            const auto& v = data[key];
                            if (v.isString() && !v.asString().empty()) { err = v.asString(); break; }
                            if (v.isArray() && !v.empty() && v[0].isString()) { err = v[0].asString(); break; }
                        }
                    }
                }
                req->session()->insert("flash_msg",  err);
                req->session()->insert("flash_type", std::string("error"));
            }
            cb(redirect(backUrl));
        },
        token);
}

