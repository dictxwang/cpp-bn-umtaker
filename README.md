# cpp-binance-umtaker

## Install Library

#### On Linux Ubuntu
sudo apt install pkg-config
sudo apt install openssl libssl-dev
sudo apt-get install libcurl4-openssl-dev
~~sudo apt-get install -y libabsl-dev~~
sudo apt-get install libmysqlclient-dev

#### On MacOS
brew install pkg-config
brew install openssl
brew install curl
~~brew install abseil~~



## Link Binance SDK As 3rdparty Library

git submodule add https://github.com/dictxwang/binancecpp.git 3rdparty/binancecpp
git submodule update --init --recursive
#### Switch one tag version（no need）

git fetch --tags
git tag
git checkout v0.0.1

## Telegram Chat Link
https://web.telegram.org/a/#-4958151325



## Start Udp Ticker Processor

taskset -c 16 ./main_ws ./ws_taker_um.conf > /data/dc/ticker/taker_um.out
nohup ./udp_client udp_bn_uperp_client.conf > out.log



## About Debug

##### open coredump file output
ulimit -c unlimited

##### if use apport, the core dump files path is:
/var/lib/apport/coredump/

##### if not use apport, you should do:

1、make the core dump file generated in current path
echo "core_%e_%t_%p" > /proc/sys/kernel/core_pattern
2、stop apport service, avoid it overwrite your custom settings
systemctl status apport
sudo service apport stop
cat /etc/default/apport

## Start Process
```shell
bin/bn_umtaker_receiver.sh start main config/receiver_config_main.json 44
bin/bn_umtaker_actuary.sh start main config/actuary_config_main.json 0
bin/bn_umtaker_trader.sh start main config/trader_config_main.json 46
```

## Warning
the item "all_base_assets" in all configuration files must have same assets, to make sure shared memory index stable

## Additional

complie 3rd-party library command

```shell
cmake ../3rdparty/binancecpp -B ./binancecpp/src/binancecpp-build -DCMAKE_INSTALL_PREFIX=./binancecpp -DCMAKE_INSTALL_LIBDIR=./binancecpp/lib -DCMAKE_CXX_FLAGS_RELEASE=-O2
cd ./binancecpp/src/binancecpp-build; make -j 1; make install; cd -

cmake ../3rdparty/spdlog -B ./spdlog/src/spdlog-build -DCMAKE_INSTALL_PREFIX=./spdlog -DCMAKE_INSTALL_LIBDIR=./spdlog/lib -DCMAKE_CXX_FLAGS_RELEASE=-O2
cd ./spdlog/src/spdlog-build; make -j 1; make install; cd -

cmake ../3rdparty/zeromq -B ./zeromq/src/zeromq-build -DCMAKE_INSTALL_PREFIX=./zeromq -DCMAKE_INSTALL_LIBDIR=./zeromq/lib -DCMAKE_CXX_FLAGS_RELEASE=-O2 "-D WITH_PERF_TOOL=OFF -D ZMQ_BUILD_TESTS=OFF -D ENABLE_CPACK=OFF -D CMAKE_BUILD_TYPE=Release"
cd ./zeromq/src/zeromq-build; make -j 1; make install; cd -

cmake ../3rdparty/protobuf -B ./protobuf/src/protobuf-build -DCMAKE_INSTALL_PREFIX=./protobuf -DCMAKE_INSTALL_LIBDIR=./protobuf/lib -DCMAKE_CXX_FLAGS_RELEASE=-O2 "-Dprotobuf_BUILD_TESTS:BOOL=OFF -Dprotobuf_WITH_ZLIB:BOOL=OFF -Dprotobuf_MSVC_STATIC_RUNTIME:BOOL=OFF -Dprotobuf_ABSL_PROVIDER=cmake -DCMAKE_CXX_STANDARD=20"
cd ./protobuf/src/protobuf-build; make -j 1; make install; cd -
```

