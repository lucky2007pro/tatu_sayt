#define NOMINMAX
#include "AdminController.h"
#include "ApiHelper.h"
#include <drogon/HttpViewData.h>

namespace {
bool isAdmin(const HttpRequestPtr& req) {
    return req->session()->find("admin_ok");
}
}

void AdminController::loginForm(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (isAdmin(req)) { cb(redirect("/admin/dashboard")); return; }
    HttpViewData data;
    data.insert("csrf_token", ensureCsrf(req));
    data.insert("error_msg",  std::string(""));
    cb(HttpResponse::newHttpViewResponse("AdminLogin", data));
}

void AdminController::handleLogin(const HttpRequestPtr& req,
                                  std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    std::string token = req->getParameter("admin_token");
    if (token == adminToken()) {
        req->session()->insert("admin_ok", std::string("1"));
        cb(redirect("/admin/dashboard"));
    } else {
        HttpViewData data;
        data.insert("csrf_token", ensureCsrf(req));
        data.insert("error_msg",  std::string("Noto'g'ri admin token."));
        cb(HttpResponse::newHttpViewResponse("AdminLogin", data));
    }
}

void AdminController::dashboard(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }
    apiGet("/api/statistics/",
        [cb = std::move(cb)](bool, const Json::Value& s) mutable
    {
        Row statsRow = jsonObjectToRow(s, {
            "total_books", "total_readers", "total_reservations",
            "total_libraries", "total_issues", "total_sections",
            "pending_readers", "pending_cards"
        });
        apiGet("/api/readers/",
            [cb = std::move(cb), statsRow](bool, const Json::Value& rj) mutable
        {
            RowList readers = jsonArrayToRows(rj,
                {"id","fullname","card_id","phone","is_approved","is_active"});

            HttpViewData data;
            data.insert("total_books",        getOr(statsRow, "total_books",        "0"));
            data.insert("total_readers",      getOr(statsRow, "total_readers",      "0"));
            data.insert("total_reservations", getOr(statsRow, "total_reservations", "0"));
            data.insert("total_libraries",    getOr(statsRow, "total_libraries",    "0"));
            data.insert("total_issues",       getOr(statsRow, "total_issues",       "0"));
            data.insert("pending_cards",      getOr(statsRow, "pending_cards",      "0"));
            data.insert("readers",            readers);
            cb(HttpResponse::newHttpViewResponse("AdminDashboard", data));
        }, "", true);
    }, "", true);
}

void AdminController::readers(const HttpRequestPtr& req,
                              std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }
    std::string flashMsg  = sessionStr(req, "flash_msg");
    std::string flashType = sessionStr(req, "flash_type");
    if (!flashMsg.empty()) { req->session()->erase("flash_msg"); req->session()->erase("flash_type"); }

    apiGet("/api/readers/", [=, cb = std::move(cb)](bool, const Json::Value& rj) mutable {
        RowList readers = jsonArrayToRows(rj,
            {"id","fullname","card_id","phone","is_active","created_at"});
        HttpViewData data;
        data.insert("readers",    readers);
        data.insert("flash_msg",  flashMsg);
        data.insert("flash_type", flashType);
        data.insert("csrf_token", ensureCsrf(req));
        cb(HttpResponse::newHttpViewResponse("AdminReaders", data));
    }, "", true);
}

void AdminController::approveReader(const HttpRequestPtr& req,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    int id)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    std::string action = req->getParameter("action");
    bool makeActive = (action == "approve");

    Json::Value body;
    body["is_active"] = makeActive;

    // Django: PATCH /api/readers/<id>/  body: {is_active: true/false}
    apiPatch("/api/readers/" + std::to_string(id) + "/", body,
        [req, cb = std::move(cb), makeActive](int status, const Json::Value&) mutable {
            bool ok = status < 400;
            req->session()->insert("flash_msg",
                ok ? std::string(makeActive ? "O'quvchi tasdiqlandi." : "O'quvchi bloklandi.")
                   : std::string("Xato yuz berdi."));
            req->session()->insert("flash_type",
                ok ? std::string("success") : std::string("error"));
            cb(redirect("/admin/readers"));
        }, "", true);
}

