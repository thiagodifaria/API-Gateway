# ARQUITETURA

## Visao geral

API Gateway e um gateway HTTPS em C++17 construido a partir da evolucao natural de um servidor HTTPS de alta performance. A base original fornecia TLS com OpenSSL, serving estatico, validacao JSON, benchmarks e modulos SIMD/assembly. A arquitetura atual adiciona o papel de gateway: receber trafego externo, aplicar regras transversais, selecionar upstreams e encaminhar requisicoes HTTP para servicos internos.

O projeto tem duas preocupacoes principais:

- Ser um gateway funcional e observavel para trafego HTTP/HTTPS.
- Preservar a identidade de engenharia de sistemas do projeto original, com componentes de baixo nivel bem isolados.

## Topologia

```text
client
  |
  | HTTPS
  v
API Gateway
  |
  | HTTP
  v
upstream services
```

O gateway termina TLS na borda, interpreta a request HTTP, aplica middlewares e roteia para handlers locais ou para upstreams configurados.

## Organizacao do repositorio

```text
client-web/
  frontend tecnico estatico servido pelo gateway

docs/
  ARQUITETURA.md
  API.md

scripts/
  build.sh
  setup.sh
  run_benchmarks.sh

service-api/
  service-cpp/
    CMakeLists.txt
    config/
    certs/
    src/
    tests/
    third_party/

  service-assembly/
    crypto/
    utils/
```

## Modulos principais

### `core`

Responsavel pelo ciclo de vida do servidor:

- Inicializacao do OpenSSL.
- Carregamento de certificado e chave privada.
- Socket HTTPS.
- Event loop.
- Thread pool.
- Aceite de conexoes.
- Leitura da request.
- Chamada do router.
- Escrita da response.

O binario principal gerado pelo CMake e `api_gateway`.

### `http`

Contem as pecas HTTP centrais:

- `HttpRequest`.
- `HttpResponse`.
- Parser HTTP.
- Router.
- Static handler.

O parser separa metodo, URI, path, query string, headers, query params e body. Headers sao normalizados para busca case-insensitive. O router suporta rotas exatas, rotas wildcard e parametros de path como `/users/:id`.

### `gateway`

Contem as pecas de encaminhamento:

- Reverse proxy HTTP.
- Upstream pool.
- Load balancing.
- Health checks ativos.
- Health tracking passivo.

O reverse proxy seleciona um target saudavel, reescreve o path quando `strip_prefix` esta configurado, adiciona headers de proxy e remove headers hop-by-hop. Upstreams atualmente usam HTTP interno.

### `middleware`

Contem middlewares independentes:

- Rate limiter Token Bucket.
- Autenticacao por API key.
- Validacao JWT HMAC-SHA256.

O pipeline de middleware permite aplicar comportamento transversal sem misturar regras de seguranca, limite e observabilidade dentro dos handlers finais.

### `metrics`

Agrega sinais operacionais:

- Total de requests.
- Requests por rota.
- Respostas por status.
- Latencia media por rota.
- Hits de rate limit.
- Hits/misses de cache registrados.
- Estado de saude de upstreams.

As metricas sao expostas em `/gateway/metrics`, em texto Prometheus por padrao ou JSON quando o cliente envia `Accept: application/json`.

### `utils`

Reune componentes de suporte:

- Logger.
- Buffer.
- Operacoes HTTP aceleradas.
- Validacao.
- Compressao.
- Operacoes de rede.
- Benchmarks.

### `crypto`

Contem primitivas e provider OpenSSL customizado. O projeto mantem benchmarks e testes para AES, SHA-256 e P-256.

### `service-assembly`

Isola as rotinas assembly por dominio:

- Crypto.
- Memoria.
- HTTP scanning.
- Validacao.
- Compressao.
- Operacoes de rede.

Assembly nao carrega regra de negocio do gateway. Ele atua como aceleracao para operacoes bem delimitadas.

## Fluxo de request

```text
Socket TLS
  -> SSL_accept
  -> leitura para Buffer
  -> deteccao de fim de headers
  -> HttpParser
  -> Router
  -> Middleware pipeline
  -> Handler local ou ReverseProxy
  -> HttpResponse
  -> SSL_write
```

