#!/bin/bash

#echo  "==== bulk evict ===="
#./run_bulk_evict.py
#echo  "==== bulk evict insert ===="
#./run_bulk_evict_insert.py
echo  "==== bulk evict (latency) ===="
./run_bulk_evict_latency.py
echo  "==== bulk evict insert (latency) ===="
./run_bulk_evict_insert_latency.py
echo  "==== bulk data ===="
./run_bulk_data.py
#echo  "==== bulk data (latency) ===="
#./run_bulk_data_latency.py

