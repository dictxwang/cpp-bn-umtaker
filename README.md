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

taskset -c 16 ./main_ws ./ws_test.conf > /data/dc/ticker/test.nohup



## About Debug

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