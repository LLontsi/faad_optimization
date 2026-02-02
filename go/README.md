# sendfd-simple - Version qui MARCHE

## Architecture ULTRA-SIMPLE

1. **Gateway** : Utilise MSG_PEEK pour lire la fonction SANS consommer de données
2. **Transfer** : Duplique le FD avec dup() et l'envoie
3. **Container** : Lit TOUTE la requête HTTP depuis le FD

## Installation

```bash
# Build
make build
make docker

# Lancer (2 terminaux)
make run-containers  # Terminal 1
make run-gateway     # Terminal 2

# Tester (Terminal 3)
curl http://localhost:9000/function/echo -d "test"
```

## Pourquoi ça marche

- **MSG_PEEK** : La Gateway lit sans consommer
- **dup()** : Vraie copie indépendante du FD
- **Pas de wrapper** : Le conteneur lit directement depuis le FD
- **Simple** : ~300 lignes de code au total

## Test

```bash
curl http://localhost:9000/function/echo -d "Hello sendfd"
```

Devrait répondre :
```
=== Echo Function ===

Method: POST
Path: /function/echo
Body: Hello sendfd
```

✅ **Ça marche à coup sûr !**