void AdminController::books(const HttpRequestPtr& req,
                            std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }
    std::string flashMsg  = sessionStr(req, "flash_msg");
    std::string flashType = sessionStr(req, "flash_type");
    if (!flashMsg.empty()) { req->session()->erase("flash_msg"); req->session()->erase("flash_type"); }

    apiGet("/api/books/", [=, cb = std::move(cb)](bool, const Json::Value& bj) mutable {
        RowList books = jsonArrayToRows(bj,
            {"id","title","author_name","library_name","section_name","is_available",
             "cover_image","ebook_file"});
        HttpViewData data;
        data.insert("books",      books);
        data.insert("flash_msg",  flashMsg);
        data.insert("flash_type", flashType);
        data.insert("csrf_token", ensureCsrf(req));
        cb(HttpResponse::newHttpViewResponse("AdminBooks", data));
    }, "", true);
}

void AdminController::deleteBook(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 int id)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    apiDelete("/api/books/" + std::to_string(id) + "/",
        [req, cb = std::move(cb)](int status) mutable {
            bool ok = status < 400;
            req->session()->insert("flash_msg",
                ok ? std::string("Kitob o'chirildi.") : std::string("O'chirishda xato."));
            req->session()->insert("flash_type",
                ok ? std::string("success") : std::string("error"));
            cb(redirect("/admin/books"));
        }, "", true);
}

// ── Kitob qo'shish (forma) ───────────────────────────────────────────────────
void AdminController::addBookForm(const HttpRequestPtr& req,
                                  std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    apiGet("/api/libraries/",
        [req, cb = std::move(cb)](bool, const Json::Value& libJson) mutable
    {
        RowList libs = jsonArrayToRows(libJson, {"id","name"});
        apiGet("/api/sections/",
            [req, libs, cb = std::move(cb)](bool, const Json::Value& secJson) mutable
        {
            RowList secs = jsonArrayToRows(secJson, {"id","name"});
            HttpViewData data;
            data.insert("libraries",  libs);
            data.insert("sections",   secs);
            data.insert("csrf_token", ensureCsrf(req));
            data.insert("edit_mode",  false);
            data.insert("book",       Row{});
            cb(HttpResponse::newHttpViewResponse("AdminBookForm", data));
        });
    });
}

void AdminController::addBook(const HttpRequestPtr& req,
                              std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    const auto& bv = req->getBody();
    std::string rawBody(bv.data(), bv.size());
    std::string ct = req->getHeader("Content-Type");

    apiPostMultipartRaw("/api/books/", rawBody, ct,
        [req, cb = std::move(cb)](int status, const Json::Value&) mutable {
            bool ok = status < 400;
            req->session()->insert("flash_msg",
                ok ? std::string("Kitob muvaffaqiyatli qo'shildi.")
                   : std::string("Kitob qo'shishda xato."));
            req->session()->insert("flash_type",
                ok ? std::string("success") : std::string("error"));
            cb(redirect("/admin/books"));
        }, true);
}

// ── Kitob tahrirlash ─────────────────────────────────────────────────────────
void AdminController::editBookForm(const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& cb,
                                   int id)
{
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    apiGet("/api/books/" + std::to_string(id) + "/",
        [req, id, cb = std::move(cb)](bool ok, const Json::Value& bookJson) mutable
    {
        if (!ok) { cb(redirect("/admin/books")); return; }
        Row book = jsonObjectToRow(bookJson,
            {"id","title","author_name","isbn","published_date","description",
             "library","section","shelf","row","cover_image","ebook_file",
             "language","total_pages"});

        apiGet("/api/libraries/",
            [req, book, cb = std::move(cb)](bool, const Json::Value& lj) mutable
        {
            RowList libs = jsonArrayToRows(lj, {"id","name"});
            apiGet("/api/sections/",
                [req, book, libs, cb = std::move(cb)](bool, const Json::Value& sj) mutable
            {
                RowList secs = jsonArrayToRows(sj, {"id","name"});
                HttpViewData data;
                data.insert("book",       book);
                data.insert("libraries",  libs);
                data.insert("sections",   secs);
                data.insert("csrf_token", ensureCsrf(req));
                data.insert("edit_mode",  true);
                cb(HttpResponse::newHttpViewResponse("AdminBookForm", data));
            });
        });
    }, "", true);
}

