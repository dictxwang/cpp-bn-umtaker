#!/usr/bin/env bash

PROJECT_ROOT=$(cd `dirname $0`; cd ..; pwd;)
cd ${PROJECT_ROOT}

cd 3rdparty/binancecpp/
git checkout main
git pull

cd -
git pull
git submodule update --init --recursive