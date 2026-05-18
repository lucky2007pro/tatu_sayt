#define NOMINMAX
#include "HomeController.h"
#include "ApiHelper.h"
#include <drogon/HttpViewData.h>
#include <drogon/utils/Utilities.h>

// ── Asosiy sahifa: kitoblar ro'yxati ─────────────────────────────────────────
void HomeController::index(const HttpRequestPtr& req,
                           std::function<void(const HttpResponsePtr&)>&& cb)
{
    std::string search  = req->getParameter("q");
    std::string library = req->getParameter("library");
    std::string section = req->getParameter("section");
    std::string status  = req->getParameter("status");
    std::string sort    = req->getParameter("sort");
    std::string author  = req->getParameter("author");
    std::string isbn    = req->getParameter("isbn");
    std::string pageStr = req->getParameter("page");
    int page = 1;
    if (!pageStr.empty()) {
        try { page = std::stoi(pageStr); if (page < 1) page = 1; } catch (...) { page = 1; }
    }

    // URL parametrlarini yig'ish — search bo'sh joy va maxsus belgilarni encode qilish
    std::string booksPath = "/api/books/?page=" + std::to_string(page);
    if (!search.empty())  booksPath += "&search="  + drogon::utils::urlEncode(search);
    if (!library.empty()) booksPath += "&library=" + drogon::utils::urlEncode(library);
    if (!section.empty()) booksPath += "&section=" + drogon::utils::urlEncode(section);
    if (!status.empty())  booksPath += "&status="  + drogon::utils::urlEncode(status);
    if (!sort.empty())    booksPath += "&ordering=" + drogon::utils::urlEncode(sort);
    if (!author.empty())  booksPath += "&author="   + drogon::utils::urlEncode(author);
    if (!isbn.empty())    booksPath += "&isbn="     + drogon::utils::urlEncode(isbn);

    bool loggedIn    = isLoggedIn(req);
    std::string name = sessionStr(req, "reader_name", "Mehmon");
    bool approved    = sessionBool(req, "reader_approved");
    std::string csrf = ensureCsrf(req);

    // 1. Kitoblar
    apiGet(booksPath, [loggedIn, name, approved, csrf,
                       search, library, section, status, sort, page, author, isbn,
                       cb = std::move(cb)](bool ok, const Json::Value& booksJson) mutable
    {
        RowList books;
        std::string hasNext, hasPrev;
        int totalCount = 0;

        if (ok) {
            if (booksJson.isObject()) {
                totalCount = booksJson["count"].isInt() ? booksJson["count"].asInt() : 0;
                hasNext = (booksJson["next"].isString() && !booksJson["next"].asString().empty()) ? "1" : "";
                hasPrev = (booksJson["previous"].isString() && !booksJson["previous"].asString().empty()) ? "1" : "";
            }
            books = jsonArrayToRows(booksJson, {"id","title","author_name","library_name",
                                                "section_name","is_available","availability_status",
                                                "cover_image","average_rating","ebook_file","view_count"});
        }

        // 2. Kutubxonalar (filtr uchun)
        apiGet("/api/libraries/",
            [loggedIn, name, approved, csrf,
             search, library, section, status, sort, page, author, isbn,
             hasNext, hasPrev, totalCount,
             cb = std::move(cb), books = std::move(books)](bool, const Json::Value& libJson) mutable
        {
            RowList libraries = jsonArrayToRows(libJson, {"id","name"});

            // 3. Bo'limlar
            apiGet("/api/sections/",
                [loggedIn, name, approved, csrf,
                 search, library, section, status, sort, page, author, isbn,
                 hasNext, hasPrev, totalCount,
                 cb = std::move(cb), books = std::move(books),
                 libraries = std::move(libraries)](bool, const Json::Value& secJson) mutable
            {
                RowList sections = jsonArrayToRows(secJson, {"id","name"});

                // 4. Popular kitoblar
                apiGet("/api/books/popular/",
                    [loggedIn, name, approved, csrf,
                     search, library, section, status, sort, page, author, isbn,
                     hasNext, hasPrev, totalCount,
                     cb = std::move(cb), books = std::move(books),
                     libraries = std::move(libraries),
                     sections = std::move(sections)](bool, const Json::Value& popJson) mutable
                {
                    RowList popular = jsonArrayToRows(popJson, {"id","title","author_name","average_rating","view_count"});
                    if (popular.size() > 6) popular.resize(6);

                    HttpViewData data;
                    data.insert("books",       books);
                    data.insert("popular",     popular);
                    data.insert("libraries",   libraries);
                    data.insert("sections",    sections);
                    data.insert("total_count", std::to_string(totalCount));
                    data.insert("has_next",    hasNext);
                    data.insert("has_prev",    hasPrev);
                    data.insert("page",        std::to_string(page));
                    data.insert("search",      search);
                    data.insert("sel_library", library);
                    data.insert("sel_section", section);
                    data.insert("sel_status",  status);
                    data.insert("sel_sort",    sort);
                    data.insert("sel_author",  author);
                    data.insert("sel_isbn",    isbn);
                    data.insert("is_logged_in",loggedIn);
                    data.insert("reader_name", name);
                    data.insert("reader_approved", approved);
                    data.insert("csrf_token",  csrf);

                    cb(HttpResponse::newHttpViewResponse("Index", data));
                });
            });
        });
    });
}

