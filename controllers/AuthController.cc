#define NOMINMAX
#include "AuthController.h"
#include "ApiHelper.h"
#include <drogon/HttpViewData.h>

// ── Login forma ───────────────────────────────────────────────────────────────
void AuthController::loginForm(const HttpRequestPtr& req,
                               std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (isLoggedIn(req)) { cb(redirect("/kabinet")); return; }
    std::string nextUrl = req->getParameter("next");
    if (nextUrl.empty() || nextUrl[0] != '/') nextUrl = "";
    HttpViewData data;
    data.insert("csrf_token", ensureCsrf(req));
    data.insert("error_msg",  std::string(""));
    data.insert("next_url",   nextUrl);
    cb(HttpResponse::newHttpViewResponse("Login", data));
}

// ── Login qayta ishlash ───────────────────────────────────────────────────────
void AuthController::handleLogin(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!checkCsrf(req)) {
        HttpViewData vd;
        vd.insert("csrf_token", ensureCsrf(req));
        vd.insert("error_msg",  std::string("Sessiya muddati tugagan. Sahifani yangilab qayta urinib ko'ring."));
        cb(HttpResponse::newHttpViewResponse("Login", vd));
        return;
    }

    std::string phone    = req->getParameter("phone");
    std::string password = req->getParameter("password");

    Json::Value body;
    body["phone"]    = phone;
    body["password"] = password;

    apiPost("/api/readers/login/", body,
        [req, cb = std::move(cb), phone](int status, const Json::Value& data) mutable {
            if (status == 200 && data.isObject() && data.isMember("token")) {
                auto s = req->session();
                s->insert("reader_token",   data["token"].asString());
                s->insert("reader_name",    jstr(data, "fullname", phone));
                s->insert("reader_card_id", jstr(data, "card_id",  phone));

                bool isActive = true;
                if (data.isMember("reader") && data["reader"].isObject() &&
                    data["reader"].isMember("is_active")) {
                    isActive = data["reader"]["is_active"].asBool();
                } else if (data.isMember("is_active")) {
                    isActive = data["is_active"].asBool();
                }
                s->insert("reader_approved", isActive);
                if (data.isMember("id") && data["id"].isInt())
                    s->insert("reader_id", std::to_string(data["id"].asInt()));
                std::string nextUrl = req->getParameter("next");
                if (nextUrl.empty() || nextUrl[0] != '/') nextUrl = "/kabinet";
                cb(redirect(nextUrl));
            } else {
                std::string err = "Noto'g'ri telefon raqam yoki parol.";
                if (data.isObject()) {
                    if (data.isMember("error"))  err = data["error"].asString();
                    if (data.isMember("detail")) err = data["detail"].asString();
                }
                HttpViewData vd;
                vd.insert("csrf_token", ensureCsrf(req));
                vd.insert("error_msg",  err);
                cb(HttpResponse::newHttpViewResponse("Login", vd));
            }
        });
}

// ── Register forma ────────────────────────────────────────────────────────────
void AuthController::registerForm(const HttpRequestPtr& req,
                                  std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (isLoggedIn(req)) { cb(redirect("/kabinet")); return; }
    HttpViewData data;
    data.insert("csrf_token", ensureCsrf(req));
    data.insert("error_msg",  std::string(""));
    cb(HttpResponse::newHttpViewResponse("Register", data));
}

// ── Register qayta ishlash ────────────────────────────────────────────────────
void AuthController::handleReg(const HttpRequestPtr& req,
                               std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!checkCsrf(req)) {
        HttpViewData vd;
        vd.insert("csrf_token", ensureCsrf(req));
        vd.insert("error_msg",  std::string("Sessiya muddati tugagan. Sahifani yangilab qayta urinib ko'ring."));
        cb(HttpResponse::newHttpViewResponse("Register", vd));
        return;
    }

    std::string fullname  = req->getParameter("fullname");
    std::string phone     = req->getParameter("phone");
    std::string password  = req->getParameter("password");
    std::string password2 = req->getParameter("password2");

    if (password != password2) {
        HttpViewData vd;
        vd.insert("csrf_token", ensureCsrf(req));
        vd.insert("error_msg",  std::string("Parollar mos kelmadi."));
        cb(HttpResponse::newHttpViewResponse("Register", vd));
        return;
    }

    // card_id avtomatik: "TATU" + UUID ning birinchi 6 belgisi
    std::string uid = drogon::utils::getUuid();
    uid.erase(std::remove(uid.begin(), uid.end(), '-'), uid.end());
    std::string cardId = "TATU" + uid.substr(0, 6);
    std::transform(cardId.begin(), cardId.end(), cardId.begin(), ::toupper);

    std::string email = req->getParameter("email");

    Json::Value body;
    body["fullname"] = fullname;
    body["card_id"]  = cardId;
    body["phone"]    = phone;
    body["password"] = password;
    if (!email.empty()) body["email"] = email;

    apiPost("/api/readers/register/", body,
        [req, cb = std::move(cb)](int status, const Json::Value& data) mutable {
            if (status == 200 || status == 201) {
                cb(redirect("/kirish?registered=1"));
            } else {
                std::string err;
                if (data.isObject()) {
                    for (const auto& key : data.getMemberNames()) {
                        const auto& v = data[key];
                        if (v.isArray() && !v.empty())
                            err += key + ": " + v[0].asString() + " ";
                        else if (v.isString())
                            err += key + ": " + v.asString() + " ";
                    }
                }
                if (err.empty()) err = "Ro'yxatdan o'tishda xato.";
                HttpViewData vd;
                vd.insert("csrf_token", ensureCsrf(req));
                vd.insert("error_msg",  err);
                cb(HttpResponse::newHttpViewResponse("Register", vd));
            }
        });
}

// ── Chiqish ───────────────────────────────────────────────────────────────────
void AuthController::logout(const HttpRequestPtr& req,
                            std::function<void(const HttpResponsePtr&)>&& cb)
{
    auto s = req->session();
    s->erase("reader_token");
    s->erase("reader_name");
    s->erase("reader_card_id");
    s->erase("reader_approved");
    s->erase("reader_id");
    cb(redirect("/"));
}
