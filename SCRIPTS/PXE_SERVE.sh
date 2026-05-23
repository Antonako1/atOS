#!/usr/bin/env bash
# PXE_SERVE.sh — Serve atOS ISO over PXE so a test PC boots it via RJ45
#
# HOW IT WORKS:
#   1. Assigns a static IP to the laptop's Ethernet NIC
#   2. Runs dnsmasq as a DHCP + TFTP server on that interface
#   3. Test PC boots via PXE -> gets an IP -> downloads pxelinux + memdisk
#   4. memdisk loads the full atOS.iso into RAM; your bootloader runs normally
#
# REQUIREMENTS (install once with SCRIPTS/INSTALL_DEPS.sh or manually):
#   sudo apt install dnsmasq syslinux pxelinux syslinux-common
#
# USAGE:
#   sudo ./SCRIPTS/PXE_SERVE.sh [ETHERNET_INTERFACE]
#   e.g.  sudo ./SCRIPTS/PXE_SERVE.sh enp3s0
#
# On the test PC: enter BIOS/UEFI -> set boot order to "LAN / PXE" first.

set -euo pipefail

# ── Dependency check ──────────────────────────────────────────────────────────
MISSING_PKGS=()
for pkg in dnsmasq syslinux pxelinux syslinux-common; do
    if ! dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "install ok installed"; then
        MISSING_PKGS+=("$pkg")
    fi
done

if [ "${#MISSING_PKGS[@]}" -gt 0 ]; then
    echo "Missing packages: ${MISSING_PKGS[*]}"
    read -rp "Install them now? [y/N] " INSTALL_CONFIRM
    if [[ "$INSTALL_CONFIRM" =~ ^[Yy]$ ]]; then
        apt-get install -y "${MISSING_PKGS[@]}"
    else
        echo "Cannot continue without required packages. Aborting."
        exit 1
    fi
fi

ISO="./OUTPUT/ISO/atOS.iso"
LAPTOP_IP="192.168.99.1"
DHCP_RANGE_START="192.168.99.100"
DHCP_RANGE_END="192.168.99.200"
LEASE_TIME="1h"
TFTP_ROOT="/tmp/atOS-pxe"

# ── Root check ────────────────────────────────────────────────────────────────
if [ "$EUID" -ne 0 ]; then
    echo "Error: run this script with sudo."
    exit 1
fi

# ── ISO check ─────────────────────────────────────────────────────────────────
if [ ! -f "$ISO" ]; then
    echo "Error: ISO not found at $ISO — run 'make iso' first."
    exit 1
fi

# ── Detect Ethernet interface ─────────────────────────────────────────────────
IFACE="${1:-}"
if [ -z "$IFACE" ]; then
    echo "Available network interfaces:"
    ip -o link show | awk -F': ' '{print "  "$2}' | grep -v lo
    echo ""

    # wait 5s, if no input, default to enp4s0 (common on many laptops)
    read -rt 5 -rp "Enter Ethernet interface to use (e.g. enp3s0): Default: enp4s0 " IFACE
    IFACE="${IFACE:-enp4s0}"
fi

if ! ip link show "$IFACE" &>/dev/null; then
    echo "Error: interface '$IFACE' not found."
    exit 1
fi

# ── Locate pxelinux / memdisk files ───────────────────────────────────────────
find_file() {
    local name="$1"
    local found
    found=$(find /usr/lib/syslinux /usr/share/syslinux /usr/lib/PXELINUX /usr/share/PXELINUX \
                 /usr/lib/syslinux/modules /usr/share/syslinux/modules \
                 -name "$name" 2>/dev/null | head -1 || true)
    # fallback: search entire /usr
    if [ -z "$found" ]; then
        found=$(find /usr -name "$name" 2>/dev/null | head -1 || true)
    fi
    if [ -z "$found" ]; then
        echo "Error: '$name' not found despite packages being installed." >&2
        echo "  Try: dpkg -L pxelinux syslinux-common | grep '$name'" >&2
        exit 1
    fi
    echo "$found"
}

