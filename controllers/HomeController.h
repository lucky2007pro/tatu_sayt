#pragma once
#include <drogon/HttpController.h>
using namespace drogon;

class HomeController : public HttpController<HomeController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(HomeController::index,      "/",              Get);
        ADD_METHOD_TO(HomeController::statistics, "/statistika",   Get);
        ADD_METHOD_TO(HomeController::libraries,  "/kutubxonalar", Get);
        ADD_METHOD_TO(HomeController::about,          "/haqida",              Get);
        ADD_METHOD_TO(HomeController::libraryDetail,   "/kutubxona/{id}",      Get);
        ADD_METHOD_TO(HomeController::favourites,      "/sevimlilar",          Get);
        ADD_METHOD_TO(HomeController::favouriteToggle, "/sevimli/{id}/toggle", Post);
        ADD_METHOD_TO(HomeController::favouriteRemove, "/sevimli/{id}/ochir",  Post);
        ADD_METHOD_TO(HomeController::favouriteIds,    "/api/my-favourite-ids", Get);
        ADD_METHOD_TO(HomeController::bookSearch,      "/api/book-search",     Get);
    METHOD_LIST_END

    void index         (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void statistics    (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void libraries     (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void about         (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void libraryDetail (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void favourites    (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void favouriteToggle(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void favouriteRemove(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
    void favouriteIds  (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void bookSearch    (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
};
