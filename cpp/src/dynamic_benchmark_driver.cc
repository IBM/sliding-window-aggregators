#include "benchmark_core.h"

typedef uint64_t timestamp;

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "error: wrong number of program options; " << argc << std::endl;
        return 1;
    }

    std::string aggregator(argv[1]);
    std::string function(argv[2]);
    std::string window_size_str(argv[3]);
    std::string iterations_str(argv[4]);

    size_t window_size;
    std::stringstream ss;
    ss << window_size_str;
    ss >> window_size;
    ss.clear();

    size_t iterations;
    ss << iterations_str;
    ss >> iterations;

    std::cout << aggregator << ", " << function << ", " << window_size << ", " << iterations << std::endl;

    std::vector<cycle_duration> dummy;
    Experiment exp(window_size, iterations, false, dummy);

    if (!(query_call_dynamic_benchmark<daba::MakeAggregate, false>("daba", aggregator, function, exp) || // daba: no caching
          query_call_dynamic_benchmark<dabalite::MakeAggregate>("daba_lite", aggregator, function, exp) ||
          query_call_dynamic_benchmark<twostacks::MakeAggregate>("two_stacks", aggregator, function, exp) ||
          query_call_dynamic_benchmark<twostackslite::MakeAggregate>("two_stacks_lite", aggregator, function, exp) ||
          query_call_dynamic_benchmark<implicit_twostackslite::MakeAggregate>("im_two_stacks_lite", aggregator, function, exp) ||
          query_call_dynamic_benchmark<flatfit::MakeAggregate>("flatfit", aggregator, function, exp) ||
          query_call_dynamic_benchmark<dynamic_flatfit::MakeAggregate>("dynamic_flatfit", aggregator, function, exp) ||

          query_call_dynamic_benchmark<btree::MakeAggregate, timestamp, 2, btree::classic>("bclassic2", aggregator, function, exp) ||
          query_call_dynamic_benchmark<btree::MakeAggregate, timestamp, 4, btree::classic>("bclassic4", aggregator, function, exp) ||
          query_call_dynamic_benchmark<btree::MakeAggregate, timestamp, 8, btree::classic>("bclassic8", aggregator, function, exp) ||

          query_call_dynamic_benchmark<btree::MakeAggregate, timestamp, 2, btree::finger>("bfinger2", aggregator, function, exp) ||
          query_call_dynamic_benchmark<btree::MakeAggregate, timestamp, 4, btree::finger>("bfinger4", aggregator, function, exp) ||
          query_call_dynamic_benchmark<btree::MakeAggregate, timestamp, 8, btree::finger>("bfinger8", aggregator, function, exp) ||
          query_call_dynamic_benchmark<btree::MakeAggregate, timestamp, 16, btree::finger>("bfinger16", aggregator, function, exp) ||
          query_call_dynamic_benchmark<btree::MakeAggregate, timestamp, 32, btree::finger>("bfinger32", aggregator, function, exp)
       )) {
        std::cerr << "error: no matching kind of experiment: " << aggregator << ", " << function << std::endl;
        return 2;
    }

    return 0;
}
