#include <drogon/drogon.h>
#include <iostream>
#include <filesystem>
#include <optional>

namespace {
std::optional<std::filesystem::path> findConfig()
{
    namespace fs = std::filesystem;
    fs::path cur = fs::current_path();
    for (int d = 0; d < 8; ++d) {
        auto p = cur / "config.json";
        if (fs::exists(p)) return fs::weakly_canonical(p);
        auto p2 = cur / "tatu_web" / "config.json";
        if (fs::exists(p2)) return fs::weakly_canonical(p2);
        if (!cur.has_parent_path() || cur.parent_path() == cur) break;
        cur = cur.parent_path();
    }
    return std::nullopt;
}
}

int main()
{
    try {
        trantor::Logger::setLogLevel(trantor::Logger::kInfo);

        auto cfg = findConfig();
        if (!cfg) throw std::runtime_error("config.json topilmadi");

        drogon::app().loadConfigFile(cfg->string());
        LOG_INFO << "Config: " << cfg->string();

        drogon::app().setCustom404Page(
            drogon::HttpResponse::newHttpViewResponse("NotFound"));

        LOG_INFO << "TATU Kutubxona Web — port 8080 da ishga tushmoqda...";
        drogon::app().run();
    }
    catch (const std::exception& e) {
        std::cerr << "XATO: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
