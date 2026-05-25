# API Gateway

![API Gateway](https://img.shields.io/badge/API%20Gateway-High%20Performance-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)

**High-performance C++17 API Gateway with HTTPS, reverse proxy, load balancing, health checks, authentication, rate limiting, observability and SIMD/assembly acceleration.**

[![C++](https://img.shields.io/badge/C++-17-00599C?style=flat&logo=cplusplus&logoColor=white)](https://isocpp.org)
[![OpenSSL](https://img.shields.io/badge/OpenSSL-TLS-721412?style=flat&logo=openssl&logoColor=white)](https://openssl.org)
[![CMake](https://img.shields.io/badge/CMake-3.16+-064F8C?style=flat&logo=cmake&logoColor=white)](https://cmake.org)
[![NASM](https://img.shields.io/badge/NASM-Assembly-FF6600?style=flat)](https://nasm.us)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat)](LICENSE)

---

## Documentation

**[Read in English](README_EN.md)**  
**[Leia em Portugues](README_PT.md)**  
**[Architecture](docs/ARQUITETURA.md)**  
**[API Reference](docs/API.md)**

---

## What Is API Gateway?

API Gateway is the natural evolution of the original HTTPS server project. The first version focused on TLS, static files, JSON validation, benchmarks and low-level performance experiments. The current version keeps that systems-engineering base and adds gateway behavior on top of it: dynamic routing, reverse proxying, upstream pools, load balancing, active/passive health checks, Token Bucket rate limiting, API key/JWT authentication and operational endpoints.

The result is a compact C++ gateway that can terminate HTTPS, inspect and route requests, protect upstream services and expose runtime visibility while still preserving the original SIMD/assembly performance identity.

## Current Capabilities

- HTTPS server built with OpenSSL.
- Dynamic HTTP router with exact routes, wildcard routes and path params such as `/users/:id`.
- Dedicated HTTP parser with request limits, case-insensitive headers, body handling through `Content-Length`, path/query split and query params.
- Middleware pipeline for cross-cutting concerns.
- Static file serving from `client-web/` with `ETag`, `Last-Modified`, `Cache-Control` and `304 Not Modified`.
- HTTP reverse proxy for internal upstreams.
- `X-Forwarded-For`, `X-Forwarded-Proto`, `X-Real-IP` and `Via` proxy headers.
- Hop-by-hop header filtering before forwarding requests.
- Upstream pools with `round_robin` and `least_connections` selection.
- Active HTTP health checks and passive failure tracking.
- Token Bucket rate limiting with `429 Too Many Requests` and `RateLimit-*` headers.
- API key authentication and HMAC-SHA256 JWT validation.
- Gateway endpoints: `/gateway/health`, `/gateway/ready` and `/gateway/metrics`.
- WebSocket RFC 6455 echo support over TLS, including handshake, text/binary frames, ping/pong and close.
- SIMD/assembly modules for crypto, HTTP scanning, validation, memory, compression and network operations.
- Benchmark endpoints and command-line benchmark targets for crypto/performance checks.

## Repository Layout

```text
client-web/                   static technical frontend
docs/                         official documentation
  ARQUITETURA.md              architecture and implementation notes
  API.md                      endpoints, config and HTTP contracts
scripts/                      build and benchmark entrypoints
service-api/service-cpp/      C++ gateway runtime, tests and CMake project
service-api/service-assembly/ assembly acceleration routines
```

## Quick Start

Prerequisites:

- C++17 compiler.
- CMake 3.16+.
- OpenSSL.
- NASM.
- Ninja is optional, but used automatically when available.

Build:

```bash
./scripts/build.sh Release
```

Windows with Git Bash:

```bash
"C:/Program Files/Git/bin/bash.exe" ./scripts/build.sh Release
```

Run:

```bash
./build/api_gateway
# Windows/Git Bash: ./build/api_gateway.exe
```

Default configuration:

```text
service-api/service-cpp/config/config.json
```

## Basic Checks

```bash
curl -k https://localhost:8443/gateway/health
curl -k https://localhost:8443/gateway/ready
curl -k https://localhost:8443/gateway/metrics
curl -k https://localhost:8443/api/benchmark
```

POST JSON with SIMD validation and network encoding helpers:

```bash
curl -k -X POST "https://localhost:8443/api/echo" \
  -H "Content-Type: application/json" \
  -d '{"message":"hello","encode_data":"gateway"}'
```

## Build Outputs

The main executable is `api_gateway`. Test and benchmark binaries are generated in `build/` as separate targets, including parser/router tests and crypto benchmarks.

## License

Distributed under the MIT License. See [LICENSE](LICENSE).

---

## Author

**Thiago Di Faria**  
Email: [thiagodifaria@gmail.com](mailto:thiagodifaria@gmail.com)  
GitHub: [@thiagodifaria](https://github.com/thiagodifaria)

Built as a systems-engineering showcase in C++17, preserving the performance-focused roots of the original HTTPS server while evolving it into an API Gateway.
