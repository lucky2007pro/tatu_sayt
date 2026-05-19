#pragma once
#include <map>
#include <string>

// CSP view fayllar uchun xavfsiz map accessor
inline std::string getOr(const std::map<std::string, std::string>& r,
                          const std::string& k,
                          const std::string& def = "") {
    auto it = r.find(k);
    return (it != r.end()) ? it->second : def;
}
