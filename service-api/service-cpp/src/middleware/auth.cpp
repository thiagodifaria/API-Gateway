#include "middleware/auth.hpp"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <fstream>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

using json = nlohmann::json;

namespace https_server::middleware {

Authenticator::Authenticator(ServerConfig::Auth config) : config_(std::move(config)) {
    std::transform(config_.jwt_algorithm.begin(),
                   config_.jwt_algorithm.end(),
                   config_.jwt_algorithm.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
}

http::HttpResponse Authenticator::authorize(const http::HttpRequest& request,
                                            bool required,
                                            const std::vector<std::string>& required_scopes) const {
    http::HttpResponse ok;
    if (!required && !config_.enabled) {
        return ok;
    }

    bool authorized = validate_api_key(request);
    bool token_seen = false;
    JwtValidation jwt_result;
    if (!authorized) {
        const auto auth = request.headers.find("authorization");
        if (auth != request.headers.end() && auth->second.find("Bearer ") == 0) {
            token_seen = true;
            jwt_result = validate_jwt(auth->second.substr(7));
            authorized = jwt_result.valid;
        }
    }

    if (authorized) {
        if (!required_scopes.empty()) {
            if (!token_seen) {
                http::HttpResponse forbidden;
                forbidden.status_code = 403;
                forbidden.status_text = "Forbidden";
                forbidden.headers["Content-Type"] = "application/json; charset=utf-8";
                forbidden.body = "{\"error\":\"forbidden\",\"message\":\"Route requires scoped JWT authorization\"}";
                return forbidden;
            }
            if (!has_required_scopes(jwt_result.scopes, required_scopes)) {
                http::HttpResponse forbidden;
                forbidden.status_code = 403;
                forbidden.status_text = "Forbidden";
                forbidden.headers["Content-Type"] = "application/json; charset=utf-8";
                forbidden.body = "{\"error\":\"insufficient_scope\",\"message\":\"JWT does not include required scopes\"}";
                return forbidden;
            }
        }
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
    std::size_t padding = 0;
    while (input.size() % 4 != 0) {
        input.push_back('=');
        padding++;
    }
    std::string output((input.size() * 3) / 4 + 4, '\0');
    const int written = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(&output[0]),
                                        reinterpret_cast<const unsigned char*>(input.data()),
                                        static_cast<int>(input.size()));
    if (written < 0) {
        return {};
    }
    output.resize(static_cast<std::size_t>(written) - padding);
    return output;
}

std::string Authenticator::base64url_encode(const unsigned char* data, std::size_t size) {
    std::string encoded(((size + 2) / 3) * 4, '\0');
    const int encoded_len = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(&encoded[0]),
                                            data,
                                            static_cast<int>(size));
    encoded.resize(static_cast<std::size_t>(encoded_len));
    std::replace(encoded.begin(), encoded.end(), '+', '-');
    std::replace(encoded.begin(), encoded.end(), '/', '_');
    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }
    return encoded;
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

std::vector<std::string> Authenticator::extract_scopes(const json& payload) {
    std::vector<std::string> scopes;
    const auto append_words = [&scopes](const std::string& value) {
        std::string current;
        for (const char ch : value) {
            if (ch == ' ' || ch == ',') {
                if (!current.empty()) {
                    scopes.push_back(current);
                    current.clear();
                }
            } else {
                current.push_back(ch);
            }
        }
        if (!current.empty()) {
            scopes.push_back(current);
        }
    };

    for (const std::string key : {"scope", "scp", "roles"}) {
        if (!payload.contains(key)) {
            continue;
        }
        if (payload[key].is_string()) {
            append_words(payload[key].get<std::string>());
        } else if (payload[key].is_array()) {
            for (const auto& item : payload[key]) {
                if (item.is_string()) {
                    scopes.push_back(item.get<std::string>());
                }
            }
        }
    }
    return scopes;
}

Authenticator::JwtValidation Authenticator::validate_jwt(const std::string& token) const {
    const auto first = token.find('.');
    const auto second = first == std::string::npos ? std::string::npos : token.find('.', first + 1);
    if (first == std::string::npos || second == std::string::npos) {
        return {};
    }

    const std::string encoded_header = token.substr(0, first);
    const std::string encoded_payload = token.substr(first + 1, second - first - 1);
    const std::string signed_part = token.substr(0, second);
    const std::string signature = token.substr(second + 1);

    try {
        const auto header = json::parse(base64url_decode(encoded_header));
        const std::string alg = header.value("alg", "");
        if (alg != config_.jwt_algorithm) {
            return {};
        }

        bool signature_ok = false;
        if (config_.jwt_algorithm == "HS256") {
            if (config_.jwt_hmac_secret.empty()) {
                return {};
            }
            unsigned char digest[EVP_MAX_MD_SIZE];
            unsigned int digest_len = 0;
            HMAC(EVP_sha256(),
                 config_.jwt_hmac_secret.data(),
                 static_cast<int>(config_.jwt_hmac_secret.size()),
                 reinterpret_cast<const unsigned char*>(signed_part.data()),
                 signed_part.size(),
                 digest,
                 &digest_len);
            signature_ok = constant_time_equals(base64url_encode(digest, digest_len), signature);
        } else if (config_.jwt_algorithm == "RS256" || config_.jwt_algorithm == "ES256") {
            signature_ok = verify_asymmetric_jwt(signed_part, signature);
        }
        if (!signature_ok) {
            return {};
        }

        const auto payload = json::parse(base64url_decode(encoded_payload));
        const auto now = static_cast<long long>(std::time(nullptr));
        if (payload.contains("exp") && payload["exp"].get<long long>() < now) return {};
        if (payload.contains("nbf") && payload["nbf"].get<long long>() > now) return {};
        if (!config_.issuer.empty() && (!payload.contains("iss") || payload["iss"].get<std::string>() != config_.issuer)) return {};
        if (!config_.audience.empty() && (!payload.contains("aud") || payload["aud"].get<std::string>() != config_.audience)) return {};
        return {true, extract_scopes(payload)};
    } catch (const std::exception&) {
        return {};
    }
}

bool Authenticator::verify_asymmetric_jwt(const std::string& signed_part, const std::string& signature) const {
    if (config_.jwt_public_key_file.empty()) {
        return false;
    }

    BIO* bio = BIO_new_file(config_.jwt_public_key_file.c_str(), "r");
    if (!bio) {
        return false;
    }

    EVP_PKEY* public_key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!public_key) {
        return false;
    }

    const std::string decoded_signature = base64url_decode(signature);
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(public_key);
        return false;
    }

    bool ok = false;
    if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, public_key) == 1 &&
        EVP_DigestVerifyUpdate(ctx, signed_part.data(), signed_part.size()) == 1 &&
        EVP_DigestVerifyFinal(ctx,
                              reinterpret_cast<const unsigned char*>(decoded_signature.data()),
                              decoded_signature.size()) == 1) {
        ok = true;
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(public_key);
    return ok;
}

bool Authenticator::has_required_scopes(const std::vector<std::string>& granted,
                                        const std::vector<std::string>& required) const {
    return std::all_of(required.begin(), required.end(), [&granted](const std::string& scope) {
        return std::find(granted.begin(), granted.end(), scope) != granted.end();
    });
}

} // namespace https_server::middleware
