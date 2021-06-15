#!/usr/bin/env bash

set -euo pipefail

# Check arguments

readonly ARGS=$1
readonly SDK_PATH=$2

if [ -z "$ARGS" ]; then
  echo "Error: No device was specified. Please specify either 'harmattan' or 'simulator', case sensitive, as the first argument to the script."
  exit 1
fi

if [[ "$ARGS" != "harmattan" ]] && [[ "$ARGS" != "simulator" ]]; then
  echo "Error: Incorrect device was specified. Please specify either 'harmattan' or 'simulator', case sensitive, as the first argument to the script."
  exit 1
fi

# More directory variables

export PATH=/opt/strawberry-gcc-11.1/bin:$PATH

readonly TOOLCHAIN_PREFIX=arm-none-linux-gnueabi

# We check sha256 of all tarballs we download
check_sha256()
{
  if ! ( echo "$1  $2" | sha256sum -c --status - )
  then
    echo "Error: sha256 of $2 doesn't match the known one."
    echo "Expected: $1  $2"
    echo -n "Got: "
    sha256sum "$2"
    exit 1
  else
    echo "sha256 matches the expected one: $1"
  fi
}

# OpenSSL

OPENSSL_VERSION=1.1.1k
OPENSSL_HASH="892a0875b9872acd04a9fde79b1f943075d5ea162415de3047c327df33fbaee5"
OPENSSL_FILENAME="openssl-$OPENSSL_VERSION.tar.gz"

build_openssl()
{
  rm -rf ./openssl-$OPENSSL_VERSION

  if [ ! -f $OPENSSL_FILENAME ];
  then
    echo "Downloading OpenSSL sources..."
    wget https://www.openssl.org/source/$OPENSSL_FILENAME
    check_sha256 "$OPENSSL_HASH" "$OPENSSL_FILENAME"
  fi

  echo "Unpacking OpenSSL sources..."
  tar xzf $OPENSSL_FILENAME || exit 1
  cd openssl-$OPENSSL_VERSION


  if [[ "$ARGS" == "harmattan" ]]
  then
    ./Configure --cross-compile-prefix=$TOOLCHAIN_PREFIX- linux-generic32 no-shared no-unit-test
  else
    ./config
  fi

  echo "Building OpenSSL..."
  make depend || exit 1
  make -j 4 || exit 1

  rm -rf ../build/crypto || exit 1
  mkdir -p ../build/crypto/lib || exit 1
  cp libcrypto.a libssl.a ../build/crypto/lib || exit 1
  cp -r include ../build/crypto || exit 1

  cd ..
}


# ZLib

ZLIB_VERSION=1.2.11
ZLIB_HASH="c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1"
ZLIB_FILENAME="zlib-$ZLIB_VERSION.tar.gz"

build_zlib()
{
  rm -rf ./zlib-$ZLIB_VERSION

  if [ ! -f $ZLIB_FILENAME ]; then
    echo "Downloading zlib sources..."
    wget https://zlib.net/$ZLIB_FILENAME
    check_sha256 "$ZLIB_HASH" "$ZLIB_FILENAME"
  fi

  echo "Unpacking zlib sources..."
  tar xzf $ZLIB_FILENAME || exit 1
  cd zlib-$ZLIB_VERSION

  if [[ "$ARGS" == "harmattan" ]]; then
    CC=$TOOLCHAIN_PREFIX-gcc CFLAGS="-fPIC" ./configure
  else
    CFLAGS="-fPIC" ./configure
  fi

  echo "Building zlib..."
  make -j 4 || exit 1

  rm -rf ../build/zlib || exit 1
  mkdir -p ../build/zlib/lib || exit 1
  mkdir -p ../build/zlib/include || exit 1
  cp libz.a ../build/zlib/lib || exit 1
  cp zconf.h zlib.h ../build/zlib/include || exit 1

  cd ..
}

# TDLib

TD_GIT_HASH=4f00f445b7c56e0fd5631440d4bce34b3773aa1f
build_tdlib()
{
  rm -rf td

  git clone https://github.com/tdlib/td
  cd td
  git checkout $TD_GIT_HASH

  if [[ "$ARGS" == "harmattan" ]]; then
    sed -i s/'TD_HAS_MMSG 1'/'TD_HAS_MMSG 0'/g tdutils/td/utils/port/config.h
  fi

  cd ..

  rm -rf build/generate
  rm -rf build/tdlib

  mkdir -p build/generate
  mkdir -p build/tdlib

  TD_ROOT=$(realpath td)
  ZLIB_ROOT=$(realpath ./build/zlib)
  ZLIB_LIBRARIES=$ZLIB_ROOT/lib/libz.a
  OPENSSL_ROOT=$(realpath ./build/crypto)
  OPENSSL_CRYPTO_LIBRARY=$OPENSSL_ROOT/lib/libcrypto.a
  OPENSSL_SSL_LIBRARY=$OPENSSL_ROOT/lib/libssl.a

  OPENSSL_OPTIONS="-DOPENSSL_FOUND=1 \
                   -DOPENSSL_INCLUDE_DIR=\"$OPENSSL_ROOT/include\" \
                   -DOPENSSL_CRYPTO_LIBRARY=\"$OPENSSL_CRYPTO_LIBRARY\" \
                   -DOPENSSL_SSL_LIBRARY=\"$OPENSSL_SSL_LIBRARY\""

  ZLIB_OPTION="-DZLIB_FOUND=1 \
               -DZLIB_LIBRARIES=\"$ZLIB_LIBRARIES\" \
               -DZLIB_INCLUDE_DIR=\"$ZLIB_ROOT/include\""


  if [[ "$ARGS" == "harmattan" ]]; then
    echo "
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm)

SET(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

" > $TD_ROOT/toolchain.cmake
  fi

  if [[ "$ARGS" == "harmattan" ]]; then
    cd build/generate
    cmake $TD_ROOT || exit 1
    cd ../..

    cd build/tdlib
    cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DTD_ENABLE_LTO=ON -DCMAKE_TOOLCHAIN_FILE=$TD_ROOT/toolchain.cmake $OPENSSL_OPTIONS $ZLIB_OPTION $TD_ROOT || exit 1
    cd ../..

    echo "Generating TDLib autogenerated source files..."
    cmake --build build/generate --target prepare_cross_compiling || exit 1
    echo "Building TDLib to for Harmattan..."
    cmake --build build/tdlib || exit 1
  else
    cd build/tdlib
    cmake -DCMAKE_BUILD_TYPE=Release $OPENSSL_OPTIONS $ZLIB_OPTION $TD_ROOT || exit 1
    cmake --build .
  fi

  if [[ "$ARGS" == "harmattan" ]]; then
    make DESTDIR="$SDK_PATH/Madde/sysroots/harmattan_sysroot_10.2011.34-1_slim" install
  else
    make install
  fi

  echo "Successfull installed!"
}

run()
{
  build_zlib
  build_openssl
  build_tdlib
}

run
