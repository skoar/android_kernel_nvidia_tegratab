#!/bin/bash
 
export ARCH=arm
export CROSS_COMPILE=`pwd`/uber/bin/arm-linux-androideabi-
make tegra_tegratab_android_defconfig
make -j8
