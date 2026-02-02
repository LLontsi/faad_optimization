CC = gcc
CFLAGS = -Wall -O2 -I./src -I./wolfssl
WOLFSSL_LIB = wolfssl/src/.libs/libwolfssl.a -lm

all: bin/gateway bin/echo bin/resize

bin/gateway: src/gateway_sni.c src/sni_parser.c src/utils.c
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/gateway src/gateway_sni.c src/sni_parser.c src/utils.c

bin/echo: src/containers/echo.c src/utils.c src/picohttpparser.c
	$(CC) $(CFLAGS) -o bin/echo src/containers/echo.c src/utils.c src/picohttpparser.c $(WOLFSSL_LIB)

bin/resize: src/containers/resize.c src/utils.c src/picohttpparser.c
	$(CC) $(CFLAGS) -o bin/resize src/containers/resize.c src/utils.c src/picohttpparser.c $(WOLFSSL_LIB)

certs:
	mkdir -p certs
	openssl genrsa -out certs/ca.key 2048 2>/dev/null
	openssl req -new -x509 -days 3650 -key certs/ca.key -subj "/CN=MyLocalRootCA" -out certs/ca.crt 2>/dev/null
	openssl genrsa -out certs/echo.key 2048 2>/dev/null
	openssl req -new -key certs/echo.key -subj "/CN=echo.local" -out certs/echo.csr 2>/dev/null
	openssl x509 -req -in certs/echo.csr -CA certs/ca.crt -CAkey certs/ca.key -CAcreateserial -out certs/echo.crt -days 365 2>/dev/null
	rm -f certs/*.csr certs/*.srl

run-all: stop
	./bin/echo > /tmp/echo.log 2>&1 &
	./bin/resize > /tmp/resize.log 2>&1 &
	sleep 1
	./bin/gateway > /tmp/gateway.log 2>&1 &
	sleep 1

stop:
	pkill -9 gateway 2>/dev/null || true
	pkill -9 echo 2>/dev/null || true
	pkill -9 resize 2>/dev/null || true
	rm -f /tmp/*.sock

test-http:
	@echo "Test HTTP sur port 8080"
	@curl -v -H "Host: echo.local" http://localhost:8080/echo
	@echo ""
	@curl -v -H "Host: echo.local" http://localhost:8080/resize

test-https:
	@echo "Test HTTPS sur port 8443"
	@(printf "GET /echo HTTP/1.1\r\nHost: echo.local\r\nConnection: keep-alive\r\n\r\n"; sleep 1; printf "GET /resize HTTP/1.1\r\nHost: echo.local\r\nConnection: close\r\n\r\n") | openssl s_client -connect localhost:8443 -servername echo.local -CAfile certs/ca.crt -quiet 2>/dev/null || true

clean:
	rm -rf bin

.PHONY: all certs run-all stop test-http test-https clean