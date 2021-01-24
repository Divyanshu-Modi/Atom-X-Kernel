#!/bin/bash

red="\033[1;31m"
yellow="\033[1;33m"
green="\033[1;32m"
echo -e "$green"

clear

# Compiler_path

CLANG_PATH=${HOME}/proton-clang/bin
GCC_64=${HOME}/aarch64/bin
GCC_32=${HOME}/arm/bin
export PATH=${CLANG_PATH}:${PATH}

# Compiler_prebuilts
CROSS_COMPILE=aarch64-linux-gnu-
CROSS_COMPILE_ARM32=arm-linux-gnueabi-
CC=clang

# Device
DEVICE=tulip

# Export
LC_ALL=C date +%Y-%m-%d	
date=`date +"%Y%m%d-%H%M"`	
BUILD_START=$(date +"%s")
VERSION='perf'
KERNEL_DIR=`pwd`
REPACK_DIR=${HOME}/Repack
OUT=$KERNEL_DIR/out

# Scripts
export ARCH=arm64
rm -rf ${DEVICE}
mkdir ${DEVICE}
make clean mrproper
make O=${DEVICE} clean mrproper ${DEVICE}_defconfig
make O=${DEVICE} CC=$CC -j8 \ CROSS_COMPILE=$CROSS_COMPILE \ CROSS_COMPILE_ARM32=$CROSS_COMPILE_ARM32

cd $REPACK_DIR
cp $KERNEL_DIR/${DEVICE}/arch/arm64/boot/Image.gz-dtb $REPACK_DIR/
FINAL_ZIP="AtomX-${VERSION}.zip"
zip -r9 "${FINAL_ZIP}" *
cp *.zip $OUT
rm *.zip
cd $KERNEL_DIR
rm $REPACK_DIR/Image.gz-dtb

BUILD_END=$(date +"%s")	
DIFF=$(($BUILD_END - $BUILD_START))	
echo -e "Build completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds."
