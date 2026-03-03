#!/bin/bash
set -e

if [ ! -f kernel.bin ]; then
    echo "[KOKIRI] No kernel.bin, run build.sh first"
    exit 1
fi

mkdir -p iso/boot/grub

cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0
menuentry "KOKIRI" {
    multiboot2 /boot/kernel.bin
    boot
}
EOF

cp kernel.bin iso/boot/kernel.bin
grub-mkrescue -o kokiri.iso iso 2>/dev/null
echo "[KOKIRI] Booting..."
qemu-system-x86_64 -cdrom kokiri.iso -display sdl -no-reboot -d int,cpu_reset 2>&1 | head -50
