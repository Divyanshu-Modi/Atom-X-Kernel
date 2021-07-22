#bin/#!/bin/bash

echo "  _____    ______   _____    ______   ___    _  "
echo " |  __ \  |  ____| / ____|  |  ____| |   \  | | "
echo " | |__| } | |___  | /  ___  | |___   | .\ \ | | "
echo " |  _  /  |  ___| | | |_  | |  ___|  | | \ \| | "
echo " | | \ \  | |____ | \___| | | |____  | |  \ ' | "
echo " |_|  \_\ |______| \_____/  |______| |_|   \__| "

PREFIX=
SUFFIX=AtomX-
AUTO=0
SDFCF=1
regen() {
    DFCF=AtomX-$DEVICE${CAM_LIB}_defconfig
    if [[ ! -f $KERNEL_DIR/arch/arm64/configs/$DFCF ]]; then
        DFCF=$DEVICE${CAM_LIB}-perf_defconfig
    fi
make O=work $DFCF ARCH=arm64          \
      CC=aarch64-elf-gcc              \
      LLVM=1                          \
      HOSTLD=ld.lld                   \
      HOSTCXX=aarch64-elf-g++         \
      HOSTCC=gcc                      \
      PATH=$HOME/gcc-arm64/bin:$PATH  \
      CROSS_COMPILE=aarch64-elf-      \
      CROSS_COMPILE_COMPAT=$HOME/gcc-arm32/bin/arm-eabi-
if [[ "$SDFCF" == "1" ]]; then
    make O=work savedefconfig
    mv work/defconfig arch/arm64/configs/$DFCF
else
    mv work/.config arch/arm64/configs/$DFCF
fi
}

tulip() {
DEVICE=tulip
regen
}

whyred() {
DEVICE=whyred
regen
}

jasmine() {
DEVICE=a26x
regen
}

lavender() {
DEVICE=lavender
regen
}

tulip
whyred
jasmine
lavender
if [[ "$AUTO" == "1" ]]; then
    if [[ "$SDFCF" == "1" ]]; then
        git commit -asm "Defconfig: Xiaomi: regen with savedefconfig" 
    else
        git commit -asm "Defconfig: Xiaomi regen"
    fi
fi
rm -rf work
