cmake_minimum_required(VERSION 3.10)

project(binance_umtaker VERSION 0.1.0 LANGUAGES CXX)

set(PROCESS_MAIN "test_main")
set(PROCESS_TICKER_RECEIVER "bn_umtaker_receiver")
set(PROCESS_OPPORTUNITY_ACTUARY "bn_umtaker_actuary")
set(PROCESS_ORDER_TRADER "bn_umtaker_trader")
set(PROCESS_PNL_EVALUATOR "bn_umtaker_evaluator")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE "Release")

add_compile_options(-fPIC)
add_compile_options(-O2)

include(dependencies.cmake)

include(protoc.cmake)

file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.cc
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    include
    src
    3rdparty/binancecpp/include
    3rdparty/spdlog/include
    3rdparty/zeromq/include
    3rdparty/protobuf/src
    # 3rdparty/protobuf/third_party/abseil-cpp/
    ${CUSTOM_INCLUDE_DIR}
)

# common
FILE(GLOB_RECURSE LIB_COMMON_SRC RELATIVE ${CMAKE_SOURCE_DIR}
    "src/common/*")
add_library(common ${LIB_COMMON_SRC})

# config
FILE(GLOB_RECURSE LIB_CONFIG_SRC RELATIVE ${CMAKE_SOURCE_DIR}
    "src/config/*")
add_library(config ${LIB_CONFIG_SRC})

# tgbot
FILE(GLOB_RECURSE LIB_TGBOT_SRC RELATIVE ${CMAKE_SOURCE_DIR}
    "src/tgbot/*")
add_library(tgbot ${LIB_TGBOT_SRC})

# shm
FILE(GLOB_RECURSE LIB_SHM_SRC RELATIVE ${CMAKE_SOURCE_DIR}
    "src/shm/*")
add_library(shm ${LIB_SHM_SRC})

# dbsource
FILE(GLOB_RECURSE LIB_DBSOURCE_SRC RELATIVE ${CMAKE_SOURCE_DIR}
    "src/dbsource/*")
add_library(dbsource ${LIB_DBSOURCE_SRC})

# receiver
FILE(GLOB_RECURSE LIB_RECEIVER_SRC RELATIVE ${CMAKE_SOURCE_DIR}
    "src/receiver/*")
add_library(receiver ${LIB_RECEIVER_SRC})
target_link_libraries(receiver
    "${BUILD_PATH}/libproject_types_proto.a"

    ${_LIB_BINANCECPP}
    ${_LIB_SPDLOG}
    ${_LIB_ZMQ}
    ${_PROTOBUF_LIBPROTOBUF}
    CURL::libcurl
    OpenSSL::SSL OpenSSL::Crypto
    common
    config
    shm
)

# actuary
FILE(GLOB_RECURSE LIB_ACTUARY_SRC RELATIVE ${CMAKE_SOURCE_DIR}
    "src/actuary/*")
add_library(actuary ${LIB_ACTUARY_SRC})
target_link_libraries(actuary

    ${_LIB_BINANCECPP}
    ${_LIB_SPDLOG}
    ${_LIB_MYSQL}
    ${_LIB_ZMQ}
    # ${_PROTOBUF_LIBPROTOBUF}
    CURL::libcurl
    OpenSSL::SSL OpenSSL::Crypto
    common
    config
    shm
    tgbot
    dbsource
)

# trader
FILE(GLOB_RECURSE LIB_TRADER_SRC RELATIVE ${CMAKE_SOURCE_DIR}
    "src/trader/*")
add_library(trader ${LIB_TRADER_SRC})
target_link_libraries(trader

    ${_LIB_BINANCECPP}
    ${_LIB_SPDLOG}
    ${_LIB_ZMQ}
    CURL::libcurl
    OpenSSL::SSL OpenSSL::Crypto
    common
    config
    shm
)

# evaluator
FILE(GLOB_RECURSE LIB_EVALUATOR_SRC RELATIVE ${CMAKE_SOURCE_DIR}
    "src/evaluator/*")
add_library(evaluator ${LIB_EVALUATOR_SRC})
target_link_libraries(evaluator

    ${_LIB_BINANCECPP}
    ${_LIB_SPDLOG}
    ${_LIB_MYSQL}
    ${_LIB_ZMQ}
    CURL::libcurl
    OpenSSL::SSL OpenSSL::Crypto
    common
    config
    dbsource
)


add_executable(${PROCESS_MAIN} "src/main.cpp")
target_link_libraries(${PROCESS_MAIN}
    # "-framework CoreFoundation"  # time_internal::cctz::local_time_zone()
    "${BUILD_PATH}/libproject_types_proto.a"

    ${_LIB_BINANCECPP}
    ${_LIB_SPDLOG}
    ${_LIB_ZMQ}
    ${_PROTOBUF_LIBPROTOBUF}
    # ${_LIB_ABSL}
    
    CURL::libcurl
    OpenSSL::SSL OpenSSL::Crypto
    config
    common
)

add_executable(${PROCESS_TICKER_RECEIVER} "src/bn_umtaker_receiver.cpp")
target_link_libraries(${PROCESS_TICKER_RECEIVER}
#     "${BUILD_PATH}/libproject_types_proto.a"

#     ${_LIB_BINANCECPP}
#     ${_LIB_SPDLOG}
#     ${_LIB_ZMQ}
#     ${_PROTOBUF_LIBPROTOBUF}
#     CURL::libcurl
#     OpenSSL::SSL OpenSSL::Crypto
#     # common
#     config
    receiver
)

add_executable(${PROCESS_OPPORTUNITY_ACTUARY} "src/bn_umtaker_actuary.cpp")
target_link_libraries(${PROCESS_OPPORTUNITY_ACTUARY}
    actuary
)

add_executable(${PROCESS_ORDER_TRADER} "src/bn_umtaker_trader.cpp")
target_link_libraries(${PROCESS_ORDER_TRADER}
    trader
)

add_executable(${PROCESS_PNL_EVALUATOR} "src/bn_sumtaker_evaluator.cpp")
target_link_libraries(${PROCESS_PNL_EVALUATOR}
    evaluator
)