// ── Statistika ────────────────────────────────────────────────────────────────
void HomeController::statistics(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb)
{
    bool loggedIn = isLoggedIn(req);
    std::string name = sessionStr(req, "reader_name", "Mehmon");
    bool approved = sessionBool(req, "reader_approved");

    apiGet("/api/statistics/",
        [loggedIn, name, approved, cb = std::move(cb)](bool, const Json::Value& statsJson) mutable
    {
        apiGet("/api/books/popular/",
            [loggedIn, name, approved, statsJson, cb = std::move(cb)](bool, const Json::Value& popJson) mutable
        {
            RowList popular = jsonArrayToRows(popJson, {"id","title","author_name",
                                                        "library_name","average_rating","view_count"});

            HttpViewData data;
            data.insert("total_books",        jstr(statsJson, "total_books", "0"));
            data.insert("total_readers",      jstr(statsJson, "total_readers", "0"));
            data.insert("total_reservations", jstr(statsJson, "total_reservations", "0"));
            data.insert("total_issues",       jstr(statsJson, "total_issues", "0"));
            data.insert("total_libraries",    jstr(statsJson, "total_libraries", "0"));
            data.insert("total_sections",     jstr(statsJson, "total_sections", "0"));
            data.insert("popular",            popular);
            data.insert("is_logged_in",       loggedIn);
            data.insert("reader_name",        name);
            data.insert("reader_approved",    approved);

            cb(HttpResponse::newHttpViewResponse("Statistics", data));
        });
    });
}

// ── Kutubxonalar ──────────────────────────────────────────────────────────────
void HomeController::libraries(const HttpRequestPtr& req,
                               std::function<void(const HttpResponsePtr&)>&& cb)
{
    bool loggedIn = isLoggedIn(req);
    std::string name = sessionStr(req, "reader_name", "Mehmon");

    apiGet("/api/libraries/",
        [loggedIn, name, cb = std::move(cb)](bool, const Json::Value& libJson) mutable
    {
        RowList libs = jsonArrayToRows(libJson,
            {"id","name","latitude","longitude"});

        HttpViewData data;
        data.insert("libraries",   libs);
        data.insert("is_logged_in",loggedIn);
        data.insert("reader_name", name);

        cb(HttpResponse::newHttpViewResponse("Libraries", data));
    });
}

// ── Kutubxona detail ─────────────────────────────────────────────────────────
void HomeController::libraryDetail(const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& cb,
                                   int id)
{
    bool loggedIn = isLoggedIn(req);
    std::string name = sessionStr(req, "reader_name", "Mehmon");

    std::string libPath = "/api/libraries/" + std::to_string(id) + "/";
    apiGet(libPath,
        [loggedIn, name, id, cb = std::move(cb)](bool ok, const Json::Value& libJson) mutable
    {
        if (!ok || !libJson.isObject()) { cb(redirect("/kutubxonalar")); return; }

        Row lib = jsonObjectToRow(libJson,
            {"id","name","address","latitude","longitude","phone","email","working_hours"});

        std::string booksPath = "/api/books/?library=" + std::to_string(id) + "&page_size=20";
        apiGet(booksPath,
            [loggedIn, name, lib, cb = std::move(cb)](bool, const Json::Value& bj) mutable
        {
            RowList books = jsonArrayToRows(bj,
                {"id","title","author_name","section_name","is_available","availability_status","cover_image","average_rating"});

            HttpViewData data;
            data.insert("library",     lib);
            data.insert("books",       books);
            data.insert("is_logged_in",loggedIn);
            data.insert("reader_name", name);
            cb(HttpResponse::newHttpViewResponse("LibraryDetail", data));
        });
    });
}

// ── Sevimlilar ────────────────────────────────────────────────────────────────
void HomeController::favourites(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb)
{
    if (!isLoggedIn(req)) { cb(redirect("/kirish?next=/sevimlilar")); return; }

    bool loggedIn = true;
    std::string name = sessionStr(req, "reader_name", "");
    bool approved    = sessionBool(req, "reader_approved");

    HttpViewData data;
    data.insert("is_logged_in",   loggedIn);
    data.insert("reader_name",    name);
    data.insert("reader_approved",approved);
    cb(HttpResponse::newHttpViewResponse("Favourites", data));
}

// ── Haqida ────────────────────────────────────────────────────────────────────
void HomeController::about(const HttpRequestPtr& req,
                           std::function<void(const HttpResponsePtr&)>&& cb)
{
    bool loggedIn = isLoggedIn(req);
    std::string name = sessionStr(req, "reader_name", "Mehmon");

    HttpViewData data;
    data.insert("is_logged_in", loggedIn);
    data.insert("reader_name",  name);
    cb(HttpResponse::newHttpViewResponse("About", data));
}
