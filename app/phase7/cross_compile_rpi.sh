#!/bin/bash
# ============================================================
#  Cross-compile rpi_someip_telemetry_service for RPi3 (aarch64)
#  Output: rpi_someip_service  (drop-and-run on RPi)
# ============================================================
set -e

# ── Toolchain ─────────────────────────────────────────────────
TOOLCHAIN_DIR=/home/abdo/x-tools/aarch64-rpi3-linux-gnu
CROSS=$TOOLCHAIN_DIR/bin/aarch64-rpi3-linux-gnu-g++

# Toolchain's own sysroot (provides libc, features.h, libstdc++, etc.)
TOOLCHAIN_SYSROOT=$TOOLCHAIN_DIR/aarch64-rpi3-linux-gnu/sysroot

# Cross-compiled vsomeip + Boost headers & libs
ARM_SYSROOT=/home/abdo/arm-sysroot

# ── Where vsomeip libs live on the RPi at runtime ─────────────
# Adjust this if your RPi installed vsomeip at a different path
RPI_LIB_PATH=/home/raspberry/someip-client/lib

# ── Output binary name ────────────────────────────────────────
OUT=rpi_someip_service

echo "=== Cross-compiling $OUT for aarch64-rpi3 ==="

$CROSS \
    -std=c++17 -O2 \
    --sysroot="$TOOLCHAIN_SYSROOT" \
    -I"$ARM_SYSROOT/include" \
    -I"$ARM_SYSROOT/include/vsomeip" \
    -L"$ARM_SYSROOT/lib" \
    -Wl,-rpath,"$RPI_LIB_PATH" \
    -Wl,-rpath-link,"$ARM_SYSROOT/lib" \
    -o "$OUT" \
    rpi_someip_telemetry_service.cpp \
    -lvsomeip3 -lboost_system -lboost_thread -lboost_log -lboost_filesystem -lpthread

# Strip debug info to reduce binary size (optional but good for demo)
STRIP=$TOOLCHAIN_DIR/bin/aarch64-rpi3-linux-gnu-strip
$STRIP "$OUT"

echo ""
echo "✅ Built: $(pwd)/$OUT"
echo ""
echo "── Deploy to RPi ─────────────────────────────────────────"
echo "  scp $OUT vsomeip_service_rpi.json raspberry@10.207.134.245:~/"
echo ""
echo "── Run on RPi ────────────────────────────────────────────"
echo "  VSOMEIP_CONFIGURATION=vsomeip_service_rpi.json ./$OUT"
