# API

Este documento descreve os endpoints, contratos HTTP e formato de configuracao do API Gateway.

## Base URL local

```text
https://localhost:8443
```

O certificado local do projeto e autoassinado. Em testes com `curl`, use `-k`.

## Endpoints locais

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/` | Frontend tecnico servido a partir de `client-web/index.html` |
| `GET` | `/about` | Pagina tecnica estatica |
| `GET` | `/bench` | Interface web de benchmarks |
| `GET` | `/style.css` | CSS estatico |
| `GET` | `/test.html` | Pagina estatica de teste |
| `GET` | `/static/*` | Arquivos estaticos com prefixo removido |
| `GET` | `/*` | Fallback para arquivos estaticos |
| `POST` | `/api/echo` | Echo JSON com validacao e operacoes auxiliares |
| `GET` | `/api/benchmark` | Benchmarks de AES, SHA-256 e P-256 |
| `GET` | `/gateway/health` | Liveness do gateway |
| `GET` | `/gateway/ready` | Estado dos upstreams |
| `GET` | `/gateway/metrics` | Metricas em Prometheus ou JSON |

`HEAD` pode reutilizar handlers `GET`; o corpo e removido da resposta.

## `GET /gateway/health`

Retorna liveness simples do processo.

Exemplo:

```bash
curl -k https://localhost:8443/gateway/health
```

Resposta:

```json
{
  "status": "ok"
}
```

## `GET /gateway/ready`

Retorna estado conhecido dos upstreams configurados.

Exemplo:

```bash
curl -k https://localhost:8443/gateway/ready
```

Resposta quando ha upstreams:

```json
{
  "users_service": [
    {
      "target": "http://127.0.0.1:9001",
      "healthy": true,
      "active_connections": 0,
      "total_requests": 10,
      "consecutive_failures": 0
    }
  ]
}
```

Quando nao ha upstreams configurados, o objeto pode vir vazio.

## `GET /gateway/metrics`

Por padrao retorna texto compativel com formato Prometheus.

```bash
curl -k https://localhost:8443/gateway/metrics
```

Exemplo:

```text
# TYPE gateway_requests_total counter
gateway_requests_total 42
gateway_rate_limit_hits_total 3
gateway_route_requests_total{route="GET /gateway/health"} 10
```

Para JSON:

```bash
curl -k -H "Accept: application/json" https://localhost:8443/gateway/metrics
```

Campos principais:

- `requests_total`
- `rate_limit_hits_total`
- `cache_hits_total`
- `cache_misses_total`
- `requests_by_route`
- `responses_by_status`
- `upstream_health`

## `POST /api/echo`

Recebe JSON, valida o corpo e devolve o payload recebido com campos adicionais. Quando `encode_data` existe, retorna tambem base64 e hex.

Request:

```bash
curl -k -X POST "https://localhost:8443/api/echo" \
  -H "Content-Type: application/json" \
  -d '{"message":"hello","encode_data":"gateway"}'
```

Resposta:

```json
{
  "message": "hello",
  "encode_data": "gateway",
  "base64_encoded": "Z2F0ZXdheQ==",
  "hex_encoded": "67617465776179",
  "received": true,
  "timestamp": 1234567890,
  "server": "API Gateway v1.0",
  "status": "success",
  "request_id": "..."
}
```

Erros:

- `400 Bad Request` quando o body esta vazio.
- `400 Bad Request` quando o JSON e invalido.
- `400 Bad Request` quando a validacao detecta UTF-8 invalido ou caracteres rejeitados.

## `GET /api/benchmark`

Executa benchmarks internos e retorna dados de AES, SHA-256 e P-256.

```bash
curl -k https://localhost:8443/api/benchmark
```

Resposta:

```json
{
  "status": "success",
  "request_id": "...",
  "timestamp": 1234567890,
  "aes_ni": {
    "throughput": "3.51 GB/s",
    "time": "0.080s",
    "blocks": 20000000
  },
  "sha256": {
    "throughput": "2.10 GB/s",
    "time": "0.120s",
    "blocks": 1000000
  },
  "p256": {
    "field_ops": 850000,
    "point_ops": 12000,
    "ecdh_est": 1660
  }
}
```

Os valores sao exemplos de formato; a performance real depende da CPU e do build.

## Arquivos estaticos e cache

Arquivos estaticos podem retornar:

- `ETag`
- `Last-Modified`
- `Cache-Control`
- `Vary`

Quando o cliente envia `If-None-Match` ou `If-Modified-Since` compativel, o gateway responde:

```text
HTTP/1.1 304 Not Modified
```

## Configuracao

Arquivo padrao:

```text
service-api/service-cpp/config/config.json
```

Arquivo alternativo:

```bash
HTTPS_SERVER_CONFIG=path/to/config.json ./build/api_gateway
```

Windows PowerShell:

```powershell
$env:HTTPS_SERVER_CONFIG = "path/to/config.json"
.\build\api_gateway.exe
```

## Campos principais de configuracao

```json
{
  "port": 8443,
  "threads": 0,
  "cert_file": "service-api/service-cpp/certs/cert.pem",
  "key_file": "service-api/service-cpp/certs/key.pem",
  "web_root": "client-web",
  "log_level": "Debug",
  "client_ca_file": "",
  "http": {
    "max_header_bytes": 65536,
    "max_body_bytes": 2097152,
    "read_timeout_ms": 5000,
    "write_timeout_ms": 5000,
    "max_uri_bytes": 8192,
    "keep_alive": false
  },
  "rate_limit": {
    "enabled": true,
    "capacity": 120,
    "refill_per_second": 20,
    "key": "ip"
  },
  "auth": {
    "enabled": false,
    "api_key_header": "X-API-Key",
    "api_keys": [],
    "jwt_algorithm": "HS256",
    "jwt_hmac_secret": "",
    "jwt_public_key_file": "",
    "issuer": "",
    "audience": ""
  },
  "observability": {
    "metrics_enabled": true,
    "json_logs": false
  },
  "websocket": {
    "enabled": true,
    "max_frame_bytes": 1048576,
    "idle_timeout_ms": 60000
  },
  "upstreams": {},
  "proxy_routes": []
}
```

## `upstreams`

Formato simples:

```json
{
  "upstreams": {
    "users_service": [
      "http://127.0.0.1:9001",
      "http://127.0.0.1:9002"
    ]
  }
}
```

Formato completo:

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
  }
}
```

Campos:

- `targets`: lista de upstreams HTTP.
- `health_path`: path usado pelo health checker.
- `health_interval_ms`: intervalo minimo entre checks por target.
- `health_timeout_ms`: timeout do health check.
- `unhealthy_threshold`: falhas consecutivas para marcar unhealthy.
- `healthy_threshold`: sucessos consecutivos para recuperar.

## `proxy_routes`

```json
{
  "proxy_routes": [
    {
      "method": "GET",
      "path": "/api/v1/users/*",
      "upstream": "users_service",
      "strip_prefix": "/api/v1/users",
      "load_balancer": "least_connections",
      "require_auth": true,
      "required_scopes": ["users:read"],
      "rate_limit": true,
      "websocket": false
    }
  ]
}
```

Campos:

- `method`: metodo HTTP registrado no router.
- `path`: padrao de rota; suporta wildcard e parametros.
- `upstream`: nome definido em `upstreams`.
- `strip_prefix`: prefixo removido antes de encaminhar.
- `load_balancer`: `round_robin` ou `least_connections`.
- `require_auth`: exige API key ou JWT valido.
- `required_scopes`: lista de escopos obrigatorios; exige JWT e retorna `403` quando o token nao contem todos os escopos.
- `rate_limit`: campo de configuracao presente; o middleware atual aplica limite global quando `rate_limit.enabled` esta ativo.
- `websocket`: campo de configuracao presente; WebSocket atual e echo local.

## Headers de proxy

O gateway adiciona:

- `Host`
- `X-Forwarded-For`
- `X-Forwarded-Proto`
- `X-Real-IP`
- `Via`

O gateway remove:

- `Connection`
- `Keep-Alive`
- `Proxy-Authenticate`
- `Proxy-Authorization`
- `TE`
- `Trailer`
- `Transfer-Encoding`
- `Upgrade`

## Rate limiting

Quando `rate_limit.enabled` esta ativo, o gateway usa Token Bucket.

Chaves suportadas:

- `ip`
- `api_key`
- `route`

Resposta em excesso:

```text
HTTP/1.1 429 Too Many Requests
RateLimit-Limit: 120
RateLimit-Remaining: 0
RateLimit-Reset: 1
Retry-After: 1
```

Body:

```json
{
  "error": "rate_limited",
  "message": "Too many requests"
}
```

## Autenticacao

Rotas com `require_auth: true` exigem API key ou JWT.

API key:

```bash
curl -k -H "X-API-Key: local-dev-key" https://localhost:8443/api/v1/users/list
```

JWT:

```bash
curl -k -H "Authorization: Bearer <token>" https://localhost:8443/api/v1/users/list
```

Algoritmos JWT suportados:

- `HS256` com `jwt_hmac_secret`.
- `RS256` com `jwt_public_key_file`.
- `ES256` com `jwt_public_key_file`.

Validacoes JWT:

- Assinatura HMAC-SHA256.
- Assinatura assimetrica por chave publica quando `jwt_algorithm` e `RS256` ou `ES256`.
- `exp`, quando presente.
- `nbf`, quando presente.
- `iss`, quando configurado.
- `aud`, quando configurado.
- Escopos em `scope`, `scp` ou `roles`, quando a rota configura `required_scopes`.

Falha:

```text
HTTP/1.1 401 Unauthorized
WWW-Authenticate: Bearer
```

Escopo insuficiente:

```text
HTTP/1.1 403 Forbidden
```

## Logs JSON

Quando `observability.json_logs` e `true`, logs sao emitidos como JSON por linha:

```json
{
  "timestamp": "2026-05-25 14:30:00",
  "level": "Info",
  "log_id": "0e1f2a3b-4c5d-6e7f-8899-aabbccddeeff",
  "message": "Access: GET /gateway/health -> 200"
}
```

## WebSocket

O gateway aceita upgrade RFC 6455 quando `websocket.enabled` esta ativo.

Headers esperados:

```text
Connection: Upgrade
Upgrade: websocket
Sec-WebSocket-Key: <base64>
Sec-WebSocket-Version: 13
```

Resposta:

```text
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: <value>
```

Comportamento atual:

- Echo de frames texto.
- Echo de frames binarios.
- Resposta pong para ping.
- Close frame ao encerrar.
- Rejeicao por limite quando o frame excede `websocket.max_frame_bytes`.

## Status codes principais

| Status | Uso |
|--------|-----|
| `200 OK` | Respostas locais, arquivos e proxy bem-sucedido |
| `101 Switching Protocols` | Upgrade WebSocket aceito |
| `304 Not Modified` | Cache de arquivo estatico valido |
| `400 Bad Request` | Request malformada, JSON invalido ou WebSocket incompleto |
| `401 Unauthorized` | Rota autenticada sem credencial valida |
| `403 Forbidden` | JWT valido sem escopos exigidos |
| `404 Not Found` | Rota ou arquivo nao encontrado |
| `405 Method Not Allowed` | Path existe para outro metodo |
| `413 Payload Too Large` | Body acima do limite |
| `414 URI Too Long` | URI acima do limite |
| `429 Too Many Requests` | Rate limit excedido |
| `431 Request Header Fields Too Large` | Headers acima do limite |
| `500 Internal Server Error` | Erro interno |
| `502 Bad Gateway` | Falha ao obter resposta valida do upstream |

## Limites reais do contrato atual

- Upstreams de proxy aceitam `http://`.
- WebSocket nao e encaminhado para upstream.
- HTTP/2, HTTP/3 e proxy gRPC nao fazem parte do runtime atual.
- Conexoes HTTP persistentes completas ainda nao fazem parte do comportamento do runtime.
