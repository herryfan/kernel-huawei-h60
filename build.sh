#!/usr/bin/bash

CROSS_COMPILE=/android/cyanogenmod/cm-11.0/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
export CROSS_COMPILE

make mrproper
rm -f drivers/vendor/hisi/build/delivery/hi3630_udp/log/obuild.log
rm -f drivers/vendor/hisi/build/delivery/hi3630_udp/timestamp.log
rm -f drivers/vendor/hisi/modem/ps/build/tl/APP_CORE/.tmp_versions/LPS.mod 

pushd drivers/vendor/hisi/build
python obuild.py product=hi3630_udp acore-oam_ps -j8
popd

make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} merge_hi3630_defconfig
make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j8
