#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#endif

#include "aes.hpp"
#include "sha256.hpp"
#include "crypto_advanced.hpp"
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <vector>
#include <openssl/aes.h>
#include <cstring>
#include <algorithm>

#pragma warning(disable: 4996)

extern "C" {
    void p256_mul_mont(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
    void p256_sqr_mont(std::uint64_t res[4], const std::uint64_t a[4]);
    void p256_add_mod(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
    void p256_sub_mod(std::uint64_t res[4], const std::uint64_t a[4], const std::uint64_t b[4]);
}

struct prov_aes_ctx {
    std::vector<std::uint8_t> round_keys;
};

struct prov_sha256_ctx {
    std::vector<std::uint32_t> hash;
    std::vector<std::uint8_t> buffer;
    std::uint64_t total_len;
    size_t buffer_len;
};

struct prov_p256_ctx {
    std::vector<std::uint64_t> private_key;
    std::vector<std::uint64_t> public_key;
    bool has_private;
    bool has_public;
};

struct prov_chacha20_ctx {
    std::vector<std::uint8_t> key;
    std::vector<std::uint8_t> nonce;
    std::uint32_t counter;
};

struct prov_blake3_ctx {
    std::vector<std::uint8_t> state;
    std::vector<std::uint8_t> buffer;
    std::uint64_t total_len;
    size_t buffer_len;
};

struct prov_x25519_ctx {
    std::vector<std::uint8_t> private_key;
    std::vector<std::uint8_t> public_key;
    bool has_private;
    bool has_public;
};

static int aes128_cipher(void *vctx, unsigned char *out, size_t *outl,
                         size_t outsize, const unsigned char *in, size_t inl) {
    const auto *ctx = static_cast<prov_aes_ctx*>(vctx);
    
    if (inl != 16 || outsize < 16) {
        return 0;
    }

    aes_encrypt_block_asm(in, out, ctx->round_keys.data());
    *outl = 16;
    return 1;
}

static void *aes_newctx(void*) { 
    return new prov_aes_ctx(); 
}

static void aes_freectx(void *vctx) { 
    delete static_cast<prov_aes_ctx*>(vctx); 
}

static int aes_einit(void *vctx, const unsigned char *key, size_t keylen,
                     const unsigned char*, size_t, const OSSL_PARAM*) {
    auto *ctx = static_cast<prov_aes_ctx*>(vctx);
    if (key && keylen == 16) {
        ctx->round_keys.resize(176);
        AES_set_encrypt_key(key, 128, reinterpret_cast<AES_KEY*>(ctx->round_keys.data()));
    }
    return 1;
}

static void *sha256_newctx(void*) {
    auto *ctx = new prov_sha256_ctx();
    ctx->hash.resize(8);
    ctx->buffer.resize(64);
    ctx->hash[0] = 0x6a09e667;
    ctx->hash[1] = 0xbb67ae85;
    ctx->hash[2] = 0x3c6ef372;
    ctx->hash[3] = 0xa54ff53a;
    ctx->hash[4] = 0x510e527f;
    ctx->hash[5] = 0x9b05688c;
    ctx->hash[6] = 0x1f83d9ab;
    ctx->hash[7] = 0x5be0cd19;
    ctx->total_len = 0;
    ctx->buffer_len = 0;
    return ctx;
}

static void sha256_freectx(void *vctx) {
    delete static_cast<prov_sha256_ctx*>(vctx);
}

static int sha256_digest_init(void *vctx, const OSSL_PARAM*) {
    auto *ctx = static_cast<prov_sha256_ctx*>(vctx);
    ctx->hash[0] = 0x6a09e667;
    ctx->hash[1] = 0xbb67ae85;
    ctx->hash[2] = 0x3c6ef372;
    ctx->hash[3] = 0xa54ff53a;
    ctx->hash[4] = 0x510e527f;
    ctx->hash[5] = 0x9b05688c;
    ctx->hash[6] = 0x1f83d9ab;
    ctx->hash[7] = 0x5be0cd19;
    ctx->total_len = 0;
    ctx->buffer_len = 0;
    return 1;
}

static int sha256_digest_update(void *vctx, const unsigned char *in, size_t inl) {
    auto *ctx = static_cast<prov_sha256_ctx*>(vctx);
    
    ctx->total_len += inl;
    
    while (inl > 0) {
        const size_t copy_len = (std::min)(inl, 64 - ctx->buffer_len);
        std::memcpy(ctx->buffer.data() + ctx->buffer_len, in, copy_len);
        ctx->buffer_len += copy_len;
        in += copy_len;
        inl -= copy_len;
        
        if (ctx->buffer_len == 64) {
            sha256_block_asm(ctx->buffer.data(), ctx->hash.data());
            ctx->buffer_len = 0;
        }
    }
    
    return 1;
}

static int sha256_digest_final(void *vctx, unsigned char *out, size_t *outl, size_t outsize) {
    auto *ctx = static_cast<prov_sha256_ctx*>(vctx);
    
    if (outsize < 32) {
        return 0;
    }
    
    ctx->buffer[ctx->buffer_len++] = 0x80;
    
    if (ctx->buffer_len > 56) {
        while (ctx->buffer_len < 64) {
            ctx->buffer[ctx->buffer_len++] = 0x00;
        }
        sha256_block_asm(ctx->buffer.data(), ctx->hash.data());
        ctx->buffer_len = 0;
    }
    
    while (ctx->buffer_len < 56) {
        ctx->buffer[ctx->buffer_len++] = 0x00;
    }
    
    const std::uint64_t bit_len = ctx->total_len * 8;
    for (int i = 7; i >= 0; --i) {
        ctx->buffer[56 + i] = static_cast<std::uint8_t>(bit_len >> (8 * (7 - i)));
    }
    
    sha256_block_asm(ctx->buffer.data(), ctx->hash.data());
    
    for (int i = 0; i < 8; ++i) {
        const std::uint32_t h = ctx->hash[i];
        out[i * 4 + 0] = static_cast<std::uint8_t>(h >> 24);
        out[i * 4 + 1] = static_cast<std::uint8_t>(h >> 16);
        out[i * 4 + 2] = static_cast<std::uint8_t>(h >> 8);
        out[i * 4 + 3] = static_cast<std::uint8_t>(h);
    }
    
    *outl = 32;
    return 1;
}

static void *chacha20_newctx(void*) {
    auto *ctx = new prov_chacha20_ctx();
    ctx->key.resize(32);
    ctx->nonce.resize(12);
    ctx->counter = 0;
    return ctx;
}

static void chacha20_freectx(void *vctx) {
    delete static_cast<prov_chacha20_ctx*>(vctx);
}

static int chacha20_einit(void *vctx, const unsigned char *key, size_t keylen,
                          const unsigned char *nonce, size_t noncelen, const OSSL_PARAM*) {
    auto *ctx = static_cast<prov_chacha20_ctx*>(vctx);
    if (key && keylen == 32) {
        std::memcpy(ctx->key.data(), key, 32);
    }
    if (nonce && noncelen == 12) {
        std::memcpy(ctx->nonce.data(), nonce, 12);
    }
    ctx->counter = 0;
    return 1;
}

static int chacha20_cipher(void *vctx, unsigned char *out, size_t *outl,
                           size_t outsize, const unsigned char *in, size_t inl) {
    auto *ctx = static_cast<prov_chacha20_ctx*>(vctx);
    
    if (inl > outsize) {
        return 0;
    }
    
    for (size_t i = 0; i < inl; i += 64) {
        const size_t chunk_size = (std::min)(static_cast<size_t>(64), inl - i);
        chacha20_encrypt_block_asm(in + i, out + i, ctx->key.data(), ctx->nonce.data(), ctx->counter);
        ctx->counter++;
    }
    
    *outl = inl;
    return 1;
}

static void *blake3_newctx(void*) {
    auto *ctx = new prov_blake3_ctx();
    ctx->state.resize(32);
    ctx->buffer.resize(64);
    ctx->total_len = 0;
    ctx->buffer_len = 0;
    return ctx;
}

static void blake3_freectx(void *vctx) {
    delete static_cast<prov_blake3_ctx*>(vctx);
}

static int blake3_digest_init(void *vctx, const OSSL_PARAM*) {
    auto *ctx = static_cast<prov_blake3_ctx*>(vctx);
    std::fill(ctx->state.begin(), ctx->state.end(), 0);
    ctx->total_len = 0;
    ctx->buffer_len = 0;
    return 1;
}

static int blake3_digest_update(void *vctx, const unsigned char *in, size_t inl) {
    auto *ctx = static_cast<prov_blake3_ctx*>(vctx);
    
    ctx->total_len += inl;
    
    while (inl > 0) {
        const size_t copy_len = (std::min)(inl, 64 - ctx->buffer_len);
        std::memcpy(ctx->buffer.data() + ctx->buffer_len, in, copy_len);
        ctx->buffer_len += copy_len;
        in += copy_len;
        inl -= copy_len;
        
        if (ctx->buffer_len == 64) {
            blake3_hash_chunk_asm(ctx->buffer.data(), 64, ctx->state.data());
            ctx->buffer_len = 0;
        }
    }
    
    return 1;
}

static int blake3_digest_final(void *vctx, unsigned char *out, size_t *outl, size_t outsize) {
    auto *ctx = static_cast<prov_blake3_ctx*>(vctx);
    
    if (outsize < 32) {
        return 0;
    }
    
    if (ctx->buffer_len > 0) {
        blake3_hash_chunk_asm(ctx->buffer.data(), ctx->buffer_len, ctx->state.data());
    }
    
    std::memcpy(out, ctx->state.data(), 32);
    *outl = 32;
    return 1;
}

static void *p256_newctx(void*) {
    auto *ctx = new prov_p256_ctx();
    ctx->private_key.resize(4, 0);
    ctx->public_key.resize(8, 0);
    ctx->has_private = false;
    ctx->has_public = false;
    return ctx;
}

static void p256_freectx(void *vctx) {
    delete static_cast<prov_p256_ctx*>(vctx);
}

static int p256_keygen(void *vctx, unsigned char *pub, size_t *publen, 
                       unsigned char *priv, size_t *privlen) {
    auto *ctx = static_cast<prov_p256_ctx*>(vctx);
    
    if (priv && *privlen >= 32) {
        for (size_t i = 0; i < 4; ++i) {
            ctx->private_key[i] = static_cast<std::uint64_t>(rand()) << 32 | rand();
        }
        std::memcpy(priv, ctx->private_key.data(), 32);
        *privlen = 32;
        ctx->has_private = true;
    }
    
    if (pub && *publen >= 64) {
        for (size_t i = 0; i < 8; ++i) {
            ctx->public_key[i] = ctx->private_key[i % 4] ^ (i * 0x123456789abcdef);
        }
        std::memcpy(pub, ctx->public_key.data(), 64);
        *publen = 64;
        ctx->has_public = true;
    }
    
    return 1;
}

static int p256_derive(void *vctx, unsigned char *secret, size_t *secretlen,
                       const unsigned char *peer_pub, size_t peer_publen) {
    auto *ctx = static_cast<prov_p256_ctx*>(vctx);
    
    if (!ctx->has_private || peer_publen != 64 || *secretlen < 32) {
        return 0;
    }
    
    std::vector<std::uint64_t> peer_point(8);
    std::memcpy(peer_point.data(), peer_pub, 64);
    
    std::vector<std::uint64_t> shared_secret(4);
    for (size_t i = 0; i < 4; ++i) {
        p256_mul_mont(&shared_secret[i], &ctx->private_key[i], &peer_point[i]);
    }
    
    std::memcpy(secret, shared_secret.data(), 32);
    *secretlen = 32;
    
    return 1;
}

static void *x25519_newctx(void*) {
    auto *ctx = new prov_x25519_ctx();
    ctx->private_key.resize(32);
    ctx->public_key.resize(32);
    ctx->has_private = false;
    ctx->has_public = false;
    return ctx;
}

static void x25519_freectx(void *vctx) {
    delete static_cast<prov_x25519_ctx*>(vctx);
}

static int x25519_keygen(void *vctx, unsigned char *pub, size_t *publen,
                         unsigned char *priv, size_t *privlen) {
    auto *ctx = static_cast<prov_x25519_ctx*>(vctx);
    
    if (priv && *privlen >= 32) {
        for (size_t i = 0; i < 32; ++i) {
            ctx->private_key[i] = static_cast<std::uint8_t>(rand());
        }
        std::memcpy(priv, ctx->private_key.data(), 32);
        *privlen = 32;
        ctx->has_private = true;
    }
    
    if (pub && *publen >= 32) {
        x25519_scalar_mult_asm(ctx->private_key.data(), nullptr, ctx->public_key.data());
        std::memcpy(pub, ctx->public_key.data(), 32);
        *publen = 32;
        ctx->has_public = true;
    }
    
    return 1;
}

static int x25519_derive(void *vctx, unsigned char *secret, size_t *secretlen,
                         const unsigned char *peer_pub, size_t peer_publen) {
    auto *ctx = static_cast<prov_x25519_ctx*>(vctx);
    
    if (!ctx->has_private || peer_publen != 32 || *secretlen < 32) {
        return 0;
    }
    
    x25519_scalar_mult_asm(ctx->private_key.data(), peer_pub, secret);
    *secretlen = 32;
    return 1;
}

static const OSSL_DISPATCH aes128_ecb_functions[] = {
    { OSSL_FUNC_CIPHER_NEWCTX, reinterpret_cast<void (*)(void)>(aes_newctx) },
    { OSSL_FUNC_CIPHER_FREECTX, reinterpret_cast<void (*)(void)>(aes_freectx) },
    { OSSL_FUNC_CIPHER_ENCRYPT_INIT, reinterpret_cast<void (*)(void)>(aes_einit) },
    { OSSL_FUNC_CIPHER_CIPHER, reinterpret_cast<void (*)(void)>(aes128_cipher) },
    { 0, nullptr }
};

static const OSSL_DISPATCH sha256_functions[] = {
    { OSSL_FUNC_DIGEST_NEWCTX, reinterpret_cast<void (*)(void)>(sha256_newctx) },
    { OSSL_FUNC_DIGEST_FREECTX, reinterpret_cast<void (*)(void)>(sha256_freectx) },
    { OSSL_FUNC_DIGEST_INIT, reinterpret_cast<void (*)(void)>(sha256_digest_init) },
    { OSSL_FUNC_DIGEST_UPDATE, reinterpret_cast<void (*)(void)>(sha256_digest_update) },
    { OSSL_FUNC_DIGEST_FINAL, reinterpret_cast<void (*)(void)>(sha256_digest_final) },
    { 0, nullptr }
};

static const OSSL_DISPATCH chacha20_functions[] = {
    { OSSL_FUNC_CIPHER_NEWCTX, reinterpret_cast<void (*)(void)>(chacha20_newctx) },
    { OSSL_FUNC_CIPHER_FREECTX, reinterpret_cast<void (*)(void)>(chacha20_freectx) },
    { OSSL_FUNC_CIPHER_ENCRYPT_INIT, reinterpret_cast<void (*)(void)>(chacha20_einit) },
    { OSSL_FUNC_CIPHER_CIPHER, reinterpret_cast<void (*)(void)>(chacha20_cipher) },
    { 0, nullptr }
};

static const OSSL_DISPATCH blake3_functions[] = {
    { OSSL_FUNC_DIGEST_NEWCTX, reinterpret_cast<void (*)(void)>(blake3_newctx) },
    { OSSL_FUNC_DIGEST_FREECTX, reinterpret_cast<void (*)(void)>(blake3_freectx) },
    { OSSL_FUNC_DIGEST_INIT, reinterpret_cast<void (*)(void)>(blake3_digest_init) },
    { OSSL_FUNC_DIGEST_UPDATE, reinterpret_cast<void (*)(void)>(blake3_digest_update) },
    { OSSL_FUNC_DIGEST_FINAL, reinterpret_cast<void (*)(void)>(blake3_digest_final) },
    { 0, nullptr }
};

static const OSSL_DISPATCH p256_keyexch_functions[] = {
    { OSSL_FUNC_KEYEXCH_NEWCTX, reinterpret_cast<void (*)(void)>(p256_newctx) },
    { OSSL_FUNC_KEYEXCH_FREECTX, reinterpret_cast<void (*)(void)>(p256_freectx) },
    { OSSL_FUNC_KEYEXCH_DERIVE, reinterpret_cast<void (*)(void)>(p256_derive) },
    { 0, nullptr }
};

static const OSSL_DISPATCH p256_keymgmt_functions[] = {
    { OSSL_FUNC_KEYMGMT_NEW, reinterpret_cast<void (*)(void)>(p256_newctx) },
    { OSSL_FUNC_KEYMGMT_FREE, reinterpret_cast<void (*)(void)>(p256_freectx) },
    { OSSL_FUNC_KEYMGMT_GEN, reinterpret_cast<void (*)(void)>(p256_keygen) },
    { 0, nullptr }
};

static const OSSL_DISPATCH x25519_keyexch_functions[] = {
    { OSSL_FUNC_KEYEXCH_NEWCTX, reinterpret_cast<void (*)(void)>(x25519_newctx) },
    { OSSL_FUNC_KEYEXCH_FREECTX, reinterpret_cast<void (*)(void)>(x25519_freectx) },
    { OSSL_FUNC_KEYEXCH_DERIVE, reinterpret_cast<void (*)(void)>(x25519_derive) },
    { 0, nullptr }
};

static const OSSL_DISPATCH x25519_keymgmt_functions[] = {
    { OSSL_FUNC_KEYMGMT_NEW, reinterpret_cast<void (*)(void)>(x25519_newctx) },
    { OSSL_FUNC_KEYMGMT_FREE, reinterpret_cast<void (*)(void)>(x25519_freectx) },
    { OSSL_FUNC_KEYMGMT_GEN, reinterpret_cast<void (*)(void)>(x25519_keygen) },
    { 0, nullptr }
};

static const OSSL_ALGORITHM algorithms[] = {
    { "AES-128-ECB", "provider=aes-ni", aes128_ecb_functions },
    { "SHA256", "provider=sha256-avx", sha256_functions },
    { "ChaCha20", "provider=chacha20-avx2", chacha20_functions },
    { "BLAKE3", "provider=blake3-avx2", blake3_functions },
    { "ECDH", "provider=p256-avx2", p256_keyexch_functions },
    { "EC", "provider=p256-avx2", p256_keymgmt_functions },
    { "X25519", "provider=x25519-avx2", x25519_keyexch_functions },
    { "X25519", "provider=x25519-avx2", x25519_keymgmt_functions },
    { nullptr, nullptr, nullptr }
};

static const OSSL_ALGORITHM *provider_query(void*, int operation_id, int *no_cache) {
    *no_cache = 0;
    
    switch (operation_id) {
        case OSSL_OP_CIPHER:
            return &algorithms[0];
        case OSSL_OP_DIGEST:
            return &algorithms[1];
        case OSSL_OP_KEYEXCH:
            return &algorithms[4];
        case OSSL_OP_KEYMGMT:
            return &algorithms[5];
        default:
            return nullptr;
    }
}

static const OSSL_DISPATCH provider_functions[] = {
    { OSSL_FUNC_PROVIDER_QUERY_OPERATION, reinterpret_cast<void (*)(void)>(provider_query) },
    { 0, nullptr }
};

extern "C" OSSL_provider_init_fn OSSL_provider_init;
int OSSL_provider_init(const OSSL_CORE_HANDLE*, const OSSL_DISPATCH*,
                       const OSSL_DISPATCH **out, void**) {
    *out = provider_functions;
    return 1;
}