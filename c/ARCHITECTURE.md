# Architecture FaaS avec SendFD + SNI + Keep-Alive

## Vue d'ensemble

```
Client ─► Gateway ───sendfd──► Container A ───Keep-Alive──► Container B
          (SNI)                (TLS Handshake)  (FD Return)
```

## Composants

### 1. Gateway (gateway_sni.c)

**Responsabilités:**
- Écoute HTTP (port 8080) et HTTPS (port 8443)
- **HTTP** : Parse l'URL avec MSG_PEEK → Route sur `/function/NAME`
- **HTTPS** : Parse le SNI du ClientHello avec MSG_PEEK → Route sur `NAME.faas.local`
- Transfère le FD via `sendmsg(SCM_RIGHTS)` au container approprié
- **Ne déchiffre JAMAIS** : Zero-copy + chiffrement end-to-end

**Table de routage:**
```c
echo    → /tmp/echo.sock
resize  → /tmp/resize.sock
hello   → /tmp/hello.sock
```

### 2. Containers (echo, resize, hello)

**Responsabilités:**
- Reçoivent le FD via `recvmsg(SCM_RIGHTS)`
- **HTTP** : Traitent directement la requête
- **HTTPS** : Font le handshake TLS avec leur propre certificat
- Supportent **toutes les méthodes** : GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS
- **Keep-Alive** : Si requête pour autre fonction → renvoient FD au Gateway

**Base commune (base_container.h):**
- `receive_fd()` : Réception FD + métadonnées
- `handle_http_connection()` : Traitement HTTP
- `handle_https_connection()` : Traitement HTTPS + TLS
- `return_fd_to_gateway()` : Migration Keep-Alive

### 3. Parsers

**sni_parser.h:**
- `parse_sni_from_client_hello()` : Extrait SNI du buffer TLS
- `peek_sni_from_socket()` : Lit SNI avec MSG_PEEK (sans consommer)

**http_parser.h:**
- `parse_http_request()` : Parse méthode, URL, headers
- `extract_function_name()` : Extrait nom de fonction depuis URL
- `build_http_response()` : Génère réponse HTTP

## Flux de requête

### HTTP Simple
```
1. Client → Gateway:8080 : GET /function/echo
2. Gateway MSG_PEEK → Parse URL → "echo"
3. Gateway sendfd(FD) → /tmp/echo.sock
4. Echo receive_fd() → Reçoit FD
5. Echo read(FD) → Traite requête
6. Echo write(FD) → Répond
7. Echo close(FD)
```

### HTTPS avec SNI
```
1. Client → Gateway:8443 : TLS ClientHello (SNI=echo.faas.local)
2. Gateway MSG_PEEK → Parse SNI → "echo"
3. Gateway sendfd(FD) → /tmp/echo.sock
4. Echo receive_fd() → Reçoit FD
5. Echo wolfSSL_accept() → Handshake TLS complet
6. Echo wolfSSL_read() → Traite requête chiffrée
7. Echo wolfSSL_write() → Répond chiffré
8. Echo wolfSSL_shutdown() + close(FD)
```

### Keep-Alive avec migration (TODO)
```
1. Client → Gateway : GET /function/echo (Keep-Alive)
2. Gateway → Echo : sendfd(FD)
3. Echo traite requête 1
4. Client → Echo : GET /function/resize (même connexion TCP)
5. Echo parse URL → "resize" (pas pour moi!)
6. Echo wolfSSL_dtls_export() → Exporte état TLS
7. Echo return_fd(FD, "resize", TLS_STATE) → Gateway
8. Gateway sendfd(FD, TLS_STATE) → Resize
9. Resize wolfSSL_dtls_import(TLS_STATE) → Importe état
10. Resize traite requête 2
```

## Avantages

1. **Zero-copy** : Pas de copie des bodies HTTP/HTTPS
2. **Chiffrement end-to-end** : Gateway ne déchiffre jamais
3. **Isolation** : Chaque fonction = certificat + processus séparé
4. **Scalabilité** : Ajout de fonctions = ajouter à la table de routage
5. **Keep-Alive** : Réutilisation connexion même entre fonctions différentes

## Limitations actuelles

- Keep-Alive migration TLS non implémenté (TODO: wolfSSL export/import)
- Pas de connection pool au Gateway
- Un seul worker par container (pas de multithreading)
- Parser HTTP basique (pas de chunked encoding, etc.)

## Pistes d'amélioration

1. **Implémenter TLS migration** : `wolfSSL_dtls_export/import`
2. **Connection pool** : Réutiliser connexions Unix
3. **Worker threads** : Pool de threads dans containers
4. **Parser HTTP avancé** : Utiliser picohttpparser
5. **Métriques** : Prometheus endpoint
6. **Load balancing** : Plusieurs instances par fonction
