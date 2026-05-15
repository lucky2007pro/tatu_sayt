#pragma once
#include <drogon/HttpController.h>
using namespace drogon;

class DashboardController : public HttpController<DashboardController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(DashboardController::index,  "/kabinet",           Get);
        ADD_METHOD_TO(DashboardController::cancel, "/bron/{id}/bekor",   Post);
    METHOD_LIST_END

    void index (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void cancel(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, int id);
};
