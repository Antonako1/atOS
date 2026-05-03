#!/usr/bin/env bash
# INSTALL_DEPS.sh — Download and install exact dependency versions for atOS
# Builds from source where package managers cannot guarantee the exact version.
# Run as a normal user; sudo is invoked only when needed.

set -euo pipefail

# --- Versions ---
QEMU_VER="8.2.2"
NASM_VER="2.16"
MAKE_VER="4.3"
GCC_VER="13.3.0"
GENISOIMAGE_VER="1.1.11"   # shipped as part of cdrkit
CMAKE_VER="3.28.3"
NINJA_VER="1.11.1"

JOBS=$(nproc)
INSTALL_PREFIX="/usr/local"
BUILD_DIR="$(pwd)/.deps_build"
mkdir -p "$BUILD_DIR"

# Colour helpers
RED='\033[0;31m'; GREEN='\033[0;32m'; CYAN='\033[0;36m'; NC='\033[0m'
info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

check_version() {
    local tool="$1" expected="$2" got="$3"
    if [[ "$got" == *"$expected"* ]]; then
        success "$tool $expected"
    else
        echo -e "${RED}[WARN]${NC}  $tool version mismatch — expected $expected, got: $got"
    fi
}

# ---------------------------------------------------------------------------
# System prerequisites (compiler, linker, libs needed to build everything)
# ---------------------------------------------------------------------------
install_build_prerequisites() {
    info "Installing build prerequisites via apt..."
    sudo apt-get update -qq
    sudo apt-get install -y --no-install-recommends \
        build-essential wget curl git ca-certificates \
        libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev \
        libslirp-dev libsdl2-dev \
        libncurses-dev libssl-dev \
        flex bison texinfo \
        python3 python3-pip ninja-build \
        cmake \
        pkg-config re2c \
        > /dev/null
    success "Build prerequisites installed"
}

# ---------------------------------------------------------------------------
# 1. NASM 2.16
# ---------------------------------------------------------------------------
install_nasm() {
    if command -v nasm &>/dev/null && nasm --version | grep -q "$NASM_VER"; then
        success "NASM $NASM_VER already installed"; return
    fi
    info "Building NASM $NASM_VER..."
    cd "$BUILD_DIR"
    wget -q "https://www.nasm.us/pub/nasm/releasebuilds/${NASM_VER}/nasm-${NASM_VER}.tar.gz"
    tar xf "nasm-${NASM_VER}.tar.gz"
    cd "nasm-${NASM_VER}"
    ./configure --prefix="$INSTALL_PREFIX" > /dev/null
    make -j"$JOBS" > /dev/null
    sudo make install > /dev/null
    check_version "NASM" "$NASM_VER" "$(nasm --version)"
}

# ---------------------------------------------------------------------------
# 2. Make 4.3
# ---------------------------------------------------------------------------
install_make() {
    if make --version 2>&1 | grep -q "4\.3"; then
        success "Make $MAKE_VER already installed"; return
    fi
    info "Building Make $MAKE_VER..."
    cd "$BUILD_DIR"
    wget -q "https://ftp.gnu.org/gnu/make/make-${MAKE_VER}.tar.gz"
    tar xf "make-${MAKE_VER}.tar.gz"
    cd "make-${MAKE_VER}"
    ./configure --prefix="$INSTALL_PREFIX" > /dev/null
    make -j"$JOBS" > /dev/null
    sudo make install > /dev/null
    check_version "Make" "$MAKE_VER" "$(make --version | head -1)"
}

# ---------------------------------------------------------------------------
# 3. GCC 13.3.0
# ---------------------------------------------------------------------------
install_gcc() {
    if gcc --version 2>&1 | grep -q "13\.3\.0"; then
        success "GCC $GCC_VER already installed"; return
    fi
    info "Building GCC $GCC_VER (this will take a while)..."
    sudo apt-get install -y --no-install-recommends \
        libgmp-dev libmpfr-dev libmpc-dev libisl-dev > /dev/null
    cd "$BUILD_DIR"
    wget -q "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.gz"
    tar xf "gcc-${GCC_VER}.tar.gz"
    mkdir -p "gcc-${GCC_VER}-build"
    cd "gcc-${GCC_VER}-build"
    "../gcc-${GCC_VER}/configure" \
        --prefix="$INSTALL_PREFIX" \
        --enable-languages=c,c++ \
        --disable-multilib \
        --disable-bootstrap \
        > /dev/null
    make -j"$JOBS" > /dev/null
    sudo make install > /dev/null
    check_version "GCC" "$GCC_VER" "$(gcc --version | head -1)"
}

