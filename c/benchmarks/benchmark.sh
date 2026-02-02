#!/bin/bash
# Script de benchmark complet

echo "========================================"
echo "  BENCHMARKS FaaS avec SNI + SendFD"
echo "========================================"
echo ""

if ! command -v wrk &> /dev/null; then
    echo "❌ wrk non installé. Installez avec: sudo apt install wrk"
    exit 1
fi

mkdir -p results
RESULT="results/bench-$(date +%Y%m%d-%H%M%S).txt"

exec > >(tee "$RESULT")

echo "Configuration: $(nproc) cores, $(free -h | grep Mem | awk '{print $2}') RAM"
echo ""

# HTTP - Toutes fonctions
echo "=== HTTP - Test de toutes les fonctions ==="
for FUNC in echo resize hello; do
    echo "Function: $FUNC"
    wrk -t4 -c50 -d10s --latency http://localhost:8080/function/$FUNC
    echo ""
done

# HTTPS - SNI Passthrough
echo "=== HTTPS - SNI Passthrough ==="
for HOST in echo resize hello; do
    echo "SNI: ${HOST}.faas.local"
    wrk -t4 -c50 -d10s --latency https://${HOST}.faas.local:8443/
    echo ""
done

# Scalabilité
echo "=== Scalabilité (fonction echo) ==="
for CONNS in 10 50 100 200 500; do
    echo "${CONNS} connexions:"
    wrk -t4 -c${CONNS} -d5s http://localhost:8080/function/echo | grep "Requests/sec"
done

echo ""
echo "✅ Benchmarks terminés: $RESULT"
