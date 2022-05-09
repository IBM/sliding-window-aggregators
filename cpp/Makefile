UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	EXTRAS = 
else
	EXTRAS = -static
endif

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	EXTRAS = 
	CXXFLAGS=-std=gnu++17 -DHAS_UNCAUGHT_EXCEPTIONS=1 -g -fsanitize=address -fno-omit-frame-pointer ${EXTRAS}
else
	CXXFLAGS=-std=gnu++17 -DHAS_UNCAUGHT_EXCEPTIONS=1 -O3 -DNDEBUG ${EXTRAS}
endif

SWAG_ALGOS=src/ChunkedArrayQueue.hpp \
		   src/TwoStacks.hpp src/TwoStacksLite.hpp\
           src/ImplicitQueueABA.hpp src/DABA.hpp src/DABALite.hpp \
		   src/SubtractOnEvict.hpp \
           src/OkasakisQueue.hpp src/FiBA.hpp \
           src/TimestampedFifo.hpp \
		   src/AMTA.hpp

all: benchmark_driver \
	 benchmark_driver_stats \
	 bulk_evict_benchmark \
	 bulk_evict_insert_benchmark \
	 ooo_benchmark \
	 ooo_benchmark_stats \
	 data_benchmark \
	 dynamic_benchmark \
	 shared

bin:
	mkdir bin

zip: bin
	cd bin; zip -D benchmark_bin.zip *

log: bin/log

bin/log: bin
	g++ --version > bin/log.txt 2>&1

builder-image:
	docker build -t swag-builder-cpp builder

test: bin/test

bulk_test: bin/bulk_test

bin/test: log src/test.cc $(SWAG_ALGOS)
	$(CXX) -std=gnu++17 -DHAS_UNCAUGHT_EXCEPTIONS=1 -ggdb src/test.cc -o bin/test

bin/bulk_test: log src/bulk_test.cc $(SWAG_ALGOS)
	$(CXX) -std=gnu++17 -DHAS_UNCAUGHT_EXCEPTIONS=1 -ggdb src/bulk_test.cc -o $@

bin/amta_test: src/amta_test.cc $(SWAG_ALGOS)
	$(CXX) -std=gnu++17 -ggdb -DHAS_UNCAUGHT_EXCEPTIONS=1 -ggdb src/amta_test.cc -o $@

benchmark_driver: log src/benchmark_driver.cc src/benchmark_core.h $(SWAG_ALGOS)
	$(CXX) $(CXXFLAGS) src/benchmark_driver.cc -o bin/benchmark_driver

benchmark_driver_stats: log src/benchmark_driver.cc src/benchmark_core.h $(SWAG_ALGOS)
	$(CXX) $(CXXFLAGS) -DCOLLECT_STATS src/benchmark_driver.cc -o bin/benchmark_driver_stats

ooo_benchmark: log src/ooo_benchmark_driver.cc src/benchmark_core.h src/FiBA.hpp
	$(CXX) $(CXXFLAGS) src/ooo_benchmark_driver.cc -o bin/ooo_benchmark_driver

bulk_evict_benchmark: log src/bulk_evict_benchmark_driver.cc src/benchmark_core.h src/FiBA.hpp
	$(CXX) $(CXXFLAGS) src/bulk_evict_benchmark_driver.cc -o bin/bulk_evict_benchmark_driver

bulk_evict_insert_benchmark: log src/bulk_evict_insert_benchmark_driver.cc src/benchmark_core.h src/FiBA.hpp
	$(CXX) $(CXXFLAGS) src/bulk_evict_insert_benchmark_driver.cc -o bin/bulk_evict_insert_benchmark_driver

ooo_benchmark_stats: log src/ooo_benchmark_driver.cc src/benchmark_core.h src/FiBA.hpp
	$(CXX) $(CXXFLAGS) -DCOLLECT_STATS src/ooo_benchmark_driver.cc -o bin/ooo_benchmark_driver_stats

data_benchmark: log src/data_benchmark.cc src/benchmark_core.h src/DataGenerators.h
	$(CXX) $(CXXFLAGS) src/data_benchmark.cc -o bin/data_benchmark

dynamic_benchmark: log src/benchmark_driver.cc src/benchmark_core.h $(SWAG_ALGOS)
	$(CXX) $(CXXFLAGS) src/dynamic_benchmark_driver.cc -o bin/dynamic_benchmark_driver

shared: log src/shared_benchmark_driver.cc src/benchmark_core.h src/FiBA.hpp
	$(CXX) $(CXXFLAGS) src/shared_benchmark_driver.cc -o bin/shared_benchmark_driver