void AdminController::editBook(const HttpRequestPtr& req,
                               std::function<void(const HttpResponsePtr&)>&& cb,
                               int id)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    const auto& bv = req->getBody();
    std::string rawBody(bv.data(), bv.size());
    std::string ct = req->getHeader("Content-Type");

    apiPatchMultipartRaw("/api/books/" + std::to_string(id) + "/", rawBody, ct,
        [req, id, cb = std::move(cb)](int status, const Json::Value&) mutable {
            bool ok = status < 400;
            req->session()->insert("flash_msg",
                ok ? std::string("Kitob yangilandi.")
                   : std::string("Kitob yangilashda xato."));
            req->session()->insert("flash_type",
                ok ? std::string("success") : std::string("error"));
            cb(redirect("/admin/books"));
        }, true);
}

// ── Kutubxona kartalari ──────────────────────────────────────────────────────
void AdminController::cards(const HttpRequestPtr& req,
                            std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }
    std::string flashMsg  = sessionStr(req, "flash_msg");
    std::string flashType = sessionStr(req, "flash_type");
    if (!flashMsg.empty()) { req->session()->erase("flash_msg"); req->session()->erase("flash_type"); }

    apiGet("/api/library-cards-admin/", [=, cb = std::move(cb)](bool, const Json::Value& cj) mutable {
        RowList cards = jsonArrayToRows(cj,
            {"id","reader_id","reader_name","reader_card_id","reader_phone",
             "library_name","card_image","is_approved","created_at"});

        int pendingCount = 0;
        for (auto& c : cards) {
            auto it = c.find("is_approved");
            if (it != c.end() && it->second == "False") pendingCount++;
        }

        HttpViewData data;
        data.insert("cards",         cards);
        data.insert("pending_count", pendingCount);
        data.insert("flash_msg",     flashMsg);
        data.insert("flash_type",    flashType);
        data.insert("csrf_token",    ensureCsrf(req));
        cb(HttpResponse::newHttpViewResponse("AdminCards", data));
    }, "", true);
}

void AdminController::cardAction(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 int id)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    std::string action = req->getParameter("action");
    bool approve = (action == "approve");

    Json::Value body;
    body["is_approved"] = approve;

    // Django: PATCH /api/library-cards-admin/<id>/  body: {is_approved: true/false}
    apiPatch("/api/library-cards-admin/" + std::to_string(id) + "/", body,
        [req, cb = std::move(cb), approve](int status, const Json::Value&) mutable {
            bool ok = status < 400;
            req->session()->insert("flash_msg",
                ok ? std::string(approve ? "Karta tasdiqlandi." : "Karta tasdiqi olib tashlandi.")
                   : std::string("Xato yuz berdi."));
            req->session()->insert("flash_type",
                ok ? std::string("success") : std::string("error"));
            cb(redirect("/admin/kartalar"));
        }, "", true);
}

// ── Admin Bronlar ─────────────────────────────────────────────────────────────
void AdminController::reservations(const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    std::string csrf     = ensureCsrf(req);
    std::string flashMsg = sessionStr(req, "flash_msg");
    std::string flashType= sessionStr(req, "flash_type");
    if (!flashMsg.empty()) {
        req->session()->erase("flash_msg");
        req->session()->erase("flash_type");
    }

    apiGet("/api/reservations/",
        [csrf, flashMsg, flashType, cb = std::move(cb)](bool, const Json::Value& rj) mutable
    {
        RowList reservations = jsonArrayToRows(rj,
            {"id","book","book_title","reader","reader_name","reserved_at"});

        HttpViewData data;
        data.insert("reservations", reservations);
        data.insert("flash_msg",    flashMsg);
        data.insert("flash_type",   flashType);
        data.insert("csrf_token",   csrf);

        cb(HttpResponse::newHttpViewResponse("AdminReservations", data));
    }, "", true);
}

