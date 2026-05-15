#include "AuthFilter.h"

void AuthFilter::doFilter(const HttpRequestPtr& req,
                          FilterCallback&&      fcb,
                          FilterChainCallback&& fccb)
{
    if (req->session()->find("reader_token")) {
        fccb(); // o'tkazib yuborish
    } else {
        auto resp = HttpResponse::newRedirectionResponse("/kirish");
        fcb(resp);
    }
}
