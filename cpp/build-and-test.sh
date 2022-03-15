#!/bin/bash

TARGETS="benchmark_driver benchmark_driver_stats ooo_benchmark data_benchmark test dynamic_benchmark bulk_evict_benchmark bulk_evict_insert_benchmark"
IMAGE="swag-builder-cpp"
CWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# only proceed if this directory is in the latest commit
cd ${CWD}/..
if $(cpp/can-skip.sh cpp); then
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

