#!/usr/bin/env bash

MAKE_JOB_NUM=2
BUILD_SHARED_LIBS=OFF
OS_UNAME=$(uname)

PROJECT_ROOT=$(cd `dirname $0`; cd ..; pwd;)
cd ${PROJECT_ROOT}

function prepare() {

    if [ ! -d build ];
    then
        mkdir build
        mkdir build/lib
    fi
}

function compile_one_third_library() {
    name=$1
    source_path=$2
    build_path=$3
    other_args=$4

    prefix_path=${build_path}/${name}
    build_path=${prefix_path}/src/${name}-build
    cmake ${source_path} -B ${build_path} -DCMAKE_INSTALL_PREFIX=${prefix_path} -DCMAKE_INSTALL_LIBDIR=${prefix_path}/lib -DCMAKE_CXX_FLAGS_RELEASE=-O2 ${other_args}
    dd=$(pwd)
    echo ">>>>> in ${dd}"
    cd ${build_path}; make -j ${MAKE_JOB_NUM}; make install; cd -
    dd=$(pwd)
    echo ">>>>> out ${dd}"
}

function build_all_third_libraries() {

    ORIGINAL_BUILD_PATH=$1
    compile_one_third_library binancecpp ../3rdparty/binancecpp ${ORIGINAL_BUILD_PATH}
    compile_one_third_library spdlog ../3rdparty/spdlog ${ORIGINAL_BUILD_PATH}
    compile_one_third_library zeromq ../3rdparty/zeromq ${ORIGINAL_BUILD_PATH} "-D WITH_PERF_TOOL=OFF -D ZMQ_BUILD_TESTS=OFF -D ENABLE_CPACK=OFF -D CMAKE_BUILD_TYPE=Release"
    compile_one_third_library protobuf ../3rdparty/protobuf/cmake ${ORIGINAL_BUILD_PATH} "-Dprotobuf_BUILD_TESTS:BOOL=OFF -Dprotobuf_WITH_ZLIB:BOOL=OFF -Dprotobuf_MSVC_STATIC_RUNTIME:BOOL=OFF -DCMAKE_CXX_STANDARD=20"
    # compile_one_third_library abseil-cpp ../3rdparty/abseil-cpp ${ORIGINAL_BUILD_PATH} "-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20"

    # if [[ ${OS_UNAME} == "Darwin" ]]; then
    #     libtool -static -o lib/libcombined_absl.a protobuf/lib/libabsl*.a
    # else
    #     ar crsT lib/libcombined_absl.a protobuf/lib/libabsl*.a
    # fi

    # PROTOBUF_PROTOC=${BUILD_PATH}/protobuf/bin/protoc
    # PROTOBUF_CONFIG_DIR=${BUILD_PATH}/protobuf/lib/cmake/protobuf
    # if [ -d ${BUILD_PATH}/protobuf/lib64/cmake/protobuf ]; 
    # then
    #     PROTOBUF_CONFIG_DIR=${BUILD_PATH}/protobuf/lib64/cmake/protobuf
    # fi
}

BUILD_TYPE="all"
if [ $# -eq 1 ];
then
    BUILD_TYPE=$1
else
    echo "Usage: ./build.sh [all|third|main]"
    exit 0
fi

prepare
cd build
build_path=$(pwd)

BUILD_SHARED_LIBS=OFF # maybe link error if no this setting

if [[ ${BUILD_TYPE} == "third" || ${BUILD_TYPE} == "all" ]]; then
    build_all_third_libraries ${build_path}
fi

if [[ ${BUILD_TYPE} == "all" || ${BUILD_TYPE} == "main" ]]; then
    cmake ..
    make
fi
