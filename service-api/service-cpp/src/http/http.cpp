#include "http/http.hpp"
#include "core/config.hpp"

namespace https_server::http {

void HttpResponse::apply_security_headers() {
    if (!security_config) return;
    
    if (security_config->enable_hsts) {
        std::string hsts_value = "max-age=" + security_config->hsts_max_age;
        if (security_config->hsts_include_subdomains) {
            hsts_value += "; includeSubDomains";
        }
        if (security_config->hsts_preload) {
            hsts_value += "; preload";
        }
        headers["Strict-Transport-Security"] = hsts_value;
    }
    
    if (security_config->enable_csp) {
        headers["Content-Security-Policy"] = security_config->csp_policy;
    }
    
    if (security_config->enable_xcto) {
        headers["X-Content-Type-Options"] = "nosniff";
    }
    
    if (security_config->enable_xfo) {
        headers["X-Frame-Options"] = "DENY";
    }
    
    headers["X-XSS-Protection"] = "1; mode=block";
    headers["Referrer-Policy"] = "strict-origin-when-cross-origin";
    headers["Permissions-Policy"] = "geolocation=(), microphone=(), camera=()";
}

} // namespace https_server::http