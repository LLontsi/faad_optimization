.PHONY: build docker run-containers run-gateway test clean

build:
	@echo "Building..."
	@go build -o bin/gateway ./cmd/gateway
	@go build -o bin/container ./cmd/container
	@echo "✅ Built"

docker:
	@echo "Building Docker image..."
	@docker build -t sendfd-echo -f docker/Dockerfile .
	@echo "✅ Docker image built"

run-containers:
	@echo "Starting containers..."
	@./scripts/start.sh

run-gateway:
	@./bin/gateway

test:
	@echo "Testing..."
	@curl -v http://localhost:9000/function/echo -d "Hello World"

clean:
	@rm -rf bin/
	@docker stop echo-func 2>/dev/null || true
	@docker rm echo-func 2>/dev/null || true
	@sudo rm -rf /var/run/functions/* 2>/dev/null || true

all: clean build docker
