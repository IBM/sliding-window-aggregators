#include "benchmark_core.h"
#include "utils.h"

#include "AMTA.hpp"
#include "FiBA.hpp"
#include "TimestampedTwoStacksLite.hpp"
#include "TimestampedImplicitTwoStacksLite.hpp"
#include "TimestampedDABALite.hpp"

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
    std::vector<cycle_duration> evict_latencies;
    std::vector<cycle_duration> insert_latencies;

    Experiment exp(window_size, iterations, degree, bulk_size, latency, latencies);
    exp.extra_latencies.push_back(evict_latencies);
    exp.extra_latencies.push_back(insert_latencies);

    if (!(query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 2, btree::finger>("bfinger2", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 4, btree::finger>("bfinger4", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeAggregate, timestamp, 8, btree::finger>("bfinger8", aggregator, function, exp) ||
          
          query_call_bulk_evict_insert_benchmark<btree::MakeBulkAggregate, timestamp, 2, btree::finger>("nbfinger2", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeBulkAggregate, timestamp, 4, btree::finger>("nbfinger4", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<btree::MakeBulkAggregate, timestamp, 8, btree::finger>("nbfinger8", aggregator, function, exp) ||

          query_call_bulk_evict_insert_benchmark<timestamped_twostacks_lite::MakeBulkAggregate, timestamp>("two_stacks_lite", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<timestamped_chunked_twostackslite::MakeBulkAggregate, timestamp>("chunked_two_stacks_lite", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<timestamped_dabalite::MakeBulkAggregate, timestamp>("daba_lite", aggregator, function, exp) ||
          query_call_bulk_evict_insert_benchmark<amta::MakeAggregate, timestamp>("amta", aggregator, function, exp)
       )) {
        std::cerr << "error: no matching kind of experiment: " << aggregator << ", " << function << std::endl;
        return 2;
    }

    if (latency) {
        write_latency(
            "results/latency_bulk_evict_insert_" + aggregator + "_" + function + "_w" + window_size_str + "_d" + degree_str + "_b" + bulk_size_str +  ".txt",
            latencies
        );
        write_latency(
            "results/latency_bulk_evict_insert_opevict_" + aggregator + "_" + function + "_w" + window_size_str + "_d" + degree_str + "_b" + bulk_size_str +  ".txt",
            evict_latencies
        );
        write_latency(
            "results/latency_bulk_evict_insert_opinsert_" + aggregator + "_" + function + "_w" + window_size_str + "_d" + degree_str + "_b" + bulk_size_str +  ".txt",
            insert_latencies
        );
    }

    return 0;
}
