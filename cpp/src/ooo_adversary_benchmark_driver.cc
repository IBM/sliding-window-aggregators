#include "benchmark_core.h"
#include "utils.h"

typedef uint64_t timestamp;

int main(int argc, char** argv) {
    if (argc != 6 && argc != 7) {
        std::cerr << "error: wrong number of program options; " << argc << std::endl;
        return 1;
    }

    std::string aggregator(argv[1]);
    std::string function(argv[2]);
    std::string window_size_str(argv[3]);
    std::string degree_str(argv[4]);
    std::string iterations_str(argv[5]);
    bool latency = false;

    if (argc == 7 && std::string(argv[6]) == "latency") {
        latency = true;
    }

    size_t window_size = from_string<int>(window_size_str);
    size_t degree = from_string<int>(degree_str);
    size_t iterations = from_string<int>(iterations_str);

    std::cout << aggregator << ", " << function << ", " << window_size << ", " << degree << ", " << iterations << std::endl;

    std::vector<cycle_duration> latencies;
    Experiment exp(window_size, iterations, degree, latency, latencies);

    if (!(query_call_ooo_adversary_benchmark<btree::MakeAggregate, timestamp, 2, btree::finger>("bfinger2", aggregator, function, exp) ||
          query_call_ooo_adversary_benchmark<btree::MakeAggregate, timestamp, 4, btree::finger>("bfinger4", aggregator, function, exp) ||
          query_call_ooo_adversary_benchmark<btree::MakeAggregate, timestamp, 8, btree::finger>("bfinger8", aggregator, function, exp) ||

          query_call_ooo_adversary_benchmark<btree::MakeAggregate, timestamp, 2, btree::classic>("bclassic2", aggregator, function, exp) ||
          query_call_ooo_adversary_benchmark<btree::MakeAggregate, timestamp, 4, btree::classic>("bclassic4", aggregator, function, exp) ||
          query_call_ooo_adversary_benchmark<btree::MakeAggregate, timestamp, 8, btree::classic>("bclassic8", aggregator, function, exp) ||

          query_call_ooo_adversary_benchmark<btree::MakeAggregate, timestamp, 2, btree::knuckle>("bknuckle2", aggregator, function, exp) ||
          query_call_ooo_adversary_benchmark<btree::MakeAggregate, timestamp, 4, btree::knuckle>("bknuckle4", aggregator, function, exp) ||
          query_call_ooo_adversary_benchmark<btree::MakeAggregate, timestamp, 8, btree::knuckle>("bknuckle8", aggregator, function, exp)
       )) {
        std::cerr << "error: no matching kind of experiment: " << aggregator << ", " << function << std::endl;
        return 2;
    }

    if (latency) {
        std::ofstream out("results/latency_adversary_" + aggregator + "_" + function + "_w" + window_size_str + "_d" + degree_str + ".txt");
        for (auto e: latencies) {
            out << e << std::endl;
        }
    }

    return 0;
}
