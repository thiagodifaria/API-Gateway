#include "middleware/auth.hpp"
#include <cassert>
#include <ctime>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/hmac.h>

using https_server::ServerConfig;
using https_server::http::HttpRequest;
using https_server::middleware::Authenticator;

namespace {

std::string base64url_encode(const unsigned char* data, std::size_t size) {
    std::string encoded(((size + 2) / 3) * 4, '\0');
    const int encoded_len = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(&encoded[0]),
                                            data,
                                            static_cast<int>(size));
    encoded.resize(static_cast<std::size_t>(encoded_len));
    for (char& ch : encoded) {
        if (ch == '+') ch = '-';
        if (ch == '/') ch = '_';
    }
    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }
    return encoded;
}

std::string base64url_encode(const std::string& data) {
    return base64url_encode(reinterpret_cast<const unsigned char*>(data.data()), data.size());
}

std::string make_hs256_token(const std::string& secret, const std::string& payload) {
    const std::string header = R"({"alg":"HS256","typ":"JWT"})";
    const std::string signed_part = base64url_encode(header) + "." + base64url_encode(payload);

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    HMAC(EVP_sha256(),
         secret.data(),
         static_cast<int>(secret.size()),
         reinterpret_cast<const unsigned char*>(signed_part.data()),
         signed_part.size(),
         digest,
         &digest_len);
    return signed_part + "." + base64url_encode(digest, digest_len);
}

} // namespace

int main() {
    ServerConfig::Auth auth_config;
    auth_config.enabled = true;
    auth_config.api_keys = {"api-key"};
    auth_config.jwt_algorithm = "HS256";
    auth_config.jwt_hmac_secret = "secret";
    auth_config.issuer = "issuer";
    auth_config.audience = "audience";

    Authenticator authenticator(auth_config);

    HttpRequest api_key_request;
    api_key_request.headers["x-api-key"] = "api-key";
    assert(authenticator.authorize(api_key_request, true).status_code == 200);

    const auto exp = static_cast<long long>(std::time(nullptr)) + 3600;
    const std::string payload =
        "{\"iss\":\"issuer\",\"aud\":\"audience\",\"exp\":" + std::to_string(exp) +
        ",\"scope\":\"users:read users:write\"}";
    const std::string token = make_hs256_token("secret", payload);

    HttpRequest scoped_request;
    scoped_request.headers["authorization"] = "Bearer " + token;
    assert(authenticator.authorize(scoped_request, true, {"users:read"}).status_code == 200);
    assert(authenticator.authorize(scoped_request, true, {"billing:read"}).status_code == 403);

    HttpRequest missing_auth;
    assert(authenticator.authorize(missing_auth, true).status_code == 401);

    std::cout << "test_auth passed\n";
    return 0;
}
