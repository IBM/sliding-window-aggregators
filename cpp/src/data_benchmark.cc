#include <malloc.h>
#include "benchmark_core.h"

#include "utils.h"

#include "DataGenerators.h"
#include "TimestampedTwoStacks.hpp"
#include "TimestampedTwoStacksLite.hpp"
#include "TimestampedImplicitTwoStacksLite.hpp"
#include "TimestampedDynamicFlatFIT.hpp"
#include "TimestampedDABA.hpp"
#include "TimestampedDABALite.hpp"
#include "TimestampedFifo.hpp"

#include <vector>
#include <sstream>

template <typename DataSet>
void call_benchmarks(std::ifstream& in, int samples, const std::string& data_set, const std::string& data_file) {

    FileDataGenerator<DataSet> gen(data_file, DataSet::skip_lines());
    gen.load();

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string aggregator;
        std::string function;
        std::string window_duration_str;
        ss >> aggregator >> function >> window_duration_str;
        if (!ss) {
            std::cerr << "error reading line from config file: " << line << std::endl;
            throw std::invalid_argument(line);
        }

        std::string latency_str;
        ss >> latency_str;
        bool latency = false;
        if (!ss.fail()) {
            latency = true;
        }

        int window_duration(from_string<int>(window_duration_str));

        std::vector<cycle_duration> latencies;
        std::vector<uint32_t> evictions;
        DataExperiment exp(std::chrono::milliseconds(window_duration), latency, latencies, evictions);

        std::string result_file = "results/" + data_set + "_data_" + aggregator + ".csv";
        std::ofstream out(result_file, std::ios::app);
        if (!out) {
            std::cerr << "could not open " + result_file << std::endl;
            throw std::invalid_argument(result_file);
        }
        out << function << "," << window_duration;

        for (int i = 0; i < samples; ++i) {
            // we run this before the benchmark, so every run has a clean heap.
            // glibc magic: to force malloc to release free memory back to the OS.
            // the argument 0 is the amount of padding we want to leave in the heap,
            // and 0 means minimal padding.
            int trim_result = malloc_trim(0);
            // the return value is 1 if it has cleaned something and 0 otherwise.
            std::cout << "malloc_trim: " << trim_result << std::endl;
            // end glibc magic

            std::cout << i << " " << aggregator << " " << function << " " << window_duration << std::endl;
            if (!(query_call_data_benchmark<DataSet, btree::MakeAggregate, 2, btree::finger>("bfinger2", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, btree::MakeAggregate, 4, btree::finger>("bfinger4", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, btree::MakeAggregate, 8, btree::finger>("bfinger8", aggregator, function, exp, gen, out) ||

                  query_call_data_benchmark<DataSet, btree::MakeAggregate, 2, btree::classic>("bclassic2", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, btree::MakeAggregate, 4, btree::classic>("bclassic4", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, btree::MakeAggregate, 8, btree::classic>("bclassic8", aggregator, function, exp, gen, out) ||

                  query_call_data_benchmark<DataSet, timestamped_twostacks::MakeAggregate>("two_stacks", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, timestamped_twostacks_lite::MakeAggregate>("two_stacks_lite", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, timestamped_rb_twostackslite::MakeAggregate>("rb_two_stacks_lite", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, timestamped_chunked_twostackslite::MakeAggregate>("chunked_two_stacks_lite", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, timestamped_dynamic_flatfit::MakeAggregate>("flatfit", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, timestamped_daba::MakeAggregate, false>("daba", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, timestamped_rb_dabalite::MakeAggregate, 5*1024*1024, 0x42>("rb_daba_lite", aggregator, function, exp, gen, out) ||
                  query_call_data_benchmark<DataSet, timestamped_dabalite::MakeAggregate>("daba_lite", aggregator, function, exp, gen, out)
                  )) {
                std::cerr << "error: no matching kind of experiment: " << aggregator << ", " << function << std::endl;
                throw std::invalid_argument(aggregator + "," + function);
            }
        }

        std::cout << std::endl;
        out << std::endl;

        if (latency) {
            std::ofstream latency_out("results/latency_" + data_set + "_data_" + aggregator + "_" + function + "_w" + std::to_string(window_duration) + ".txt");
            for (auto e: latencies) {
                latency_out << e << std::endl;
            }
        }
    }
}

int main(int argc, char** argv) {
    // The system we run experiments on has its timezone set up such that mktime will shift times
    // we read from files by an hour. The following call counteracts that. See:
    //     https://stackoverflow.com/questions/3660983/c-time-t-problem
    std::string set_utc = "TZ=UTC";
    putenv(const_cast<char*>(set_utc.c_str()));

    if (argc != 5) {
        std::cerr << "error: wrong number of program options; " << argc << " provied, correct usage:" << std::endl
                  << "\tdata_benchmark config_file samples data_set data_file" << std::endl;
        return 1;
    }

    std::string config_file(argv[1]);
    int samples = from_string<int>(argv[2]);
    std::string data_set(argv[3]);
    std::string data_file(argv[4]);

    std::ifstream in(config_file, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Could not open " << config_file << std::endl;
        throw std::invalid_argument(config_file);
    }

    if (data_set == "taxi") {
        call_benchmarks<YellowTaxiCsv>(in, samples, data_set, data_file);
    }
    else if (data_set == "bike") {
        call_benchmarks<CitiBikeCsv>(in, samples, data_set, data_file);
    }
    else if (data_set == "mfgdebs") {
        call_benchmarks<MfgDEBS>(in, samples, data_set, data_file);
    }
    else {
        throw std::invalid_argument(data_set);
    }

    return 0;
}
