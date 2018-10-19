# Variables added for the build system
ANDROID_ROOT=/home/amlogic/n-amlogic
BIONIC_LIB=${ANDROID_ROOT}/bionic/libc
LIBRARY_OUT=${ANDROID_ROOT}/out/target/product/txlx_t962x_r314/obj/lib/
ANDROID_CROSS_GCC=/opt/gcc-linaro-4.9.4-2017.01-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-

ANDROID_HEADERS=" -I${ANDROID_ROOT}/vendor/amlogic/dvb/android/ndk/include"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/arch-arm64/include"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/kernel/uapi"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/kernel/android/uapi"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/kernel/uapi/asm-arm64"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/stdio"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/include"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/../libm/include"

# Location of DTVKIT root folder

export DTVKIT_ROOT=${ANDROID_ROOT}/external/dtvkit
export DTVKIT_DVBCORE_ROOT=${DTVKIT_ROOT}/DVBCore

# Folder where the compilation products will be placed. This overrides the default 'build'
#export DTVKIT_OUTPUT_DIR=

export DTVKIT_CC=${ANDROID_CROSS_GCC}gcc
export DTVKIT_AR=${ANDROID_CROSS_GCC}ar

# For 64 bit compilation, set this to '1' 
export DTVKIT_USE_STDINT=1

# Additional compiler options
export DTVKIT_ADDITIONAL_COMPILER_OPTIONS="-Wall  -fno-short-enums -fPIC ${ANDROID_HEADERS}"
# For 64 bit compilation and to build on the Raspberry Pi, '-m32' MUST be removed
#export DTVKIT_ADDITIONAL_COMPILER_OPTIONS=-Wall

# When compiling in release mode, this overrides the default -O2
#export DTVKIT_OPTIMISATION_OPTION=

# Option to include platform source required for DTVKit HbbTV v1.5
export DTVKIT_INCLUDE_HBBTV=0
