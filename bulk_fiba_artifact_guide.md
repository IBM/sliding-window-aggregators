## Bulk FiBA Artifact Guide

There are a few relevant folders:
* `cpp/src/` - all C++ source files, importantly `FiBA.hpp` and `AMTA.hpp`.
* `cpp/` - the `Makefile` to build the benchmark drivers.
* `experiments/` - helper scripts to run and process data from experiments.
    

### Compilation Instructions
The code can be compiled using just `make`, a recent `g++`, and a recent `libboost` installation. Our experiments took advantage of `mimalloc`; this is *not* necessary but can deliver lower latency variability in memory management calls.

To compile the code, change into `cpp/` and run `make` like so:

```bash
make bulk_evict_benchmark bulk_evict_insert_benchmark bulk_data_benchmark
```
This will build the relevant binaries and store them in `cpp/bin/`.

*Optional:* If `mimalloc` is desired, follow the instructions in `README.md` therein.

### Running the experiments and parsing the results

The scripts in `experiments/` that pertain to bulk operations match the pattern `run_bulk*.py`, all written in Python 3. An example of how all these are called can be found in `run_all_bulk.sh` in the same folder. These scripts store experimental results in `experiments/results`.  

For real-world data experiments, datasets are expected in the folder `experiments/data/`.  The accompanying paper uses the NYC Citi Bike dataset. Once the data is downloaded and concatenated according to the instructions in the paper, it is stored as `experiments/data/NYC-Citi-Bike/catted-citibike-tripdata.csv`. 

The processing scripts to parse the results are `process_bulk*.py`. They read from `experiments/results`.  These scripts expect `pandas`, `matplotlib` and `numpy`, and generate figures in `figures/`.
