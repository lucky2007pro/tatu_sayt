#define NOMINMAX
#include "ProfileController.h"
#include "ApiHelper.h"
#include <drogon/HttpViewData.h>

// ── Profil sahifasi ────────────────────────────────────────────────────────────
void ProfileController::show(const HttpRequestPtr& req,
                              std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isLoggedIn(req)) { cb(redirect("/kirish")); return; }

    std::string token     = sessionStr(req, "reader_token");
    std::string name      = sessionStr(req, "reader_name");
    bool        approved  = sessionBool(req, "reader_approved");
    std::string csrf      = ensureCsrf(req);
    std::string flashMsg  = sessionStr(req, "flash_msg");
    std::string flashType = sessionStr(req, "flash_type");
    if (!flashMsg.empty()) {
        req->session()->erase("flash_msg");
        req->session()->erase("flash_type");
    }

    // 1) O'quvchi ma'lumotlari
    apiGet("/api/readers/me/",
        [token, name, approved, csrf, flashMsg, flashType,
         cb = std::move(cb)](bool ok, const Json::Value& me) mutable
    {
        Row reader;
        if (ok && me.isObject())
            reader = jsonObjectToRow(me, {"id","fullname","phone","card_id","is_active"});
        else
            reader["fullname"] = name;

        // 2) Foydalanuvchi kutubxona kartlari
        apiGet("/api/readers/library-cards/",
            [approved, csrf, flashMsg, flashType, reader, token,
             cb = std::move(cb)](bool, const Json::Value& cardsJson) mutable
        {
            RowList cards = jsonArrayToRows(cardsJson,
                {"id","library","library_name","is_approved","card_image"});

            // 3) Barcha kutubxonalar (karta yuklash uchun dropdown)
            apiGet("/api/libraries/",
                [approved, csrf, flashMsg, flashType, reader, cards,
                 cb = std::move(cb)](bool, const Json::Value& libJson) mutable
            {
                RowList libraries = jsonArrayToRows(libJson, {"id","name"});

                HttpViewData data;
                data.insert("reader",          reader);
                data.insert("cards",           cards);
                data.insert("libraries",       libraries);
                data.insert("reader_approved", approved);
                data.insert("is_logged_in",    true);
                data.insert("flash_msg",       flashMsg);
                data.insert("flash_type",      flashType);
                data.insert("csrf_token",      csrf);

                cb(HttpResponse::newHttpViewResponse("Profile", data));
            });
        }, token);
    }, token);
}

// ── Profil tahrirlash ──────────────────────────────────────────────────────────
void ProfileController::editProfile(const HttpRequestPtr& req,
                                    std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isLoggedIn(req)) { cb(redirect("/kirish")); return; }

    std::string token    = sessionStr(req, "reader_token");
    std::string fullname = req->getParameter("fullname");
    std::string phone    = req->getParameter("phone");
    std::string password = req->getParameter("password");

    Json::Value body;
    if (!fullname.empty()) body["fullname"] = fullname;
    if (!phone.empty())    body["phone"]    = phone;
    if (!password.empty()) body["password"] = password;

    apiPatch("/api/readers/update-me/", body,
        [req, cb = std::move(cb), fullname](int status, const Json::Value&) mutable {
            bool ok = status < 400;
            if (ok && !fullname.empty())
                req->session()->insert("reader_name", fullname);
            req->session()->insert("flash_msg",
                ok ? std::string("Profil muvaffaqiyatli yangilandi.")
                   : std::string("Profilni yangilashda xato yuz berdi."));
            req->session()->insert("flash_type",
                ok ? std::string("success") : std::string("error"));
            cb(redirect("/profil"));
        }, token, false);
}

// ── Kutubxona kartasi yuklash ──────────────────────────────────────────────────
void ProfileController::uploadCard(const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isLoggedIn(req)) { cb(redirect("/kirish")); return; }

    std::string token       = sessionStr(req, "reader_token");
    std::string libraryId   = req->getParameter("library_id");
    std::string imageBase64 = req->getParameter("card_image_base64");

    if (libraryId.empty() || imageBase64.empty()) {
        req->session()->insert("flash_msg",  std::string("Kutubxona va karta rasmi majburiy."));
        req->session()->insert("flash_type", std::string("error"));
        cb(redirect("/profil"));
        return;
    }

    Json::Value body;
    body["library"]           = std::stoi(libraryId);
    body["card_image_base64"] = imageBase64;

    apiPost("/api/readers/library-cards/", body,
        [req, cb = std::move(cb)](int status, const Json::Value& data) mutable {
            bool ok = status < 400;
            std::string msg = ok
                ? "Karta yuklandi. Admin tasdig'ini kuting."
                : "Karta yuklashda xato yuz berdi.";
            if (!ok && data.isObject() && data.isMember("detail") && data["detail"].isString())
                msg = data["detail"].asString();
            req->session()->insert("flash_msg",  msg);
            req->session()->insert("flash_type", ok ? std::string("success") : std::string("error"));
            cb(redirect("/profil"));
        }, token);
}
