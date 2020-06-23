#!/bin/bash

TARGETS="benchmark_driver benchmark_driver_stats ooo_adversary data_benchmark test dynamic_benchmark"
FQDN_IMAGE="swag-builder-cpp"
CWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# build the docker image if not present
[ ! -z $(docker images -q ${FQDN_IMAGE}) ]  || make -C ${CWD} builder-image

docker run -it \
       --user $(id -u):$(id -g) \
       -v ${CWD}:/stage \
       ${FQDN_IMAGE} \
       bash -c "cd /stage; make ${TARGETS} && make zip && ./bin/test"



