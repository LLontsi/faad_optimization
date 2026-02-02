# 🚀 Projet FaaS Avancé avec SendFD + SNI

Prototype de Function-as-a-Service démontrant:
- **Zero-copy** avec sendfd()
- **SNI Passthrough** pour routing HTTPS
- **Multi-containers** (echo, resize, hello)
- **Toutes méthodes HTTP** (GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS)

## 🎯 Quick Start

```bash
# Installation dépendances
sudo apt install build-essential libwolfssl-dev openssl curl wrk

# Compilation
make

# Certificats
make certs

# /etc/hosts
make hosts

# Lancer
make run-all

# Tester
make test
```

## 📊 Tests

### HTTP
```bash
curl http://localhost:8080/function/echo
curl -X POST http://localhost:8080/function/echo -d "data"
curl http://localhost:8080/function/resize
curl http://localhost:8080/function/hello
```

### HTTPS (SNI)
```bash
curl -k https://echo.faas.local:8443/
curl -k https://resize.faas.local:8443/
curl -k https://hello.faas.local:8443/
```

### Benchmarks
```bash
make benchmark
```

## 📁 Structure

```
src/
├── gateway_sni.c          # Gateway avec SNI Passthrough
├── sni_parser.h           # Parser SNI
├── http_parser.h          # Parser HTTP
└── containers/
    ├── base_container.h   # Code commun
    ├── echo.c             # Container echo
    ├── resize.c           # Container resize
    └── hello.c            # Container hello
```

## 🎓 Contribution scientifique

- **SNI Passthrough** : Routing HTTPS sans déchiffrement
- **Zero-copy** : Transfert FD sans copie bodies
- **Chiffrement end-to-end** : Gateway ne voit jamais les données

---

**Projet Master Recherche**
