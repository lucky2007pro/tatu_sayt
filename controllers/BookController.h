#pragma once
#include <drogon/HttpController.h>
using namespace drogon;

class BookController : public HttpController<BookController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(BookController::detail,         "/kitob/{id}",               Get);
        ADD_METHOD_TO(BookController::reserve,        "/kitob/{id}/band",          Post);
        ADD_METHOD_TO(BookController::rateBook,       "/kitob/{id}/baholash",      Post);
        ADD_METHOD_TO(BookController::waitlist,       "/kitob/{id}/navbat",        Post);
        ADD_METHOD_TO(BookController::removeWaitlist, "/kitob/{id}/navbat/chiqish",Post);
    METHOD_LIST_END

    void detail         (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void reserve        (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void rateBook       (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void waitlist       (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void removeWaitlist (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
};
