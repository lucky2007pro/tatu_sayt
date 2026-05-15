#pragma once
#include <drogon/HttpController.h>
using namespace drogon;

class AdminController : public HttpController<AdminController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AdminController::loginForm,    "/admin",           Get);
        ADD_METHOD_TO(AdminController::handleLogin,  "/admin",           Post);
        ADD_METHOD_TO(AdminController::dashboard,    "/admin/dashboard", Get);
        ADD_METHOD_TO(AdminController::readers,      "/admin/readers",   Get);
        ADD_METHOD_TO(AdminController::approveReader,"/admin/readers/{id}/holat", Post);
        ADD_METHOD_TO(AdminController::books,        "/admin/books",     Get);
        ADD_METHOD_TO(AdminController::deleteBook,   "/admin/books/{id}/ochir",  Post);
        ADD_METHOD_TO(AdminController::addBookForm,   "/admin/books/yangi",       Get);
        ADD_METHOD_TO(AdminController::addBook,       "/admin/books/yangi",       Post);
        ADD_METHOD_TO(AdminController::editBookForm,  "/admin/books/{id}/tahrir", Get);
        ADD_METHOD_TO(AdminController::editBook,      "/admin/books/{id}/tahrir", Post);
        ADD_METHOD_TO(AdminController::cards,             "/admin/kartalar",            Get);
        ADD_METHOD_TO(AdminController::cardAction,        "/admin/kartalar/{id}/holat", Post);
        ADD_METHOD_TO(AdminController::reservations,      "/admin/bronlar",             Get);
        ADD_METHOD_TO(AdminController::cancelReservation, "/admin/bronlar/{id}/bekor",  Post);
        ADD_METHOD_TO(AdminController::issues,            "/admin/berishlar",                  Get);
        ADD_METHOD_TO(AdminController::addIssue,          "/admin/berishlar/yangi",            Post);
        ADD_METHOD_TO(AdminController::returnBook,        "/admin/berishlar/{id}/qaytarildi",  Post);
        ADD_METHOD_TO(AdminController::logout,            "/admin/chiqish",             Get);
        ADD_METHOD_TO(AdminController::analytics,         "/admin/analitika",           Get);
        ADD_METHOD_TO(AdminController::exportCsv,         "/admin/csv/{type}",          Get);
    METHOD_LIST_END

    void loginForm        (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void handleLogin      (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void dashboard        (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void readers          (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void approveReader    (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void books            (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void deleteBook       (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void addBookForm     (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void addBook         (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void editBookForm    (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void editBook        (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void cards            (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void cardAction       (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void reservations     (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void cancelReservation(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void issues           (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void addIssue         (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void returnBook       (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void logout           (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void analytics        (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void exportCsv        (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, const std::string& type);
};
