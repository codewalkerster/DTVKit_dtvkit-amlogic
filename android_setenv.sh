# Variables added for the build system
ANDROID_ROOT=${1}
BIONIC_LIB=${ANDROID_ROOT}/bionic/libc
LIBRARY_OUT=${2}
ANDROID_CROSS_GCC=${ANDROID_ROOT}/${3}

ANDROID_HEADERS=" -I${ANDROID_ROOT}/vendor/amlogic/dvb/android/ndk/include"
ANDROID_HEADERS+=" -I${ANDROID_ROOT}/vendor/amlogic/dvb/include"
ANDROID_HEADERS+=" -I${ANDROID_ROOT}/vendor/amlogic/dvb/include/am_adp"
ANDROID_HEADERS+=" -I${ANDROID_ROOT}/vendor/amlogic/dvb/include/am_mw"
ANDROID_HEADERS+=" -I${ANDROID_ROOT}/vendor/amlogic/dvb/android/ndk/include/linux"
ANDROID_HEADERS+=" -I${ANDROID_ROOT}/external/sqlite/dist"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/arch-arm64/include"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/kernel/uapi"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/kernel/android/uapi"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/stdio"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/include"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/../libm/include"
echo "platform $4"
if [ $4 = 'arm' ]; then
ANDROID_HEADERS+=" -I${BIONIC_LIB}/arch-arm/include"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/kernel/uapi/asm-arm"
elif [ $4 = 'arm64' ];then
ANDROID_HEADERS+=" -I${BIONIC_LIB}/arch-arm64/include"
ANDROID_HEADERS+=" -I${BIONIC_LIB}/kernel/uapi/asm-arm64"
else
    echo "Error!"
    return
fi

# Location of DTVKIT root folder

export DTVKIT_ROOT=${ANDROID_ROOT}/vendor/amlogic/external/dtvkit
export DTVKIT_DVBCORE_ROOT=${DTVKIT_ROOT}/DVBCore
export DTVKIT_CIPLUS_ROOT=${DTVKIT_ROOT}/CI-Plus
export DTVKIT_MHEG5_ROOT=${DTVKIT_ROOT}/MHEG5

# Folder where the compilation products will be placed. This overrides the default 'build'
#export DTVKIT_OUTPUT_DIR=

export DTVKIT_CC=${ANDROID_CROSS_GCC}gcc
export DTVKIT_AR=${ANDROID_CROSS_GCC}ar

# For 64 bit compilation, set this to '1' 
if [ $4 = 'arm' ]; then
export DTVKIT_USE_STDINT=0
elif [ $4 = 'arm64' ]; then
export DTVKIT_USE_STDINT=1
else
    echo "Error!!!"
    return
fi

# Additional compiler options
export DTVKIT_ADDITIONAL_COMPILER_OPTIONS="-Wall  -fno-short-enums -fPIC ${ANDROID_HEADERS}"
# For 64 bit compilation and to build on the Raspberry Pi, '-m32' MUST be removed
#export DTVKIT_ADDITIONAL_COMPILER_OPTIONS=-Wall

# When compiling in release mode, this overrides the default -O2
#export DTVKIT_OPTIMISATION_OPTION=

# Option to include platform source required for DTVKit HbbTV v1.5
export DTVKIT_INCLUDE_HBBTV=0

# To include hard coded CI Plus test keys and certificates, enable
#export DTVKIT_INCLUDE_TEST_KEYS=1
