#pragma once
#include <drogon/HttpController.h>
using namespace drogon;

class ProfileController : public HttpController<ProfileController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ProfileController::show,        "/profil",             Get);
        ADD_METHOD_TO(ProfileController::editProfile, "/profil/edit",        Post);
        ADD_METHOD_TO(ProfileController::uploadCard,  "/profil/karta",       Post);
    METHOD_LIST_END

    void show        (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void editProfile (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
    void uploadCard  (const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb);
};