void AdminController::cancelReservation(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& cb,
                                        int id)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    apiDelete("/api/reservations/" + std::to_string(id) + "/",
        [req, cb = std::move(cb)](int status) mutable {
            bool ok = status >= 200 && status < 300;
            req->session()->insert("flash_msg",
                ok ? std::string("Bron bekor qilindi.")
                   : std::string("Bronni bekor qilishda xato."));
            req->session()->insert("flash_type",
                ok ? std::string("success") : std::string("error"));
            cb(redirect("/admin/bronlar"));
        }, "", true);
}

// ── Admin Berishlar ───────────────────────────────────────────────────────────
void AdminController::issues(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    std::string csrf     = ensureCsrf(req);
    std::string flashMsg = sessionStr(req, "flash_msg");
    std::string flashType= sessionStr(req, "flash_type");
    if (!flashMsg.empty()) {
        req->session()->erase("flash_msg");
        req->session()->erase("flash_type");
    }

    apiGet("/api/issues/",
        [csrf, flashMsg, flashType, cb = std::move(cb)](bool, const Json::Value& ij) mutable
    {
        RowList issues = jsonArrayToRows(ij,
            {"id","book","book_title","reader","reader_name","issue_date","return_date"});

        apiGet("/api/readers/",
            [csrf, flashMsg, flashType, issues, cb = std::move(cb)](bool, const Json::Value& rj) mutable
        {
            RowList readers = jsonArrayToRows(rj, {"id","fullname","card_id"});

            apiGet("/api/books/",
                [csrf, flashMsg, flashType, issues, readers, cb = std::move(cb)](bool, const Json::Value& bj) mutable
            {
                RowList books = jsonArrayToRows(bj, {"id","title"});

                HttpViewData data;
                data.insert("issues",    issues);
                data.insert("readers",   readers);
                data.insert("books",     books);
                data.insert("flash_msg", flashMsg);
                data.insert("flash_type",flashType);
                data.insert("csrf_token",csrf);

                cb(HttpResponse::newHttpViewResponse("AdminIssues", data));
            }, "", true);
        }, "", true);
    }, "", true);
}

void AdminController::addIssue(const HttpRequestPtr& req,
                               std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!checkCsrf(req)) { cb(errResp(403, "CSRF xato")); return; }
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    std::string readerIdStr  = req->getParameter("reader");
    std::string bookIdStr    = req->getParameter("book");
    std::string returnDate   = req->getParameter("return_date");

    if (readerIdStr.empty() || bookIdStr.empty() || returnDate.empty()) {
        req->session()->insert("flash_msg",  std::string("Barcha maydonlar to'ldirilishi shart."));
        req->session()->insert("flash_type", std::string("error"));
        cb(redirect("/admin/berishlar"));
        return;
    }

    Json::Value body;
    body["reader"]      = std::stoi(readerIdStr);
    body["book"]        = std::stoi(bookIdStr);
    body["return_date"] = returnDate;

    apiPost("/api/issues/", body,
        [req, cb = std::move(cb)](int status, const Json::Value& data) mutable {
            bool ok = status < 400;
            std::string msg = ok ? "Kitob muvaffaqiyatli berildi." : "Kitob berishda xato yuz berdi.";
            if (!ok && data.isObject() && data.isMember("detail") && data["detail"].isString())
                msg = data["detail"].asString();
            req->session()->insert("flash_msg",  msg);
            req->session()->insert("flash_type", ok ? std::string("success") : std::string("error"));
            cb(redirect("/admin/berishlar"));
        }, "", true);
}

void AdminController::logout(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb)
{
    req->session()->erase("admin_ok");
    cb(redirect("/admin"));
}