# ---------------------------------------------------------------------------
# 4. CMake 3.28.3
# ---------------------------------------------------------------------------
install_cmake() {
    if cmake --version 2>&1 | grep -q "3\.28\.3"; then
        success "CMake $CMAKE_VER already installed"; return
    fi
    info "Installing CMake $CMAKE_VER via pre-built binary..."
    cd "$BUILD_DIR"
    ARCH=$(uname -m)
    CMAKE_PKG="cmake-${CMAKE_VER}-linux-${ARCH}.sh"
    wget -q "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VER}/${CMAKE_PKG}"
    chmod +x "$CMAKE_PKG"
    sudo sh "$CMAKE_PKG" --skip-license --prefix="$INSTALL_PREFIX" > /dev/null
    check_version "CMake" "$CMAKE_VER" "$(cmake --version | head -1)"
}

# ---------------------------------------------------------------------------
# 5. Ninja 1.11.1
# ---------------------------------------------------------------------------
install_ninja() {
    if ninja --version 2>&1 | grep -q "1\.11\.1"; then
        success "Ninja $NINJA_VER already installed"; return
    fi
    info "Installing Ninja $NINJA_VER via pre-built binary..."
    cd "$BUILD_DIR"
    ARCH=$(uname -m)
    # Map arch names to ninja release naming
    case "$ARCH" in
        x86_64)  NINJA_ARCH="linux" ;;
        aarch64) NINJA_ARCH="linux-aarch64" ;;
        *)        NINJA_ARCH="linux" ;;
    esac
    wget -q "https://github.com/ninja-build/ninja/releases/download/v${NINJA_VER}/ninja-${NINJA_ARCH}.zip"
    unzip -o "ninja-${NINJA_ARCH}.zip" > /dev/null
    sudo install -m 755 ninja "$INSTALL_PREFIX/bin/ninja"
    check_version "Ninja" "$NINJA_VER" "$(ninja --version)"
}

# ---------------------------------------------------------------------------
# 6. Genisoimage 1.1.11  (cdrkit fork of cdrtools)
# ---------------------------------------------------------------------------
install_genisoimage() {
    if genisoimage --version 2>&1 | grep -q "1\.1\.11"; then
        success "Genisoimage $GENISOIMAGE_VER already installed"; return
    fi
    info "Building Genisoimage $GENISOIMAGE_VER (cdrkit)..."
    sudo apt-get install -y --no-install-recommends \
        libcap-dev libacl1-dev > /dev/null
    cd "$BUILD_DIR"
    wget -q "http://ftp.de.debian.org/debian/pool/main/c/cdrkit/cdrkit_${GENISOIMAGE_VER}.orig.tar.gz" \
        -O "cdrkit-${GENISOIMAGE_VER}.tar.gz" \
        || wget -q "https://launchpad.net/ubuntu/+archive/primary/+sourcefiles/cdrkit/${GENISOIMAGE_VER}-5/cdrkit_${GENISOIMAGE_VER}.orig.tar.gz" \
                -O "cdrkit-${GENISOIMAGE_VER}.tar.gz"
    tar xf "cdrkit-${GENISOIMAGE_VER}.tar.gz"
    cd "cdrkit-${GENISOIMAGE_VER}"
    mkdir -p build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" > /dev/null
    make -j"$JOBS" > /dev/null
    sudo make install > /dev/null
    check_version "Genisoimage" "$GENISOIMAGE_VER" "$(genisoimage --version 2>&1 | head -1)"
}

# ---------------------------------------------------------------------------
# 7. QEMU 8.2.2
# ---------------------------------------------------------------------------
install_qemu() {
    if qemu-system-i386 --version 2>&1 | grep -q "8\.2\.2"; then
        success "QEMU $QEMU_VER already installed"; return
    fi
    info "Building QEMU $QEMU_VER (i386 target only)..."
    sudo apt-get install -y --no-install-recommends \
        libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev \
        libslirp-dev libsdl2-dev libaio-dev libcap-ng-dev \
        libattr1-dev > /dev/null
    cd "$BUILD_DIR"
    wget -q "https://download.qemu.org/qemu-${QEMU_VER}.tar.xz"
    tar xf "qemu-${QEMU_VER}.tar.xz"
    cd "qemu-${QEMU_VER}"
    ./configure \
        --prefix="$INSTALL_PREFIX" \
        --target-list="i386-softmmu" \
        --enable-sdl \
        --enable-slirp \
        > /dev/null
    make -j"$JOBS" > /dev/null
    sudo make install > /dev/null
    check_version "QEMU" "$QEMU_VER" "$(qemu-system-i386 --version | head -1)"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
main() {
    echo "========================================"
    echo " atOS Dependency Installer"
    echo " Build dir: $BUILD_DIR"
    echo "========================================"

    install_build_prerequisites
    install_nasm
    install_make
    install_gcc
    install_cmake
    install_ninja
    install_genisoimage
    install_qemu

    echo ""
    echo "========================================"
    success "All dependencies installed."
    echo "========================================"
    echo ""
    info "Installed versions:"
    qemu-system-i386 --version | head -1
    nasm --version | head -1
    make --version  | head -1
    gcc  --version  | head -1
    genisoimage --version 2>&1 | head -1 || true
    cmake --version | head -1
    ninja --version

    info "You may need to open a new shell or run 'hash -r' for PATH changes to take effect."
}

main "$@"
