#!/usr/bin/env bash
# FLASH_USB.sh — Write the atOS ISO image to a USB stick for booting
# Usage: ./SCRIPTS/FLASH_USB.sh [/dev/sdX]
#   If no device is given, the script lists available drives and prompts.

set -euo pipefail

ISO="./OUTPUT/ISO/atOS.iso"

# --- Verify ISO exists ---
if [ ! -f "$ISO" ]; then
    echo "Error: ISO not found at $ISO"
    echo "Run 'make iso' first to build the image."
    exit 1
fi

ISO_SIZE=$(du -sh "$ISO" | cut -f1)
echo "ISO: $ISO ($ISO_SIZE)"

# --- Determine target device ---
TARGET="${1:-}"

if [ -z "$TARGET" ]; then
    echo ""
    echo "Available block devices:"
    lsblk -d -o NAME,SIZE,MODEL,TRAN | grep -v "^loop\|^sr" || true
    echo ""
    read -rp "Enter target device (e.g. /dev/sdb): " TARGET
fi

# --- Validate device ---
if [ ! -b "$TARGET" ]; then
    echo "Error: '$TARGET' is not a valid block device."
    exit 1
fi

# Refuse to write to an obvious system disk (mounted as /)
ROOT_DISK=$(lsblk -no PKNAME "$(findmnt -n -o SOURCE /)" 2>/dev/null | head -1 || true)
if [ -n "$ROOT_DISK" ] && [ "/dev/$ROOT_DISK" = "$TARGET" ]; then
    echo "Error: '$TARGET' appears to be the system drive. Aborting."
    exit 1
fi

# --- Confirm before writing ---
echo ""
echo "WARNING: ALL DATA ON $TARGET WILL BE DESTROYED."
lsblk "$TARGET" || true
echo ""
read -rp "Type 'yes' to confirm: " CONFIRM

if [ "$CONFIRM" != "yes" ]; then
    echo "Aborted."
    exit 0
fi

# --- Unmount any mounted partitions on the device ---
echo ""
echo "Unmounting partitions on $TARGET ..."
MOUNTS=$(lsblk -ln -o MOUNTPOINT "$TARGET" | grep -v '^$' || true)
if [ -n "$MOUNTS" ]; then
    grep -oP "/dev/[^ ]+" /proc/mounts | grep "^${TARGET}" | while read -r part; do
        sudo umount "$part" && echo "  Unmounted $part"
    done
fi

# --- Write ISO ---
echo ""
echo "Flashing $ISO to $TARGET ..."
sudo dd if="$ISO" of="$TARGET" bs=4M status=progress conv=fsync
echo ""
echo "Syncing..."
sudo sync

echo ""
echo "Ejecting $TARGET ..."
if command -v udisksctl &>/dev/null; then
    sudo udisksctl power-off -b "$TARGET"
else
    sudo eject "$TARGET"
fi

echo "Done. $TARGET can be safely removed."
