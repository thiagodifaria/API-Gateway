#include "middleware/auth.hpp"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <ctime>
#include <openssl/evp.h>
#include <openssl/hmac.h>

using json = nlohmann::json;

namespace https_server::middleware {

Authenticator::Authenticator(ServerConfig::Auth config) : config_(std::move(config)) {}

http::HttpResponse Authenticator::authorize(const http::HttpRequest& request, bool required) const {
    http::HttpResponse ok;
    if (!required && !config_.enabled) {
        return ok;
    }

    bool authorized = validate_api_key(request);
    if (!authorized) {
        const auto auth = request.headers.find("authorization");
        if (auth != request.headers.end() && auth->second.find("Bearer ") == 0) {
            authorized = validate_jwt(auth->second.substr(7));
        }
    }

    if (authorized) {
        return ok;
    }

    http::HttpResponse denied;
    denied.status_code = 401;
    denied.status_text = "Unauthorized";
    denied.headers["Content-Type"] = "application/json; charset=utf-8";
    denied.headers["WWW-Authenticate"] = "Bearer";
    denied.body = "{\"error\":\"unauthorized\",\"message\":\"Authentication required\"}";
    return denied;
}

bool Authenticator::validate_api_key(const http::HttpRequest& request) const {
    if (config_.api_keys.empty()) {
        return false;
    }
    std::string header = config_.api_key_header;
    std::transform(header.begin(), header.end(), header.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    const auto it = request.headers.find(header);
    if (it == request.headers.end()) {
        return false;
    }
    return std::any_of(config_.api_keys.begin(), config_.api_keys.end(), [&](const std::string& key) {
        return constant_time_equals(key, it->second);
    });
}

std::string Authenticator::base64url_decode(std::string input) {
    std::replace(input.begin(), input.end(), '-', '+');
    std::replace(input.begin(), input.end(), '_', '/');
    while (input.size() % 4 != 0) {
        input.push_back('=');
    }
    std::string output((input.size() * 3) / 4 + 4, '\0');
    const int written = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(&output[0]),
                                        reinterpret_cast<const unsigned char*>(input.data()),
                                        static_cast<int>(input.size()));
    if (written < 0) {
        return {};
    }
    output.resize(static_cast<std::size_t>(written));
    while (!output.empty() && output.back() == '\0') {
        output.pop_back();
    }
    return output;
}

bool Authenticator::constant_time_equals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }
    unsigned char diff = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return diff == 0;
}

bool Authenticator::validate_jwt(const std::string& token) const {
    if (config_.jwt_hmac_secret.empty()) {
        return false;
    }

    const auto first = token.find('.');
    const auto second = first == std::string::npos ? std::string::npos : token.find('.', first + 1);
    if (first == std::string::npos || second == std::string::npos) {
        return false;
    }

    const std::string signed_part = token.substr(0, second);
    const std::string signature = token.substr(second + 1);

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    HMAC(EVP_sha256(),
         config_.jwt_hmac_secret.data(),
         static_cast<int>(config_.jwt_hmac_secret.size()),
         reinterpret_cast<const unsigned char*>(signed_part.data()),
         signed_part.size(),
         digest,
         &digest_len);

    std::string encoded(((digest_len + 2) / 3) * 4, '\0');
    const int encoded_len = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(&encoded[0]), digest, digest_len);
    encoded.resize(static_cast<std::size_t>(encoded_len));
    std::replace(encoded.begin(), encoded.end(), '+', '-');
    std::replace(encoded.begin(), encoded.end(), '/', '_');
    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }
    if (!constant_time_equals(encoded, signature)) {
        return false;
    }

    try {
        const auto payload = json::parse(base64url_decode(token.substr(first + 1, second - first - 1)));
        const auto now = static_cast<long long>(std::time(nullptr));
        if (payload.contains("exp") && payload["exp"].get<long long>() < now) return false;
        if (payload.contains("nbf") && payload["nbf"].get<long long>() > now) return false;
        if (!config_.issuer.empty() && (!payload.contains("iss") || payload["iss"].get<std::string>() != config_.issuer)) return false;
        if (!config_.audience.empty() && (!payload.contains("aud") || payload["aud"].get<std::string>() != config_.audience)) return false;
    } catch (const std::exception&) {
        return false;
    }

    return true;
}

} // namespace https_server::middleware