Quando a request e WebSocket, o servidor detecta `Upgrade: websocket`, executa o handshake RFC 6455 e entra no loop de frames de echo.

## Roteamento

O router aplica prioridade de forma simples e previsivel:

- Rotas exatas.
- Rotas parametrizadas.
- Rotas wildcard.

Quando uma rota existe para outro metodo, o gateway retorna `405 Method Not Allowed`. Requests `HEAD` podem usar handlers `GET`, com corpo removido na resposta.

## Reverse proxy

O reverse proxy:

- Seleciona target em um upstream pool.
- Usa `round_robin` ou `least_connections`.
- Aplica `strip_prefix` quando configurado.
- Encaminha metodo, path, query string, headers permitidos e body.
- Define `Host` para o target selecionado.
- Adiciona `X-Forwarded-For`, `X-Forwarded-Proto`, `X-Real-IP` e `Via`.
- Remove headers hop-by-hop.
- Registra sucesso/falha para health tracking passivo.
- Retorna `502 Bad Gateway` quando nao consegue obter resposta valida do upstream.

Headers hop-by-hop removidos:

- `Connection`
- `Keep-Alive`
- `Proxy-Authenticate`
- `Proxy-Authorization`
- `TE`
- `Trailer`
- `Transfer-Encoding`
- `Upgrade`

## Upstream pool e resiliencia

Cada upstream contem um conjunto de targets. O pool mantem:

- Estado `healthy`.
- Conexoes ativas.
- Total de requests.
- Falhas consecutivas.
- Sucessos consecutivos.
- Ultimo health check.

Health checks ativos fazem requests HTTP periodicos para o `health_path` configurado. O tracking passivo marca falhas quando conexoes ou respostas do upstream falham. Thresholds configuraveis controlam quando um target entra ou sai do estado unhealthy.

## Rate limiting

O rate limiting usa Token Bucket. A chave pode ser:

- IP.
- API key.
- Rota.

Quando o limite e excedido, o gateway retorna `429 Too Many Requests` com headers `RateLimit-*` e `Retry-After`.

## Autenticacao

A autenticacao suporta:

- API key por header configuravel.
- JWT assinado com HMAC-SHA256.

A validacao JWT verifica assinatura e campos temporais `exp` e `nbf`. Quando configurados, tambem valida `iss` e `aud`.

## Cache HTTP de estaticos

Arquivos estaticos servidos por `client-web/` recebem headers de cache. O handler calcula metadados do arquivo e responde `304 Not Modified` quando `If-None-Match` ou `If-Modified-Since` permite.

## WebSocket

O suporte WebSocket atual cobre:

- Handshake RFC 6455.
- Validacao de upgrade.
- Calculo de `Sec-WebSocket-Accept`.
- Decodificacao de frames mascarados client-to-server.
- Frames texto.
- Frames binarios.
- Ping/pong.
- Close.
- Limite de tamanho de frame.

O comportamento atual e echo local. O gateway nao faz proxy WebSocket para upstreams.

## Configuracao

O arquivo padrao fica em:

```text
service-api/service-cpp/config/config.json
```

Um arquivo alternativo pode ser selecionado com `HTTPS_SERVER_CONFIG`.

## Limites conhecidos

Esta secao descreve o estado real do runtime atual:

- Upstreams do reverse proxy usam HTTP, nao HTTPS.
- WebSocket esta implementado como echo local, nao como proxy para upstream.
- `required_scopes` existe na configuracao, mas nao ha enforcement de escopos no middleware atual.
- `observability.json_logs` existe na configuracao, mas o logger atual emite texto.
- O loop de conexao fecha a conexao HTTP apos uma resposta; `keep_alive` existe na configuracao, mas conexoes HTTP persistentes completas nao estao implementadas.

## Build e runtime

Build:

```bash
./scripts/build.sh Release
```

Execucao:

```bash
./build/api_gateway
# Windows/Git Bash: ./build/api_gateway.exe
```
