# cpp-binance-umtaker

## Install Library
### On Linux Ubuntu
sudo apt install pkg-config
sudo apt install openssl libssl-dev
sudo apt-get install libcurl4-openssl-dev
sudo apt-get install -y libabsl-dev
sudo apt-get install libmysqlclient-dev

### On MacOS
brew install pkg-config
brew install openssl
brew install curl
brew install abseil


## Link 3rdparty Library
git submodule add https://github.com/dictxwang/binancecpp.git 3rdparty/binancecpp
git submodule update --init --recursive
### Switch one tag version
git fetch --tags
git tag
git checkout v0.0.1

# tg chat
https://web.telegram.org/a/#-4958151325

# start udp ticker test processor
taskset -c 16 ./main_ws ./ws_test.conf > /data/dc/ticker/test.nohup


# debug
# if use apport, the core dump files path is:
/var/lib/apport/coredump/

# if not use apport, you should do:

# 1 make the core dump file generated in current path
echo "core_%e_%t_%p" > /proc/sys/kernel/core_pattern
# 2 stop apport service, avoid it overwrite your custom settings
systemctl status apport
sudo service apport stop
cat /etc/default/apport