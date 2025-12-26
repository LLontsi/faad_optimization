#!/bin/bash
set -e

echo "Creating socket directory..."
sudo mkdir -p /var/run/functions
sudo chmod 777 /var/run/functions

echo "Stopping old containers..."
docker stop echo-func 2>/dev/null || true
docker rm echo-func 2>/dev/null || true

echo "Cleaning sockets..."
sudo rm -f /var/run/functions/*.sock

echo "Starting echo container..."
docker run -d \
  --name echo-func \
  -v /var/run/functions:/var/run/function \
  -e FUNCTION_SOCKET=/var/run/function/echo.sock \
  sendfd-echo

sleep 2

echo "Setting permissions..."
sudo chmod 777 /var/run/functions/*.sock

echo ""
echo "✅ Container ready!"
docker ps | grep echo-func
ls -la /var/run/functions/
