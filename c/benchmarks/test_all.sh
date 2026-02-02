#!/bin/bash
# Tests fonctionnels de toutes les fonctionnalités

echo "=========================================="
echo "  Tests Fonctionnels FaaS"
echo "=========================================="
echo ""

# Vérifier que tout tourne
if ! pgrep -f gateway > /dev/null; then
    echo "❌ Gateway n'est pas lancé"
    echo "   Lancez: make run-all"
    exit 1
fi

echo "✅ Gateway actif"
echo ""

# Test HTTP - toutes fonctions
echo "=== Test HTTP ==="
for FUNC in echo resize hello; do
    echo -n "Test $FUNC... "
    RESPONSE=$(curl -s http://localhost:8080/function/$FUNC)
    if [[ $RESPONSE == *"Function"* ]]; then
        echo "✅"
    else
        echo "❌ ÉCHEC"
    fi
done
echo ""

# Test HTTPS - SNI
echo "=== Test HTTPS (SNI) ==="
for HOST in echo resize hello; do
    echo -n "Test ${HOST}.faas.local... "
    RESPONSE=$(curl -k -s https://${HOST}.faas.local:8443/)
    if [[ $RESPONSE == *"Function"* ]]; then
        echo "✅"
    else
        echo "❌ ÉCHEC"
    fi
done
echo ""

# Test toutes méthodes HTTP
echo "=== Test méthodes HTTP (echo) ==="
for METHOD in GET POST PUT DELETE HEAD; do
    echo -n "Test $METHOD... "
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X $METHOD http://localhost:8080/function/echo -d "test")
    if [[ $STATUS == "200" ]]; then
        echo "✅"
    else
        echo "❌ (HTTP $STATUS)"
    fi
done

echo ""
echo "✅ Tests terminés"
