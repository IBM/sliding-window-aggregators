#!/bin/bash

TARGETS="benchmark_driver benchmark_driver_stats ooo_adversary data_benchmark test dynamic_benchmark"
IMAGE="swag-builder-cpp"
CWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# only proceed if this directory is in the latest commit
if [ $(${CWD}/can-skip.sh .) ]; then
    echo "Skipping build and test for C++ code."
    exit 0
fi

# build the docker image if not present
[ ! -z $(docker images -q ${IMAGE}) ]  || make -C ${CWD} builder-image

docker run -it \
       --user $(id -u):$(id -g) \
       -v ${CWD}:/stage \
       ${IMAGE} \
       bash -c "cd /stage; make ${TARGETS} && make zip && ./bin/test"

