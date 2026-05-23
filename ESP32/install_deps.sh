#!/usr/bin/env bash
# install_deps.sh - Install Arduino CLI, ESP32 core, and required libraries

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/config.sh"

echo "====================================="
echo " ESP32 Dependency Installer"
echo "====================================="
echo

# 1/4 Arduino CLI
if ! command -v arduino-cli >/dev/null 2>&1; then
    echo "[1/4] Installing Arduino CLI..."
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
    sudo mv bin/arduino-cli /usr/local/bin/
else
    echo "[1/4] Arduino CLI already installed."
fi

# 2/4 Init config + add ESP32 board URL
echo
echo "[2/4] Configuring Arduino CLI..."
arduino-cli config init || true
arduino-cli config set board_manager.additional_urls \
    https://espressif.github.io/arduino-esp32/package_esp32_index.json

# 3/4 Update index
echo
echo "[3/4] Updating package index..."
arduino-cli core update-index

# 4/4 Install core + libraries
echo
echo "[4/4] Installing ESP32 core and libraries..."
arduino-cli core install esp32:esp32
arduino-cli lib install "LiquidCrystal I2C"

echo
echo "Done. All dependencies installed."
