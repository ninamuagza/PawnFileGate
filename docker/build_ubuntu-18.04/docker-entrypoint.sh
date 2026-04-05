#!/bin/sh
[ -z $CONFIG ] && config=Release || config="$CONFIG"
[ -z $BUILD_SERVER ] && build_server=1 || build_server="$BUILD_SERVER"
[ -z $BUILD_DIR ] && build_dir=build || build_dir="$BUILD_DIR"
[ -z $PAWNREST_ENABLE_TLS ] && tls_enabled=OFF || tls_enabled="$PAWNREST_ENABLE_TLS"
[ -z $PAWNREST_TLS_STATIC_OPENSSL ] && tls_static=OFF || tls_static="$PAWNREST_TLS_STATIC_OPENSSL"

cmake \
    -S . \
    -B "$build_dir" \
    -G Ninja \
    -DCMAKE_C_FLAGS=-m32 \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_BUILD_TYPE=$config \
    -DSTATIC_STDCXX=true \
    -DBUILD_SERVER=$build_server \
    -DPAWNREST_ENABLE_TLS="$tls_enabled" \
    -DPAWNREST_TLS_STATIC_OPENSSL="$tls_static" \
&&
cmake \
    --build "$build_dir" \
    --config $config \
    --parallel $(nproc)
