#!/bin/bash
# certs/gen_certs.sh
mkdir -p certs
cd certs

# 1. Créer une CA (Certificate Authority) locale
openssl genrsa -out ca.key 2048
openssl req -x509 -new -nodes -key ca.key -sha256 -days 1024 -out ca.crt -subj "/CN=MyLocalRootCA"

# 2. Créer le certificat pour echo.local
openssl genrsa -out echo.key 2048
openssl req -new -key echo.key -out echo.csr -subj "/CN=echo.local"

# 3. Signer le certificat echo avec notre CA
openssl x509 -req -in echo.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out echo.crt -days 500 -sha256

# 4. Idem pour resize.local (pour plus tard)
openssl genrsa -out resize.key 2048
openssl req -new -key resize.key -out resize.csr -subj "/CN=resize.local"
openssl x509 -req -in resize.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out resize.crt -days 500 -sha256

echo "✅ Certificats générés dans certs/"