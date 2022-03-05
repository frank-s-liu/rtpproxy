#!/bin/bash

trap "" HUP

pid=$(ps -ef | grep "/rtpproxy" | grep -v "grep" | awk '{print $2}') 
if [[ -n ${pid} ]];
then
    kill -9 ${pid}
    echo "kill legacy rtpproxy process ${pid}"
fi

rtpproxypath=$(dirname $0)

cd ${rtpproxypath}
cd ..

workpath=$(pwd);
cd ${rtpproxypath}

#valgrind --tool=memcheck --leak-check=full --log-file=./vg.txt --show-leak-kinds=all --error-limit=no ./rtpproxy --cwd="${pwddir}"
./rtpproxy --cwd="${workpath}"

echo "start rtpproxy"
