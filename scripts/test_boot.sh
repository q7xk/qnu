#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
make -j4
make build/qnu.img
LIBS=$(find /tmp/opencode/qnu-tools/extract -name '*.so*' -type f -exec dirname {} \; | sort -u | tr '\n' ':')
QEMU=${QEMU:-/tmp/opencode/qnu-tools/extract/qemu/usr/bin/qemu-system-x86_64}
OVMF=${OVMF:-/tmp/opencode/qnu-tools/extract/ovmf/usr/share/edk2/x64/OVMF_CODE.4m.fd}
OVMF_VARS=${OVMF_VARS:-/tmp/opencode/qnu-tools/extract/ovmf/usr/share/edk2/x64/OVMF_VARS.4m.fd}
cp "$OVMF_VARS" build/OVMF_VARS.fd
OUT=$(timeout 25 env LD_LIBRARY_PATH="$LIBS" "$QEMU" \
  -L /tmp/opencode/qnu-tools/roms \
  -machine q35 -m 256M -nic none -serial stdio -display none -no-reboot -no-shutdown \
  -drive if=pflash,format=raw,readonly=on,file="$OVMF" \
  -drive if=pflash,format=raw,file=build/OVMF_VARS.fd \
  -drive format=raw,file=build/qnu.img 2>&1 || true)
echo "$OUT" | tail -80
for s in "QNU Kernel 0.0.7" "Boot: UEFI/BIOS (Limine)" "[C] Core initialized" "[ASM] CPU environment initialized" "[RUST] Safe kernel components initialized" "[ZIG] System components initialized" "[OK] Memory manager initialized" "[OK] Interrupt manager initialized" "[OK] Timer initialized" "QNU: Kernel initialized successfully."; do
  echo "$OUT" | grep -Fq "$s" || { echo "MISSING: $s"; exit 1; }
done
echo "UEFI BOOT TEST PASS"

# BIOS/ISO test
LD_LIBRARY_PATH="$LIBS" make build/qnu.iso
OUT2=$(timeout 25 env LD_LIBRARY_PATH="$LIBS" "$QEMU" \
  -L /tmp/opencode/qnu-tools/roms \
  -machine q35 -m 256M -nic none -serial stdio -display none -no-reboot -no-shutdown \
  -cdrom build/qnu.iso -boot d 2>&1 || true)
echo "$OUT2" | tail -40
echo "$OUT2" | grep -Fq "QNU: Kernel initialized successfully." || { echo "MISSING BIOS BOOT"; exit 1; }
echo "BIOS BOOT TEST PASS"
