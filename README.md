# cpp-binance-umtaker

## Install Library
### On Linux Ubuntu
sudo apt install pkg-config
sudo apt install openssl libssl-dev
sudo apt-get install libcurl4-openssl-dev
sudo apt-get install -y libabsl-dev

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