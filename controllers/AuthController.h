#pragma once
#include <drogon/HttpController.h>
using namespace drogon;

class AuthController : public HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthController::loginForm,    "/kirish",  Get);
        ADD_METHOD_TO(AuthController::handleLogin,  "/kirish",  Post);
        ADD_METHOD_TO(AuthController::registerForm, "/royxat",  Get);
        ADD_METHOD_TO(AuthController::handleReg,    "/royxat",  Post);
        ADD_METHOD_TO(AuthController::logout,       "/chiqish", Get);
    METHOD_LIST_END

    void loginForm   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void handleLogin (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void registerForm(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void handleReg   (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void logout      (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
};
