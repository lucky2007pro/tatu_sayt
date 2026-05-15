#!/bin/bash
# TATU Web — Linux serverga o'rnatish skripti
# Foydalanish: sudo bash setup_server.sh

set -e

echo "=== 1. Paketlar yangilanmoqda ==="
apt-get update -y

echo "=== 2. Kerakli kutubxonalar o'rnatilmoqda ==="
apt-get install -y \
    git cmake g++ \
    libssl-dev libjsoncpp-dev \
    uuid-dev zlib1g-dev \
    libc-ares-dev libbrotli-dev

echo "=== 3. Drogon o'rnatilmoqda ==="
if [ ! -d "/tmp/drogon" ]; then
    git clone --depth 1 --recurse-submodules https://github.com/drogonframework/drogon /tmp/drogon
fi
cd /tmp/drogon
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make install
ldconfig
cd /

echo "=== 4. Loyiha build qilinmoqda ==="
REPO_DIR="/opt/tatu_web"
mkdir -p "$REPO_DIR/build"
cd "$REPO_DIR"
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

echo "=== 5. Systemd service o'rnatilmoqda ==="
cp "$REPO_DIR/tatu_web.service" /etc/systemd/system/
systemctl daemon-reload
systemctl enable tatu_web
systemctl restart tatu_web

echo ""
echo "=== O'rnatish tugadi! ==="
systemctl status tatu_web
