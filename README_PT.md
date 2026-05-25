# API Gateway

![API Gateway](https://img.shields.io/badge/API%20Gateway-Alta%20Performance-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)

**API Gateway em C++17 com HTTPS, reverse proxy, balanceamento, health checks, autenticacao, rate limiting, observabilidade e aceleracoes SIMD/assembly.**

[![C++](https://img.shields.io/badge/C++-17-00599C?style=flat&logo=cplusplus&logoColor=white)](https://isocpp.org)
[![OpenSSL](https://img.shields.io/badge/OpenSSL-TLS-721412?style=flat&logo=openssl&logoColor=white)](https://openssl.org)
[![CMake](https://img.shields.io/badge/CMake-3.16+-064F8C?style=flat&logo=cmake&logoColor=white)](https://cmake.org)
[![NASM](https://img.shields.io/badge/NASM-Assembly-FF6600?style=flat)](https://nasm.us)
[![Licenca](https://img.shields.io/badge/Licenca-MIT-green.svg?style=flat)](LICENSE)

---

## Documentacao

**[README principal](README.md)**  
**[English README](README_EN.md)**  
**[Arquitetura](docs/ARQUITETURA.md)**  
**[API](docs/API.md)**

---

## O que e o API Gateway?

API Gateway e a evolucao natural do projeto que antes era um servidor HTTPS de alta performance. A primeira versao nasceu focada em TLS, arquivos estaticos, validacao JSON, benchmarks e experimentos de baixo nivel com SIMD e assembly. A versao atual preserva essa base de engenharia de sistemas e adiciona recursos reais de gateway: roteamento dinamico, reverse proxy, pools de upstream, balanceamento de carga, health checks ativos/passivos, rate limiting, autenticacao e endpoints operacionais.

O objetivo do projeto agora e atuar como uma camada de entrada para servicos internos. Ele termina HTTPS, interpreta a requisicao, aplica regras transversais, encaminha para upstreams HTTP e expoe sinais de saude e metricas. A identidade de performance do projeto anterior continua presente nos modulos de crypto, parsing, validacao, memoria, compressao e operacoes de rede.

## Capacidades atuais

- Servidor HTTPS com OpenSSL.
- Router HTTP dinamico com rotas exatas, wildcard e parametros como `/users/:id`.
- Parser HTTP dedicado com limites configuraveis, headers case-insensitive, body por `Content-Length`, separacao de path/query e query params.
- Pipeline de middlewares para aplicar comportamento transversal.
- Arquivos estaticos servidos a partir de `client-web/`.
- Cache HTTP para estaticos com `ETag`, `Last-Modified`, `Cache-Control` e `304 Not Modified`.
- Reverse proxy HTTP para servicos internos.
- Headers de proxy: `X-Forwarded-For`, `X-Forwarded-Proto`, `X-Real-IP` e `Via`.
- Remocao de headers hop-by-hop antes do encaminhamento.
- Pools de upstream com selecao `round_robin` e `least_connections`.
- Health checks HTTP ativos e rastreamento passivo de falhas.
- Rate limiting por Token Bucket com resposta `429 Too Many Requests`.
- Headers `RateLimit-Limit`, `RateLimit-Remaining`, `RateLimit-Reset` e `Retry-After`.
- Autenticacao por API key e validacao JWT HMAC-SHA256.
- Endpoints operacionais: `/gateway/health`, `/gateway/ready` e `/gateway/metrics`.
- Suporte WebSocket RFC 6455 em modo echo sobre TLS, com handshake, frames texto/binario, ping/pong e close.
- Modulos SIMD/assembly para crypto, busca de fim de header HTTP, validacao, memoria, compressao e operacoes de rede.
- Benchmarks via endpoint HTTP e binarios de benchmark gerados pelo CMake.

## Estrutura do repositorio

```text
client-web/                   frontend tecnico estatico
docs/                         documentacao oficial
  ARQUITETURA.md              arquitetura e detalhes de implementacao
  API.md                      endpoints, configuracao e contratos HTTP
scripts/                      scripts oficiais de build e benchmark
service-api/service-cpp/      runtime C++, testes e projeto CMake
service-api/service-assembly/ rotinas assembly de aceleracao
```

## Build

Pre-requisitos:

- Compilador com suporte a C++17.
- CMake 3.16+.
- OpenSSL.
- NASM.
- Ninja opcional; o script usa automaticamente quando estiver disponivel.

Comando principal:

```bash
./scripts/build.sh Release
```

No Windows, execute pelo Git Bash:

```bash
"C:/Program Files/Git/bin/bash.exe" ./scripts/build.sh Release
```

## Execucao

```bash
./build/api_gateway
# Windows/Git Bash: ./build/api_gateway.exe
```

Configuracao padrao:

```text
service-api/service-cpp/config/config.json
```

Tambem e possivel apontar para outro arquivo de configuracao usando a variavel `HTTPS_SERVER_CONFIG`.

## Uso rapido

Health check:

```bash
curl -k https://localhost:8443/gateway/health
```

Ready check com estado dos upstreams:

```bash
curl -k https://localhost:8443/gateway/ready
```

Metricas em formato Prometheus:

```bash
curl -k https://localhost:8443/gateway/metrics
```

Metricas em JSON:

```bash
curl -k -H "Accept: application/json" https://localhost:8443/gateway/metrics
```

Echo JSON com validacao e operacoes auxiliares:

```bash
curl -k -X POST "https://localhost:8443/api/echo" \
  -H "Content-Type: application/json" \
  -d '{"message":"ola","encode_data":"gateway"}'
```

Benchmarks via HTTP:

```bash
curl -k https://localhost:8443/api/benchmark
```

## Exemplo de configuracao de gateway

```json
{
  "upstreams": {
    "users_service": {
      "targets": [
        "http://127.0.0.1:9001",
        "http://127.0.0.1:9002"
      ],
      "health_path": "/gateway/health",
      "health_interval_ms": 5000,
      "health_timeout_ms": 1000,
      "unhealthy_threshold": 2,
      "healthy_threshold": 1
    }
  },
  "proxy_routes": [
    {
      "method": "GET",
      "path": "/api/v1/users/*",
      "upstream": "users_service",
      "strip_prefix": "/api/v1/users",
      "load_balancer": "round_robin",
      "require_auth": true
    }
  ],
  "rate_limit": {
    "enabled": true,
    "capacity": 120,
    "refill_per_second": 20,
    "key": "ip"
  },
  "auth": {
    "enabled": true,
    "api_key_header": "X-API-Key",
    "api_keys": ["local-dev-key"],
    "jwt_hmac_secret": "local-secret"
  }
}
```

## Testes e benchmarks

Depois do build, os binarios de teste ficam em `build/`:

```bash
./build/unit_test_http_parser
./build/unit_test_router
./build/unit_test_aes
./build/unit_test_sha256
./build/unit_test_p256
./build/test_fast_memory
```

Benchmarks:

```bash
./scripts/run_benchmarks.sh
```

## Observacoes tecnicas

- O reverse proxy atual encaminha para upstreams HTTP.
- O suporte WebSocket implementado no gateway e echo local; ele nao faz proxy WebSocket para upstream.
- A autenticacao JWT implementada usa HMAC-SHA256.
- O campo `required_scopes` existe na configuracao de rota, mas a validacao de escopo ainda nao e aplicada pelo runtime atual.
- O campo `observability.json_logs` existe na configuracao, mas o logger atual permanece em texto.

## Licenca

Distribuido sob a licenca MIT. Veja [LICENSE](LICENSE).

---

## Autor

**Thiago Di Faria**  
Email: [thiagodifaria@gmail.com](mailto:thiagodifaria@gmail.com)  
GitHub: [@thiagodifaria](https://github.com/thiagodifaria)

Construido como showcase de engenharia de sistemas em C++17, preservando a base de performance do antigo servidor HTTPS e evoluindo para uma API Gateway.