PXELINUX_BIN=$(find_file "pxelinux.0")
MEMDISK_BIN=$(find_file "memdisk")
LDLINUX_C32=$(find_file "ldlinux.c32")

# ── Build TFTP directory ───────────────────────────────────────────────────────
echo "Building TFTP root at $TFTP_ROOT ..."
rm -rf "$TFTP_ROOT"
mkdir -p "$TFTP_ROOT/pxelinux.cfg"

cp "$PXELINUX_BIN" "$TFTP_ROOT/pxelinux.0"
cp "$MEMDISK_BIN"  "$TFTP_ROOT/memdisk"
cp "$LDLINUX_C32"  "$TFTP_ROOT/ldlinux.c32"

ISO_ABS=$(realpath "$ISO")
cp "$ISO_ABS" "$TFTP_ROOT/atOS.iso"
echo "  Copied ISO ($(du -sh "$TFTP_ROOT/atOS.iso" | cut -f1))"

cat > "$TFTP_ROOT/pxelinux.cfg/default" <<EOF
DEFAULT atOS
PROMPT 0
TIMEOUT 30

LABEL atOS
  MENU LABEL Boot atOS (memdisk)
  LINUX memdisk
  APPEND iso
  INITRD atOS.iso
EOF

echo "  PXE config written."

# ── Assign static IP to the laptop NIC ────────────────────────────────────────
echo ""
echo "Configuring $IFACE with static IP $LAPTOP_IP ..."
ip link set "$IFACE" up
ip addr flush dev "$IFACE"
ip addr add "$LAPTOP_IP/24" dev "$IFACE"

# ── Write temporary dnsmasq config ────────────────────────────────────────────
DNSMASQ_CONF=$(mktemp /tmp/atOS-dnsmasq-XXXXXX.conf)
cat > "$DNSMASQ_CONF" <<EOF
# atOS PXE server — temporary config
interface=$IFACE
bind-interfaces

# DHCP
dhcp-range=$DHCP_RANGE_START,$DHCP_RANGE_END,$LEASE_TIME

# TFTP
enable-tftp
tftp-root=$TFTP_ROOT

# PXE boot file (BIOS legacy)
dhcp-boot=pxelinux.0,$LAPTOP_IP
EOF

# ── Stop any running system dnsmasq to avoid port 67/69 conflicts ─────────────
if systemctl is-active --quiet dnsmasq 2>/dev/null; then
    echo "Stopping system dnsmasq temporarily..."
    systemctl stop dnsmasq
    STOPPED_DNSMASQ=1
else
    STOPPED_DNSMASQ=0
fi

# ── Cleanup on exit ───────────────────────────────────────────────────────────
cleanup() {
    echo ""
    echo "Shutting down PXE server..."
    kill "$DNSMASQ_PID" 2>/dev/null || true
    rm -f "$DNSMASQ_CONF"
    ip addr flush dev "$IFACE" 2>/dev/null || true
    if [ "$STOPPED_DNSMASQ" -eq 1 ]; then
        systemctl start dnsmasq || true
    fi
    echo "Done. Interface $IFACE released."
}
trap cleanup EXIT INT TERM

# ── Start dnsmasq ─────────────────────────────────────────────────────────────
echo ""
echo "Starting PXE/DHCP/TFTP server on $IFACE ($LAPTOP_IP) ..."
dnsmasq --no-daemon --conf-file="$DNSMASQ_CONF" --log-dhcp &
DNSMASQ_PID=$!

echo ""
echo "============================================================"
echo "  PXE server is running."
echo "  Laptop:    $IFACE  →  $LAPTOP_IP"
echo "  Test PC:   set BIOS boot order to LAN/PXE and power on."
echo "  Press Ctrl+C to stop."
echo "============================================================"
echo ""

wait "$DNSMASQ_PID"
