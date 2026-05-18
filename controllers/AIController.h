#pragma once
#include <drogon/HttpController.h>
using namespace drogon;

class AIController : public HttpController<AIController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AIController::advisor,    "/ai-maslahatchi",          Get);
        ADD_METHOD_TO(AIController::chatProxy,  "/api/ai-chat",             Post);
        ADD_METHOD_TO(AIController::statusJson, "/api/ai-status",           Get);
    METHOD_LIST_END

    void advisor    (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void chatProxy  (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void statusJson (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
};