// ── Admin Analitika ───────────────────────────────────────────────────────────
void AdminController::analytics(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isAdmin(req)) { cb(redirect("/admin")); return; }

    apiGet("/api/statistics/",
        [cb = std::move(cb)](bool, const Json::Value& stats) mutable
    {
        apiGet("/api/books/popular/",
            [stats, cb = std::move(cb)](bool, const Json::Value& pop) mutable
        {
            apiGet("/api/books/?page_size=5&ordering=-view_count",
                [stats, pop, cb = std::move(cb)](bool, const Json::Value& topRead) mutable
            {
                apiGet("/api/reservations/?page_size=200",
                    [stats, pop, topRead, cb = std::move(cb)](bool, const Json::Value& resJson) mutable
                {
                    using Row = std::map<std::string, std::string>;
                    using RowList = std::vector<Row>;

                    RowList popular = jsonArrayToRows(pop,
                        {"id","title","author_name","library_name","average_rating","view_count"});
                    if (popular.size() > 8) popular.resize(8);

                    HttpViewData data;
                    data.insert("total_books",        jstr(stats, "total_books", "0"));
                    data.insert("total_readers",      jstr(stats, "total_readers", "0"));
                    data.insert("total_reservations", jstr(stats, "total_reservations", "0"));
                    data.insert("total_issues",       jstr(stats, "total_issues", "0"));
                    data.insert("total_libraries",    jstr(stats, "total_libraries", "0"));
                    data.insert("popular",            popular);

                    cb(HttpResponse::newHttpViewResponse("AdminAnalytics", data));
                });
            });
        });
    });
}

// ── CSV eksport ───────────────────────────────────────────────────────────────
void AdminController::exportCsv(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                const std::string& type)
{
    if (!isAdmin(req)) { cb(errResp(403, "Ruxsat yo'q")); return; }

    std::string apiPath;
    std::string filename;
    std::vector<std::string> fields;

    if (type == "books") {
        apiPath = "/api/books/?page_size=10000";
        filename = "kitoblar.csv";
        fields = {"id","title","author_name","library_name","section_name","is_available","view_count"};
    } else if (type == "readers") {
        apiPath = "/api/readers/?page_size=10000";
        filename = "oqituvchilar.csv";
        fields = {"id","fullname","phone","card_id","is_approved","is_active"};
    } else if (type == "reservations") {
        apiPath = "/api/reservations/?page_size=10000";
        filename = "bronlar.csv";
        fields = {"id","reader_name","book_title","reserved_at","note"};
    } else if (type == "issues") {
        apiPath = "/api/issues/?page_size=10000";
        filename = "berishlar.csv";
        fields = {"id","reader_name","book_title","issue_date","return_date"};
    } else {
        cb(errResp(400, "Noma'lum tur")); return;
    }

    apiGet(apiPath,
        [fields, filename, cb = std::move(cb)](bool ok, const Json::Value& j) mutable
    {
        if (!ok) { cb(errResp(502, "API xatosi")); return; }

        // CSV sarlavhasi
        std::string csv;
        for (size_t i = 0; i < fields.size(); i++) {
            if (i) csv += ",";
            csv += fields[i];
        }
        csv += "\r\n";

        // Ma'lumotlar
        const Json::Value& arr = j.isObject() && j.isMember("results") ? j["results"] : j;
        if (arr.isArray()) {
            for (const auto& row : arr) {
                for (size_t i = 0; i < fields.size(); i++) {
                    if (i) csv += ",";
                    std::string val = row.isMember(fields[i]) && !row[fields[i]].isNull()
                        ? (row[fields[i]].isString() ? row[fields[i]].asString()
                           : row[fields[i]].asBool() ? "true" : row[fields[i]].asString())
                        : "";
                    // CSV quoting
                    if (val.find(',') != std::string::npos ||
                        val.find('"') != std::string::npos ||
                        val.find('\n') != std::string::npos) {
                        std::string escaped;
                        for (char c : val) { if (c == '"') escaped += "\"\""; else escaped += c; }
                        val = "\"" + escaped + "\"";
                    }
                    csv += val;
                }
                csv += "\r\n";
            }
        }

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeString("text/csv; charset=utf-8");
        resp->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
        resp->setBody(csv);
        cb(resp);
    });
}
