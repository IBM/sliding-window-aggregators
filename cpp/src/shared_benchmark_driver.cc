#include "benchmark_core.h"

typedef uint64_t timestamp;

int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "error: wrong number of program options; " << argc << std::endl;
        return 1;
    }

    std::string aggregator(argv[1]);
    std::string function(argv[2]);
    std::string window_size_big_str(argv[3]);
    std::string window_size_small_str(argv[4]);
    std::string iterations_str(argv[5]);
    bool twin = false;

    if (std::string(argv[6]) == "twin") {
        twin = true;
    }
    else if (std::string(argv[6]) == "range") {
        twin = false;
    }
    else {
        std::cerr << "expecting `twin` or `range` as last option, got: " << argv[6] << std::endl;
        return 3;
    }

    std::stringstream ss;
    size_t window_size_big;
    ss << window_size_big_str;
    ss >> window_size_big;
    ss.clear();

    size_t window_size_small;
    ss << window_size_small_str;
    ss >> window_size_small;
    ss.clear();

    size_t iterations;
    ss << iterations_str;
    ss >> iterations;

    std::cout << aggregator << ", " << function << std::endl;

    std::vector<cycle_duration> latencies;
    SharingExperiment exp(window_size_big, window_size_small, window_size_small/2, iterations, twin);

    if (!(query_call_sharing_benchmark<btree::MakeAggregate, timestamp, 2, btree::finger>("bfinger2", aggregator, function, exp) ||
          query_call_sharing_benchmark<btree::MakeAggregate, timestamp, 4, btree::finger>("bfinger4", aggregator, function, exp) ||
          query_call_sharing_benchmark<btree::MakeAggregate, timestamp, 8, btree::finger>("bfinger8", aggregator, function, exp) ||
          query_call_sharing_benchmark<btree::MakeAggregate, timestamp, 16, btree::finger>("bfinger16", aggregator, function, exp) ||
          query_call_sharing_benchmark<btree::MakeAggregate, timestamp, 32, btree::finger>("bfinger32", aggregator, function, exp) ||
          query_call_sharing_benchmark<btree::MakeAggregate, timestamp, 64, btree::finger>("bfinger64", aggregator, function, exp) ||
          query_call_sharing_benchmark<btree::MakeAggregate, timestamp, 128, btree::finger>("bfinger128", aggregator, function, exp)

          // removed daba because we changed the benchmark to have an out-of-order distance; we can put it back in if we 
          // ever change the experiment back
          // query_call_sharing_benchmark<daba::MakeAggregate, false>("daba", aggregator, function, exp)
       )) {
        std::cerr << "error: no matching kind of experiment: " << aggregator << ", " << function << std::endl;
        return 2;
    }

    return 0;
}
