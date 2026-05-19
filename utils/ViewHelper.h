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

// Premium head — barcha sahifalarda ishlatiladi
#define PREMIUM_HEAD \
R"(<meta charset="UTF-8">\n\
<meta name="viewport" content="width=device-width, initial-scale=1.0">\n\
<link rel="manifest" href="/manifest.json">\n\
<meta name="theme-color" content="#2563eb">\n\
<link rel="icon" type="image/svg+xml" href="/icons/icon-192.svg">\n\
<link rel="apple-touch-icon" href="/icons/icon-192.svg">\n\
<script src="https://cdn.tailwindcss.com"></script>\n\
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800;900&display=swap" rel="stylesheet">\n\
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css"/>\n\
<link rel="stylesheet" href="/premium.css">\n)"
