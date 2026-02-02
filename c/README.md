#  FaaS avec Keep-Alive et Migration TLS

Système Function-as-a-Service avec support HTTP/HTTPS, Keep-Alive et migration dynamique de sessions TLS entre containers.

---

##  Prérequis
```bash
sudo apt-get update
sudo apt-get install -y build-essential autoconf automake libtool openssl curl netcat
```

---

##  Installation

### 1. Cloner le projet
```bash
git clone <votre-repo>
cd faad_optimization/c
```

### 2. Installer WolfSSL localement
```bash
# Cloner WolfSSL dans le dossier du projet c
git clone https://github.com/wolfSSL/wolfssl.git

# Compiler WolfSSL avec toutes les fonctionnalités
cd wolfssl
./autogen.sh
./configure --enable-all --enable-debug
make
cd ..
```

**Important** : Le dossier `wolfssl/` doit être à la racine du projet c.

### 3. Compiler le projet
```bash
make clean
make
```

### 4. Générer les certificats
```bash
make certs
```

---

## 🚀 Lancement
```bash
make run-all
```

Le système démarre :
- **Gateway** : Port 8080 (HTTP) et 8443 (HTTPS)
- **Echo container** : `/tmp/echo.sock`
- **Resize container** : `/tmp/resize.sock`

---

##  Tests

### Test HTTP Keep-Alive
```bash
# 3 requêtes sur 1 connexion (echo → resize → echo)
(printf "GET /echo HTTP/1.1\r\nHost: echo.local\r\nConnection: keep-alive\r\n\r\n"; \
 sleep 1; \
 printf "GET /resize HTTP/1.1\r\nHost: echo.local\r\nConnection: keep-alive\r\n\r\n"; \
 sleep 1; \
 printf "GET /echo HTTP/1.1\r\nHost: echo.local\r\nConnection: close\r\n\r\n") | \
nc localhost 8080
```

### Test HTTPS Keep-Alive
```bash
(printf "GET /echo HTTP/1.1\r\nHost: echo.local\r\nConnection: keep-alive\r\n\r\n"; \
 sleep 1; \
 printf "GET /resize HTTP/1.1\r\nHost: echo.local\r\nConnection: close\r\n\r\n") | \
openssl s_client -connect localhost:8443 -servername echo.local \
  -CAfile certs/ca.crt -quiet 2>/dev/null
```

### Test avec curl
```bash
# HTTP
curl -H "Host: echo.local" http://localhost:8080/echo
curl -H "Host: echo.local" http://localhost:8080/resize

# HTTPS
curl -k https://echo.local:8443/echo
curl -k https://echo.local:8443/resize
```

---

##  Voir les logs
```bash
# Logs en temps réel
tail -f /tmp/echo.log
tail -f /tmp/resize.log
tail -f /tmp/gateway.log
```

---

## Arrêter
```bash
make stop
```

---

##  Structure du projet
```
c/
├── wolfssl/              # ← WolfSSL (clone git)
├── src/
│   ├── gateway_sni.c     # Gateway HTTP/HTTPS
│   ├── sni_parser.c/h    # Parser SNI (HTTPS)
│   ├── host_parser.h     # Parser Host (HTTP)
│   ├── utils.c/h         # Utilitaires
│   ├── ipc_protocol.h    # Protocole IPC
│   ├── tls_transfer.h    # Export/Import TLS
│   ├── handler_types.h   # Types handlers
│   └── containers/
│       ├── base_container.h  # Code commun
│       ├── echo.c           # Container echo
│       └── resize.c         # Container resize
├── certs/                # Certificats SSL
├── bin/                  # Exécutables compilés
├── Makefile
└── README.md
```

---

##Fonctionnalités

- ✅ **HTTP** (port 8080) et **HTTPS** (port 8443)
- ✅ **Keep-Alive** avec multiples requêtes par connexion
- ✅ **Migration TLS** entre containers (Hot Potato)
- ✅ **Zero-copy** avec `sendfd()`
- ✅ **Chiffrement end-to-end** (Gateway ne déchiffre pas)
- ✅ **Timeout** : 30 secondes d'inactivité
- ✅ **Limite** : 100 requêtes par connexion

---

##Dépannage

### Problème : `wolfssl/internal.h` introuvable
```bash
# Vérifier que wolfssl est compilé
ls wolfssl/src/.libs/libwolfssl.a
# Si vide, recompiler
cd wolfssl && make && cd ..
```

### Problème : Port déjà utilisé
```bash
make stop
# Ou manuellement
pkill -9 gateway echo resize
rm -f /tmp/*.sock
```

### Problème : Certificats invalides
```bash
rm -rf certs
make certs
```

---

##  Documentation

- **Architecture** : Voir `ARCHITECTURE.md`
- **API WolfSSL** : https://www.wolfssl.com/documentation/
- **Projet Master** : Migration TLS cross-container

---


