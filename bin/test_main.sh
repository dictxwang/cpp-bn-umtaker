#!/usr/bin/env bash

PROCESS_NAME=test_main
PID_FILE=test_main.pid

ACTION=$1
SUB_NAME=$2
CONFIG_FILE=$3
CPU_NO=$4

if [[ -n ${SUB_NAME} && ${SUB_NAME} != "none" ]]; then
    PID_FILE=${SUB_NAME}_${PID_FILE}
fi
PID_FILE=bin/${PID_FILE}

PROJECT_ROOT=$(cd `dirname $0`; cd ..; pwd;)

if [[ ${ACTION} == "start" ]]; then
    ${PROJECT_ROOT}/bin/catalina.sh start ${PROCESS_NAME} ${PID_FILE} ${CONFIG_FILE} ${CPU_NO}
elif [[ ${ACTION} == "stop" ]]; then
    ${PROJECT_ROOT}/bin/catalina.sh stop ${PROCESS_NAME} ${PID_FILE}
elif [[ ${ACTION} == "pause" ]]; then
    ${PROJECT_ROOT}/bin/catalina.sh pause ${PROCESS_NAME} ${PID_FILE} 
elif [[ ${ACTION} == "resume" ]]; then
    ${PROJECT_ROOT}/bin/catalina.sh resume ${PROCESS_NAME} ${PID_FILE} 
elif [[ ${ACTION} == "status" ]]; then
    ${PROJECT_ROOT}/bin/catalina.sh status ${PROCESS_NAME} ${PID_FILE} 
  s=$?
  if [[ $s == 1 ]];then
    echo "process '${PROCESS_NAME}' is running."
  else
    echo "process '${PROCESS_NAME}' had stopped."
  fi
fi
