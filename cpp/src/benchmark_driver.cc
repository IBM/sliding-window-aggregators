#include "benchmark_core.h"

typedef uint64_t timestamp;

template <template <typename, typename time> class MakeAggregate, typename time>
bool query_call_benchmark(std::string aggregator,
                          std::string aggregator_req,
                          std::string function_req,
                          Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        benchmark(MakeAggregate<Sum<int>, time>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        benchmark(MakeAggregate<Max<int>, time>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        benchmark(MakeAggregate<Mean<int>, time>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        benchmark(MakeAggregate<SampleStdDev<int>, time>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        benchmark(MakeAggregate<BloomFilter<int>, time>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        benchmark(MakeAggregate<Collect<int>, time>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        benchmark(MakeAggregate<MinCount<int>, time>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        benchmark(MakeAggregate<GeometricMean<int>, time>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        benchmark(MakeAggregate<BusyLoop<int>, time>()(0), exp);
        return true;
    }
    return false;
}

int main(int argc, char** argv) {
    if (argc != 5 && argc != 6) {
        std::cerr << "error: wrong number of program options; " << argc << std::endl;
        return 1;
    }

    std::string aggregator(argv[1]);
    std::string function(argv[2]);
    std::string window_size_str(argv[3]);
    std::string iterations_str(argv[4]);
    bool latency = false;

    if (argc == 6 && std::string(argv[5]) == "latency") {
        latency = true;
    }

    size_t window_size;
    std::stringstream ss;
    ss << window_size_str;
    ss >> window_size;
    ss.clear();

    size_t iterations;
    ss << iterations_str;
    ss >> iterations;

    std::cout << aggregator << ", " << function << ", " << window_size << ", " << iterations << std::endl;

    std::vector<cycle_duration> latencies;
    Experiment exp(window_size, iterations, latency, latencies);

    if (!(query_call_benchmark<soe::MakeAggregate>("sub_evict", aggregator, function, exp) ||
          query_call_benchmark<daba::MakeAggregate, false>("daba", aggregator, function, exp) || // daba: no caching
          query_call_benchmark<daba::MakeAggregate, true>("daba_caching", aggregator, function, exp) || // daba: with caching
          query_call_benchmark<dabalite::MakeAggregate>("daba_lite", aggregator, function, exp) ||
          query_call_benchmark<aba::MakeAggregate>("aba", aggregator, function, exp) ||
          query_call_benchmark<twostacks::MakeAggregate>("two_stacks", aggregator, function, exp) ||
          query_call_benchmark<twostackslite::MakeAggregate>("two_stacks_lite", aggregator, function, exp) ||
          query_call_benchmark<dynamic_flatfit::MakeAggregate>("flatfit", aggregator, function, exp) ||
          query_call_benchmark<reactive::MakeAggregate>("reactive", aggregator, function, exp) ||
          query_call_benchmark<recalc::MakeAggregate>("recalc", aggregator, function, exp) ||
          query_call_benchmark<okasaki::MakeAggregate>("ioa", aggregator, function, exp) ||

          query_call_benchmark<btree::MakeAggregate, timestamp, 2, btree::classic>("bclassic2", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 4, btree::classic>("bclassic4", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 8, btree::classic>("bclassic8", aggregator, function, exp) ||

          query_call_benchmark<btree::MakeAggregate, timestamp, 2, btree::finger>("bfinger2", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 4, btree::finger>("bfinger4", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 8, btree::finger>("bfinger8", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 16, btree::finger>("bfinger16", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 32, btree::finger>("bfinger32", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 64, btree::finger>("bfinger64", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 128, btree::finger>("bfinger128", aggregator, function, exp) ||

          // knuckle experiments
          query_call_benchmark<btree::MakeAggregate, timestamp, 2, btree::knuckle>("bknuckle2", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 4, btree::knuckle>("bknuckle4", aggregator, function, exp) ||
          query_call_benchmark<btree::MakeAggregate, timestamp, 8, btree::knuckle>("bknuckle8", aggregator, function, exp)
       )) {
        std::cerr << "error: no matching kind of experiment: " << aggregator << ", " << function << std::endl;
        return 2;
    }

    if (latency) {
        std::ofstream out("results/latency_" + aggregator + "_" + function + "_w" + std::to_string(window_size) + ".txt");
        for (auto e: latencies) {
            out << e << std::endl;
        }
    }

    return 0;
}
