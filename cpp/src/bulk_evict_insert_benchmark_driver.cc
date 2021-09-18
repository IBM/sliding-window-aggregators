#include "benchmark_core.h"
#include "utils.h"

typedef uint64_t timestamp;

int main(int argc, char** argv) {
    if (argc != 7 && argc != 8) {
        std::cerr << "error: wrong number of program options; " << argc << std::endl;
        return 1;
    }

    std::string aggregator(argv[1]);
    std::string function(argv[2]);
    std::string window_size_str(argv[3]);
    std::string degree_str(argv[4]);
    std::string bulk_size_str(argv[5]);
    std::string iterations_str(argv[6]);
    bool latency = false;

    if (argc == 8 && std::string(argv[7]) == "latency") {
        latency = true;
    }

    uint64_t window_size = from_string<uint64_t>(window_size_str);
    uint64_t degree = from_string<uint64_t>(degree_str);
    uint64_t bulk_size = from_string<uint64_t>(bulk_size_str);
    uint64_t iterations = from_string<uint64_t>(iterations_str);

    std::cout << aggregator << ", " 
              << function << ", " 
              << window_size << ", " 
              << degree << ", " 
              << bulk_size << ", " 
              << iterations << std::endl;

    std::vector<cycle_duration> latencies;
    Experiment exp(window_size, iterations, degree, bulk_size, latency, latencies);

    if (!(query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 2, btree::finger>("bfinger2", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 4, btree::finger>("bfinger4", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 8, btree::finger>("bfinger8", aggregator, function, exp) ||

          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 2, btree::classic>("bclassic2", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 4, btree::classic>("bclassic4", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 8, btree::classic>("bclassic8", aggregator, function, exp) ||

          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 2, btree::knuckle>("bknuckle2", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 4, btree::knuckle>("bknuckle4", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 8, btree::knuckle>("bknuckle8", aggregator, function, exp)
       )) {
        std::cerr << "error: no matching kind of experiment: " << aggregator << ", " << function << std::endl;
        return 2;
    }

    if (latency) {
        std::ofstream out("results/latency_bulk_evict_" + aggregator + "_" + function + "_w" + window_size_str + "_d" + degree_str + ".txt");
        for (auto e: latencies) {
            out << e << std::endl;
        }
    }

    return 0;
}
