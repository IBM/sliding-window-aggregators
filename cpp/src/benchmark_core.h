#ifndef _BENCHMARK_CORE_H
#define _BENCHMARK_CORE_H

#include <functional>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <atomic>
#include <chrono>
#include <iterator>

#include "AggregationFunctions.hpp"

#include "SubtractOnEvict.hpp"
#include "FiBA.hpp"

#include "utils.h"

template <typename T>
struct IdentityLifter {
    T operator()(const T& other) const {
        return other;
    }
};

template <typename T>
void silly_combine(T& a, const T& b) {
    a += b;
}

template <typename T>
void silly_combine(std::list<T>& a, const std::list<T>& b) {
    a.front() += b.back();
}

template <>
void silly_combine(std::chrono::system_clock::time_point& a, const std::chrono::system_clock::time_point& b) {
    auto dur = a - b;
    a += dur;
}

struct Experiment {
    size_t window_size;
    uint64_t iterations;
    uint64_t ooo_distance;
    uint64_t bulk_size;
    bool latency;
    std::vector<cycle_duration>& latencies;
    std::vector<std::reference_wrapper<std::vector<cycle_duration>>> extra_latencies;

    Experiment(size_t w, uint64_t i, bool l, std::vector<cycle_duration>& ls):
        window_size(w), iterations(i), ooo_distance(0), bulk_size(1), latency(l), latencies(ls),
        extra_latencies()
    {}

    Experiment(size_t w, uint64_t i, uint64_t d, bool l, std::vector<cycle_duration>& ls):
        window_size(w), iterations(i), ooo_distance(d), bulk_size(1), latency(l), latencies(ls),
        extra_latencies()
    {}

    Experiment(size_t w, uint64_t i, uint64_t d, uint64_t b, bool l, std::vector<cycle_duration>& ls):
        window_size(w), iterations(i), ooo_distance(d), bulk_size(b), latency(l), latencies(ls),
        extra_latencies()
    {}

};

struct SharingExperiment {
    size_t window_size_big;
    size_t window_size_small;
    uint64_t ooo_distance;
    uint64_t iterations;
    bool twin;

    SharingExperiment(size_t wb, size_t ws, uint64_t d, uint64_t i, bool t):
        window_size_big(wb), window_size_small(ws), ooo_distance(d), iterations(i), twin(t)
    {}
};

struct DataExperiment {
    std::chrono::milliseconds window_duration;
    bool latency;
    std::vector<cycle_duration>& latencies;
    std::vector<uint32_t>& evictions;

    DataExperiment(const std::chrono::milliseconds& wd, 
                   bool l, 
                   std::vector<cycle_duration>& ls,
                   std::vector<uint32_t>& e):
        window_duration(wd), latency(l), latencies(ls), evictions(e)
    {}
};

template <typename Aggregate>
void benchmark(Aggregate agg, Experiment exp) {
    typename Aggregate::outT force_side_effect = typename Aggregate::outT();
    if (!exp.latency) {
        for (uint64_t i = 0; i < exp.window_size; ++i) {
            agg.insert(1 + (i % 101));
        }

        if (agg.size() != exp.window_size) {
            std::cerr << "window is not exactly full; should be " << exp.window_size << ", but is " << agg.size();
            exit(2);
        }

        auto start = std::chrono::system_clock::now();

        for (uint64_t i = exp.window_size; i < exp.iterations; ++i) {
            std::atomic_thread_fence(std::memory_order_seq_cst);

            agg.evict();
            agg.insert(1 + (i % 101));
            silly_combine(force_side_effect, agg.query());
        }
        std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
        std::cout << "core runtime: " << runtime.count() << std::endl;
        std::cerr << force_side_effect << std::endl;
    }
    else {
        for (uint64_t i = 0; i < exp.window_size; ++i) {
            agg.insert(1 + (i % 101));
        }

        if (agg.size() != exp.window_size) {
            std::cerr << "window is not exactly full; should be " << exp.window_size << ", but is " << agg.size();
            exit(2);
        }

        for (uint64_t i = exp.window_size; i < exp.iterations; ++i) {
            std::atomic_thread_fence(std::memory_order_seq_cst);

            auto before = rdtsc(); // std::chrono::high_resolution_clock::now();

            agg.evict();
            agg.insert(1 + (i % 101));
            silly_combine(force_side_effect, agg.query());

            std::atomic_thread_fence(std::memory_order_seq_cst);
            auto after = rdtsc(); // std::chrono::high_resolution_clock::now();
            exp.latencies.push_back(after - before);
        }
        std::cerr << force_side_effect << std::endl;
    }
}

template <typename Aggregate>
void dynamic_benchmark(Aggregate agg, Experiment exp) {
    typename Aggregate::outT force_side_effect = typename Aggregate::outT();
    if (!exp.latency) {
        auto start = std::chrono::system_clock::now();

        uint64_t i = 0;
        while (i < exp.iterations) {

            for (uint64_t j = 0; j < exp.window_size; ++j) {
                std::atomic_thread_fence(std::memory_order_seq_cst);

                agg.insert(1 + (i % 101));
                silly_combine(force_side_effect, agg.query());
                ++i;
                if (i >= exp.iterations) {
                    break;
                }
            }

            while (agg.size() > 0) {
                std::atomic_thread_fence(std::memory_order_seq_cst);
                agg.evict();
            }
        }

        std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
        std::cout << "core runtime: " << runtime.count() << std::endl;
        std::cerr << force_side_effect << std::endl;
    }
    else {
        std::cerr << "latency is not implemented for doubling window" << std::endl;
        exit(2);
    }
}

template <typename Aggregate>
void ooo_benchmark(Aggregate agg, Experiment exp) {
    typename Aggregate::outT force_side_effect = typename Aggregate::outT();
    typename Aggregate::timeT i = 0;

    std::cout << "window size " << exp.window_size << ", iterations " << exp.iterations << std::endl;
    if (!exp.latency) {
        for (i = exp.iterations - exp.ooo_distance; i < exp.iterations; ++i) {
            agg.insert(i, 1 + (i % 101));
        }
        for (i = 0; i < exp.window_size - exp.ooo_distance; ++i) {
            agg.insert(i, 1 + (i % 101));
        }

        if (agg.size() != exp.window_size) {
            std::cerr << "window is not exactly full; should be " << exp.window_size << ", but is " << agg.size();
            exit(2);
        }

        auto start = std::chrono::system_clock::now();

        for (i = exp.window_size - exp.ooo_distance; i < exp.iterations - exp.ooo_distance; ++i) {
            std::atomic_thread_fence(std::memory_order_seq_cst);

            agg.evict();
            agg.insert(i, 1 + (i % 101));
            silly_combine(force_side_effect, agg.query());
        }

        std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
        std::cout << "core runtime: " << runtime.count() << std::endl;
        std::cerr << force_side_effect << std::endl;
    }
    else {
        for (i = exp.iterations - exp.ooo_distance; i < exp.iterations; ++i) {
            agg.insert(i, 1 + (i % 101));
        }
        for (i = 0; i < exp.window_size - exp.ooo_distance; ++i) {
            agg.insert(i, 1 + (i % 101));
        }

        if (agg.size() != exp.window_size) {
            std::cerr << "window is not exactly full; should be " << exp.window_size << ", but is " << agg.size();
            exit(2);
        }

        for (i = exp.window_size - exp.ooo_distance; i < exp.iterations - exp.ooo_distance; ++i) {
            std::atomic_thread_fence(std::memory_order_acquire);
            auto before = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);

            agg.evict();
            agg.insert(i, 1 + (i % 101));
            silly_combine(force_side_effect, agg.query());

            std::atomic_thread_fence(std::memory_order_acquire);
            auto after = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);
            exp.latencies.push_back(after - before);
        }
        std::cerr << force_side_effect << std::endl;
    }
}

template <typename Aggregate>
void bulk_evict_benchmark(Aggregate agg, Experiment exp) {
    typename Aggregate::outT force_side_effect = typename Aggregate::outT();
    typename Aggregate::timeT i = 0;

    std::cout << "window size " << exp.window_size 
              << ", iterations " << exp.iterations 
              << ", ooo " << exp.ooo_distance
              << ", bulk size " << exp.bulk_size
              << std::endl;
    if (!exp.latency) {
        for (i = exp.iterations - exp.ooo_distance; i < exp.iterations; ++i) {
            agg.insert(i, 1 + (i % 101));
        }
        for (i = 0; i < exp.window_size - exp.ooo_distance; ++i) {
            agg.insert(i, 1 + (i % 101));
        }

        if (agg.size() != exp.window_size) {
            std::cerr << "window is not exactly full; should be " << exp.window_size << ", but is " << agg.size();
            exit(2);
        }

        auto start = std::chrono::system_clock::now();

        uint64_t start_pos = exp.window_size - exp.ooo_distance;
        uint64_t stop_pos = exp.iterations - exp.ooo_distance;
        for (i = start_pos; i < stop_pos; /* nop */) {
            agg.bulkEvict(i - exp.window_size + exp.bulk_size - 1);

            for (typename Aggregate::timeT j = 0; j < exp.bulk_size && i < stop_pos; ++j, ++i) {
                std::atomic_thread_fence(std::memory_order_seq_cst);
                agg.insert(i, 1 + (i % 101));
            }
            silly_combine(force_side_effect, agg.query());
        }

        std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
        std::cout << "core runtime: " << runtime.count() << std::endl;
        std::cerr << force_side_effect << std::endl;
    }
    else {
        for (i = exp.iterations - exp.ooo_distance; i < exp.iterations; ++i) {
            agg.insert(i, 1 + (i % 101));
        }
        for (i = 0; i < exp.window_size - exp.ooo_distance; ++i) {
            agg.insert(i, 1 + (i % 101));
        }

        if (agg.size() != exp.window_size) {
            std::cerr << "window is not exactly full; should be " << exp.window_size << ", but is " << agg.size();
            exit(2);
        }

        uint64_t start_pos = exp.window_size - exp.ooo_distance;
        uint64_t stop_pos = exp.iterations - exp.ooo_distance;
        for (i = start_pos; i < stop_pos; /* nop */) {
            std::atomic_thread_fence(std::memory_order_acquire);
            auto before = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);

            agg.bulkEvict(i - exp.window_size + exp.bulk_size - 1);

            std::atomic_thread_fence(std::memory_order_acquire);
            auto evict = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);
            for (typename Aggregate::timeT j = 0; j < exp.bulk_size && i < stop_pos; ++j, ++i) {
                std::atomic_thread_fence(std::memory_order_seq_cst);
                agg.insert(i, 1 + (i % 101));
            }
            std::atomic_thread_fence(std::memory_order_acquire);
            auto insert = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);
            silly_combine(force_side_effect, agg.query());
            std::atomic_thread_fence(std::memory_order_acquire);
            auto after = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);
            exp.latencies.push_back(after - before);
            exp.extra_latencies[0].get().push_back(evict - before);
            exp.extra_latencies[1].get().push_back(insert - evict);
        }
        std::cerr << force_side_effect << std::endl;
    }
}

template<typename Timestamp, typename Value>
class SyntheticGenerator {
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = Timestamp;
    using value_type = std::pair<Timestamp, Value>;
    using pointer = value_type*;
    using reference = value_type&;

    SyntheticGenerator(const Timestamp& ts, const Value& val):
        _next(std::make_pair(ts, val))
    {}

    reference operator*() {
        return _next;
    }

    pointer operator->() {
        return &_next;
    }

    SyntheticGenerator& operator++() {
        ++_next.first;
        _next.second = 1 + (_next.second % 101);
        return *this;
    }

    SyntheticGenerator operator++(int) {
        SyntheticGenerator tmp = *this;
        ++(*this);
        return tmp;
    }

    // For equality purposes, we only care about the timestamp, not the value 
    // associated with that timestamp.
    friend bool operator==(const SyntheticGenerator& a, const SyntheticGenerator& b) {
        return a._next.first == b._next.first;
    }

    friend bool operator!=(const SyntheticGenerator& a, const SyntheticGenerator& b) {
        return a._next.first != b._next.first;
    }

private:
    value_type _next;
};

template <typename Aggregate>
void bulk_evict_insert_benchmark(Aggregate agg, Experiment exp) {
    typename Aggregate::outT force_side_effect = typename Aggregate::outT();
    typename Aggregate::timeT i = 0;

    std::cout << "window size " << exp.window_size 
              << ", iterations " << exp.iterations 
              << ", ooo " << exp.ooo_distance
              << ", bulk size " << exp.bulk_size
              << std::endl;
    if (!exp.latency) {
        for (i = exp.iterations - exp.ooo_distance; i < exp.iterations; ++i) {
            agg.insert(i, 1 + (i % 101));
        }
        for (i = 0; i < exp.window_size - exp.ooo_distance; ++i) {
            agg.insert(i, 1 + (i % 101));
        }

        if (agg.size() != exp.window_size) {
            std::cerr << "window is not exactly full; should be " << exp.window_size << ", but is " << agg.size();
            exit(2);
        }

        auto start = std::chrono::system_clock::now();

        uint64_t start_pos = exp.window_size - exp.ooo_distance;
        uint64_t stop_pos = exp.iterations - exp.ooo_distance;
        for (i = start_pos; i < stop_pos; i += exp.bulk_size) {
            agg.bulkEvict(i - exp.window_size + exp.bulk_size - 1);

            using SG = SyntheticGenerator<typename Aggregate::timeT, uint64_t>;
            SG begin(i, i);
            SG end(i + exp.bulk_size + 1, i + exp.bulk_size + 1);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            agg.bulkInsert(begin, end);

            silly_combine(force_side_effect, agg.query());
        }

        std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
        std::cout << "core runtime: " << runtime.count() << std::endl;
        std::cerr << force_side_effect << std::endl;
    }
    else {
        for (i = exp.iterations - exp.ooo_distance; i < exp.iterations; ++i) {
            agg.insert(i, 1 + (i % 101));
        }
        for (i = 0; i < exp.window_size - exp.ooo_distance; ++i) {
            agg.insert(i, 1 + (i % 101));
        }

        if (agg.size() != exp.window_size) {
            std::cerr << "window is not exactly full; should be " << exp.window_size << ", but is " << agg.size();
            exit(2);
        }

        uint64_t start_pos = exp.window_size - exp.ooo_distance;
        uint64_t stop_pos = exp.iterations - exp.ooo_distance;
        for (i = start_pos; i < stop_pos; i += exp.bulk_size) {
            std::atomic_thread_fence(std::memory_order_acquire);
            auto before = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);

            agg.bulkEvict(i - exp.window_size + exp.bulk_size - 1);

            std::atomic_thread_fence(std::memory_order_acquire);
            auto evict = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);

            using SG = SyntheticGenerator<typename Aggregate::timeT, uint64_t>;
            SG begin(i, i);
            SG end(i + exp.bulk_size + 1, i + exp.bulk_size + 1);
            agg.bulkInsert(begin, end);
            std::atomic_thread_fence(std::memory_order_acquire);
            auto insert = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);

            silly_combine(force_side_effect, agg.query());

            std::atomic_thread_fence(std::memory_order_acquire);
            auto after = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);
            exp.latencies.push_back(after - before);
            exp.extra_latencies[0].get().push_back(evict - before);
            exp.extra_latencies[1].get().push_back(insert - evict);
        }

        std::cerr << force_side_effect << std::endl;
    }
}

template <typename Aggregate>
void range_query_benchmark(Aggregate agg, SharingExperiment exp) {
    typename Aggregate::outT force_side_effect = typename Aggregate::outT();
    uint64_t i = 0;

    std::cout << "range big " << exp.window_size_big << ", small " << exp.window_size_small << ", distance " << exp.ooo_distance << ", iterations " << exp.iterations << std::endl;

    for (i = exp.iterations - exp.ooo_distance; i < exp.iterations; ++i) {
        agg.insert(i, 1 + (i % 101));
    }
    for (i = 0; i < exp.window_size_big - exp.ooo_distance; ++i) {
        agg.insert(i, 1 + (i % 101));
    }

    if (agg.size() != exp.window_size_big) {
        std::cerr << "window is not exactly full; should be " << exp.window_size_big << ", but is " << agg.size();
        exit(2);
    }

    auto start = std::chrono::system_clock::now();

    for (i = exp.window_size_big - exp.ooo_distance; i < exp.iterations - exp.ooo_distance; ++i) {
        std::atomic_thread_fence(std::memory_order_seq_cst);

        agg.evict();
        agg.insert(i, 1 + (i % 101));
        silly_combine(force_side_effect, agg.query());
        silly_combine(force_side_effect, agg.rangeQuery(i - exp.window_size_small + exp.ooo_distance, exp.iterations));
    }

    std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
    std::cout << "core runtime: " << runtime.count() << std::endl;
    std::cerr << force_side_effect << std::endl;
}

template <typename Aggregate>
void twin_benchmark(Aggregate agg_big, Aggregate agg_small, SharingExperiment exp) {
    typename Aggregate::outT force_side_effect = typename Aggregate::outT();
    uint64_t i = 0;

    std::cout << "twin big " << exp.window_size_big << ", small " << exp.window_size_small << ", distance " << exp.ooo_distance << ", iterations " << exp.iterations << std::endl;

    for (i = exp.iterations - exp.ooo_distance; i < exp.iterations; ++i) {
        agg_big.insert(i, 1 + (i % 101));
        agg_small.insert(i, 1 + (i % 101));
    }

    for (i = 0; i < exp.window_size_big - exp.ooo_distance; ++i) {
        agg_big.insert(i, 1 + (i % 101));
    }

    if (agg_big.size() != exp.window_size_big) {
        std::cerr << "big window is not exactly full; should be " << exp.window_size_big << ", but is " << agg_big.size();
        exit(2);
    }

    for (i = 0; i < exp.window_size_small - exp.ooo_distance; ++i) {
        agg_small.insert(i, 1 + (i % 101));
    }

    if (agg_small.size() != exp.window_size_small) {
        std::cerr << "small window is not exactly full at first pass; should be " << exp.window_size_small << ", but is " << agg_small.size();
        exit(2);
    }

    for (i = exp.window_size_small - exp.ooo_distance; i < exp.window_size_big - exp.ooo_distance; ++i) {
        agg_small.evict();
        agg_small.insert(i, 1 + (i % 101));
    }

    if (agg_small.size() != exp.window_size_small) {
        std::cerr << "small window is not exactly full at second pass; should be " << exp.window_size_small << ", but is " << agg_small.size();
        exit(2);
    }

    auto start = std::chrono::system_clock::now();

    for (i = exp.window_size_big - exp.ooo_distance; i < exp.iterations - exp.ooo_distance; ++i) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        agg_big.evict();
        agg_big.insert(i, 1 + (i % 101));
        silly_combine(force_side_effect, agg_big.query());

        agg_small.evict();
        agg_small.insert(i, 1 + (i % 101));
        silly_combine(force_side_effect, agg_small.query());
    }
    std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
    std::cout << "core runtime: " << runtime.count() << std::endl;
    std::cerr << force_side_effect << std::endl;
}

template <typename Data, typename Aggregate, typename Generator>
void data_benchmark(Aggregate agg, DataExperiment exp, Generator& gen, std::ostream& out) {
    typename Aggregate::outT force_side_effect = typename Aggregate::outT();

    gen.reset();

    if (!exp.latency) {
        std::chrono::time_point<std::chrono::system_clock> start;
        bool warmup = true;
        uint64_t count = 0;

        for (; gen.is_valid(); ++gen) {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if (agg.size() == 0 || exp.window_duration >= agg.youngest() - (*gen).order()) {
                agg.insert((*gen).order(), *gen);
                if (!warmup) {
                    ++count;
                }

                typename Aggregate::timeT youngest = agg.youngest();
                while (exp.window_duration < (youngest - agg.oldest())) {
                    agg.evict();
                    if (warmup) {
                        start = std::chrono::system_clock::now();
                    }
                    warmup = false;
                }
                silly_combine(force_side_effect, agg.query());
            }
        }

        std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
        if (warmup) {
            std::cout << "core runtime: invalid" << std::endl;
        }
        else {
            std::cout << "core runtime: " << runtime.count() << std::endl;
        }
        std::cout << force_side_effect << std::endl;

        out << ", " << count << "," << runtime.count();
    }
    else {
        bool warmup = true;

        for (; gen.is_valid(); ++gen) {
            std::atomic_thread_fence(std::memory_order_acquire);
            auto before = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);
            if (agg.size() == 0 || exp.window_duration >= agg.youngest() - (*gen).order()) {
                agg.insert((*gen).order(), *gen);

                typename Aggregate::timeT youngest = agg.youngest();
                while (exp.window_duration < (youngest - agg.oldest())) {
                    agg.evict();
                    warmup = false;
                }
                silly_combine(force_side_effect, agg.query());
            }

            std::atomic_thread_fence(std::memory_order_acquire);
            auto after = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);
            if (!warmup) {
                exp.latencies.push_back(after - before);
            }
        }

        if (warmup) {
            std::cerr << "warmup still true, results invalid" << std::endl;
        }

        std::cout << force_side_effect << std::endl;
    }
}

template <typename Data, typename Aggregate, typename Generator>
void bulk_data_benchmark(Aggregate agg, DataExperiment exp, Generator& gen, std::ostream& out) {
    typename Aggregate::outT force_side_effect = typename Aggregate::outT();
    // add 1 time unit so that youngest - delta is the last timestamp to be evicted
    auto delta = exp.window_duration; 
    delta++;

    gen.reset();

    if (!exp.latency) {
        std::chrono::time_point<std::chrono::system_clock> start;
        bool warmup = true;
        uint64_t count = 0;
        uint64_t added = 0; // meh

        for (; gen.is_valid(); ++gen) {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if (agg.size() == 0 || exp.window_duration >= agg.youngest() - (*gen).order()) {
                agg.insert((*gen).order(), *gen);
                ++added;
                if (!warmup) {
                    ++count;
                }

                typename Aggregate::timeT youngest = agg.youngest();
                if (exp.window_duration < (youngest - agg.oldest())) {
                    added = 0;
                    agg.bulkEvict(youngest - delta); 
                    if (warmup) { 
                        start = std::chrono::system_clock::now();
                    }
                    warmup = false;
                }
#ifdef _COLLECT_STATS
                printf("== size: size=%lu\n", agg.size());
#endif
                silly_combine(force_side_effect, agg.query());
            }
        }

        std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
        if (warmup) {
            std::cout << "core runtime: invalid" << std::endl;
        }
        else {
            std::cout << "core runtime: " << runtime.count() << std::endl;
        }
        std::cout << force_side_effect << std::endl;

        out << ", " << count << "," << runtime.count();
    }
    else {
        bool warmup = true;

        for (; gen.is_valid(); ++gen) {
            std::atomic_thread_fence(std::memory_order_acquire);
            auto before = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);
            if (agg.size() == 0 || exp.window_duration >= agg.youngest() - (*gen).order()) {
                agg.insert((*gen).order(), *gen);

                typename Aggregate::timeT youngest = agg.youngest();
                if (exp.window_duration < (youngest - agg.oldest())) {
                    agg.bulkEvict(youngest - delta); 
                    warmup = false;
                }
                silly_combine(force_side_effect, agg.query());
            }

            std::atomic_thread_fence(std::memory_order_acquire);
            auto after = rdtsc();
            std::atomic_thread_fence(std::memory_order_release);
            if (!warmup) {
                exp.latencies.push_back(after - before);
            }
        }

        if (warmup) {
            std::cerr << "warmup still true, results invalid" << std::endl;
        }

        std::cout << force_side_effect << std::endl;
    }
}

template <
    template <
        typename
    > class MakeAggregate
>
bool query_call_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        benchmark(MakeAggregate<Sum<int>>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        benchmark(MakeAggregate<Max<int>>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        benchmark(MakeAggregate<Mean<int>>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        benchmark(MakeAggregate<SampleStdDev<int>>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        benchmark(MakeAggregate<BloomFilter<int>>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        benchmark(MakeAggregate<Collect<int>>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        benchmark(MakeAggregate<MinCount<int>>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        benchmark(MakeAggregate<GeometricMean<int>>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        benchmark(MakeAggregate<BusyLoop<int>>()(0), exp);
        return true;
    }
    return false;
}

template <>
bool query_call_benchmark<soe::MakeAggregate>(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        benchmark(soe::MakeAggregate<Sum<int>>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        std::cerr << "error: sub_evict cannot do max" << std::endl;
        std::exit(2);
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        benchmark(soe::MakeAggregate<Mean<int>>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        benchmark(soe::MakeAggregate<SampleStdDev<int>>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        std::cerr << "error: sub_evict cannot do argmax" << std::endl;
        std::exit(2);
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        std::cerr << "error: sub_evict cannot do bloom" << std::endl;
        std::exit(2);
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        benchmark(soe::MakeAggregate<Collect<int>>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        std::cerr << "error: sub_evict cannot do mincount" << std::endl;
        std::exit(2);
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        benchmark(soe::MakeAggregate<GeometricMean<int>>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        benchmark(soe::MakeAggregate<BusyLoop<int>>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        bool caching
    > class MakeAggregate, 
    bool caching
>
bool query_call_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        benchmark(MakeAggregate<Sum<int>, caching>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        benchmark(MakeAggregate<Max<int>, caching>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        benchmark(MakeAggregate<Mean<int>, caching>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        benchmark(MakeAggregate<SampleStdDev<int>, caching>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, caching>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        benchmark(MakeAggregate<BloomFilter<int>, caching>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        benchmark(MakeAggregate<Collect<int>, caching>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        benchmark(MakeAggregate<MinCount<int>, caching>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        benchmark(MakeAggregate<GeometricMean<int>, caching>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        benchmark(MakeAggregate<BusyLoop<int>, caching>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        size_t MAX_CAPACITY
    > class MakeAggregate, 
    size_t MAX_CAPACITY,
    int magic
>
bool query_call_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        benchmark(MakeAggregate<Sum<int>, MAX_CAPACITY>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        benchmark(MakeAggregate<Max<int>, MAX_CAPACITY>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        benchmark(MakeAggregate<Mean<int>, MAX_CAPACITY>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        benchmark(MakeAggregate<SampleStdDev<int>, MAX_CAPACITY>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, MAX_CAPACITY>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        benchmark(MakeAggregate<BloomFilter<int>, MAX_CAPACITY>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        benchmark(MakeAggregate<Collect<int>, MAX_CAPACITY>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        benchmark(MakeAggregate<MinCount<int>, MAX_CAPACITY>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        benchmark(MakeAggregate<GeometricMean<int>, MAX_CAPACITY>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        benchmark(MakeAggregate<BusyLoop<int>, MAX_CAPACITY>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        typename time, 
        int minDegree, 
        btree::Kind kind
    > class MakeAggregate, 
    typename time, 
    int minDegree, 
    btree::Kind kind
>
bool query_call_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        benchmark(MakeAggregate<Sum<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        benchmark(MakeAggregate<Max<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        benchmark(MakeAggregate<Mean<int>, time, minDegree, kind>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        benchmark(MakeAggregate<SampleStdDev<int>, time, minDegree, kind>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time, minDegree, kind>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        benchmark(MakeAggregate<BloomFilter<int>, time, minDegree, kind>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        benchmark(MakeAggregate<Collect<int>, time, minDegree, kind>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        benchmark(MakeAggregate<MinCount<int>, time, minDegree, kind>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        benchmark(MakeAggregate<GeometricMean<int>, time, minDegree, kind>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        benchmark(MakeAggregate<BusyLoop<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename
    > class MakeAggregate
>
bool query_call_dynamic_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        dynamic_benchmark(MakeAggregate<Sum<int>>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        dynamic_benchmark(MakeAggregate<BloomFilter<int>>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        dynamic_benchmark(MakeAggregate<GeometricMean<int>>()(GeometricMean<int>::identity), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename,
        size_t MAX_CAPACITY
    > class MakeAggregate,
    size_t MAX_CAPACITY,
    int magic_no
>
bool query_call_dynamic_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        dynamic_benchmark(MakeAggregate<Sum<int>, MAX_CAPACITY>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        dynamic_benchmark(MakeAggregate<BloomFilter<int>, MAX_CAPACITY>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        dynamic_benchmark(MakeAggregate<GeometricMean<int>, MAX_CAPACITY>()(GeometricMean<int>::identity), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        bool caching
    > class MakeAggregate, 
    bool caching
>
bool query_call_dynamic_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        dynamic_benchmark(MakeAggregate<Sum<int>, caching>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        dynamic_benchmark(MakeAggregate<BloomFilter<int>, caching>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        dynamic_benchmark(MakeAggregate<GeometricMean<int>, caching>()(GeometricMean<int>::identity), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        typename time, 
        int minDegree, 
        btree::Kind kind
    > class MakeAggregate, 
    typename time, 
    int minDegree, 
    btree::Kind kind
>
bool query_call_dynamic_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        dynamic_benchmark(MakeAggregate<Sum<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        dynamic_benchmark(MakeAggregate<BloomFilter<int>, time, minDegree, kind>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        dynamic_benchmark(MakeAggregate<GeometricMean<int>, time, minDegree, kind>()(GeometricMean<int>::identity), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        typename time, 
        int minDegree, 
        btree::Kind kind
    > class MakeAggregate, 
    typename time, 
    int minDegree, 
    btree::Kind kind
>
bool query_call_ooo_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        ooo_benchmark(MakeAggregate<Sum<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        ooo_benchmark(MakeAggregate<Max<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        ooo_benchmark(MakeAggregate<Mean<int>, time, minDegree, kind>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        ooo_benchmark(MakeAggregate<SampleStdDev<int>, time, minDegree, kind>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        ooo_benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time, minDegree, kind>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        ooo_benchmark(MakeAggregate<BloomFilter<int>, time, minDegree, kind>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        ooo_benchmark(MakeAggregate<Collect<int>, time, minDegree, kind>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        ooo_benchmark(MakeAggregate<MinCount<int>, time, minDegree, kind>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        ooo_benchmark(MakeAggregate<GeometricMean<int>, time, minDegree, kind>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        ooo_benchmark(MakeAggregate<BusyLoop<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        typename time
    > class MakeAggregate, 
    typename time
>
bool query_call_ooo_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        ooo_benchmark(MakeAggregate<Sum<int>, time>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        ooo_benchmark(MakeAggregate<Max<int>, time>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        ooo_benchmark(MakeAggregate<Mean<int>, time>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        ooo_benchmark(MakeAggregate<SampleStdDev<int>, time>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        ooo_benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        ooo_benchmark(MakeAggregate<BloomFilter<int>, time>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        ooo_benchmark(MakeAggregate<Collect<int>, time>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        ooo_benchmark(MakeAggregate<MinCount<int>, time>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        ooo_benchmark(MakeAggregate<GeometricMean<int>, time>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        ooo_benchmark(MakeAggregate<BusyLoop<int>, time>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        typename time, 
        int minDegree, 
        btree::Kind kind
    > class MakeAggregate, 
    typename time, 
    int minDegree, 
    btree::Kind kind
>
bool query_call_bulk_evict_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        bulk_evict_benchmark(MakeAggregate<Sum<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        bulk_evict_benchmark(MakeAggregate<Max<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        bulk_evict_benchmark(MakeAggregate<Mean<int>, time, minDegree, kind>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        bulk_evict_benchmark(MakeAggregate<SampleStdDev<int>, time, minDegree, kind>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        bulk_evict_benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time, minDegree, kind>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        bulk_evict_benchmark(MakeAggregate<BloomFilter<int>, time, minDegree, kind>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        bulk_evict_benchmark(MakeAggregate<Collect<int>, time, minDegree, kind>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        bulk_evict_benchmark(MakeAggregate<MinCount<int>, time, minDegree, kind>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        bulk_evict_benchmark(MakeAggregate<GeometricMean<int>, time, minDegree, kind>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        bulk_evict_benchmark(MakeAggregate<BusyLoop<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        typename time
    > class MakeAggregate, 
    typename time
>
bool query_call_bulk_evict_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        bulk_evict_benchmark(MakeAggregate<Sum<int>, time>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        bulk_evict_benchmark(MakeAggregate<Max<int>, time>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        bulk_evict_benchmark(MakeAggregate<Mean<int>, time>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        bulk_evict_benchmark(MakeAggregate<SampleStdDev<int>, time>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        bulk_evict_benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        bulk_evict_benchmark(MakeAggregate<BloomFilter<int>, time>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        bulk_evict_benchmark(MakeAggregate<Collect<int>, time>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        bulk_evict_benchmark(MakeAggregate<MinCount<int>, time>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        bulk_evict_benchmark(MakeAggregate<GeometricMean<int>, time>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        bulk_evict_benchmark(MakeAggregate<BusyLoop<int>, time>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        typename time, 
        int minDegree, 
        btree::Kind kind
    > class MakeAggregate, 
    typename time, 
    int minDegree, 
    btree::Kind kind
>
bool query_call_bulk_evict_insert_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        bulk_evict_insert_benchmark(MakeAggregate<Sum<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        bulk_evict_insert_benchmark(MakeAggregate<Max<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        bulk_evict_insert_benchmark(MakeAggregate<Mean<int>, time, minDegree, kind>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        bulk_evict_insert_benchmark(MakeAggregate<SampleStdDev<int>, time, minDegree, kind>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        bulk_evict_insert_benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time, minDegree, kind>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        bulk_evict_insert_benchmark(MakeAggregate<BloomFilter<int>, time, minDegree, kind>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        bulk_evict_insert_benchmark(MakeAggregate<Collect<int>, time, minDegree, kind>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        bulk_evict_insert_benchmark(MakeAggregate<MinCount<int>, time, minDegree, kind>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        bulk_evict_insert_benchmark(MakeAggregate<GeometricMean<int>, time, minDegree, kind>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        bulk_evict_insert_benchmark(MakeAggregate<BusyLoop<int>, time, minDegree, kind>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        typename time
    > class MakeAggregate, 
    typename time
>
bool query_call_bulk_evict_insert_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, Experiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        bulk_evict_insert_benchmark(MakeAggregate<Sum<int>, time>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        bulk_evict_insert_benchmark(MakeAggregate<Max<int>, time>()(0), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        bulk_evict_insert_benchmark(MakeAggregate<Mean<int>, time>()(Mean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        bulk_evict_insert_benchmark(MakeAggregate<SampleStdDev<int>, time>()(SampleStdDev<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        bulk_evict_insert_benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        bulk_evict_insert_benchmark(MakeAggregate<BloomFilter<int>, time>()(BloomFilter<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        bulk_evict_insert_benchmark(MakeAggregate<Collect<int>, time>()(Collect<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        bulk_evict_insert_benchmark(MakeAggregate<MinCount<int>, time>()(MinCount<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        bulk_evict_insert_benchmark(MakeAggregate<GeometricMean<int>, time>()(GeometricMean<int>::identity), exp);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        bulk_evict_insert_benchmark(MakeAggregate<BusyLoop<int>, time>()(0), exp);
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        bool caching
    > class MakeAggregate, 
    bool caching
>
bool query_call_sharing_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, SharingExperiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<Sum<int>, caching>()(0), 
                           MakeAggregate<Sum<int>, caching>()(0), exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<Max<int>, caching>()(0), 
                           MakeAggregate<Max<int>, caching>()(0), exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<Mean<int>, caching>()(Mean<int>::identity), 
                           MakeAggregate<Mean<int>, caching>()(Mean<int>::identity), exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<SampleStdDev<int>, caching>()(SampleStdDev<int>::identity), 
                           MakeAggregate<SampleStdDev<int>, caching>()(SampleStdDev<int>::identity), exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, caching>()(ArgMax<int, int, IdentityLifter<int>>::identity), 
                           MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, caching>()(ArgMax<int, int, IdentityLifter<int>>::identity),
                           exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<BloomFilter<int>, caching>()(BloomFilter<int>::identity), 
                           MakeAggregate<BloomFilter<int>, caching>()(BloomFilter<int>::identity), exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<Collect<int>, caching>()(Collect<int>::identity), 
                           MakeAggregate<Collect<int>, caching>()(Collect<int>::identity), exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<MinCount<int>, caching>()(MinCount<int>::identity), 
                           MakeAggregate<MinCount<int>, caching>()(MinCount<int>::identity), exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<GeometricMean<int>, caching>()(GeometricMean<int>::identity), 
                           MakeAggregate<GeometricMean<int>, caching>()(GeometricMean<int>::identity), exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<BusyLoop<int>, caching>()(0), 
                           MakeAggregate<BusyLoop<int>, caching>()(0), exp);
        }
        else {
            std::cerr << "range_query requested for aggregator that does not support it" << std::endl;
        }
        return true;
    }
    return false;
}

template <
    template <
        typename, 
        typename time, 
        int minDegree, 
        btree::Kind kind
    > class MakeAggregate, 
    typename time, 
    int minDegree, 
    btree::Kind kind
>
bool query_call_sharing_benchmark(std::string aggregator, std::string aggregator_req, std::string function_req, SharingExperiment exp) {
    if (aggregator_req == aggregator && function_req == "sum") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<Sum<int>, time, minDegree, kind>()(0), 
                           MakeAggregate<Sum<int>, time, minDegree, kind>()(0), exp);
        }
        else {
            range_query_benchmark(MakeAggregate<Sum<int>, time, minDegree, kind>()(0), exp);
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<Max<int>, time, minDegree, kind>()(0), 
                           MakeAggregate<Max<int>, time, minDegree, kind>()(0), exp);
        }
        else {
            range_query_benchmark(MakeAggregate<Max<int>, time, minDegree, kind>()(0), exp);
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mean") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<Mean<int>, time, minDegree, kind>()(Mean<int>::identity), 
                           MakeAggregate<Mean<int>, time, minDegree, kind>()(Mean<int>::identity), exp);
        }
        else {
            range_query_benchmark(MakeAggregate<Mean<int>, time, minDegree, kind>()(Mean<int>::identity), exp);
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "stddev") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<SampleStdDev<int>, time, minDegree, kind>()(SampleStdDev<int>::identity), 
                           MakeAggregate<SampleStdDev<int>, time, minDegree, kind>()(SampleStdDev<int>::identity), exp);
        }
        else {
            range_query_benchmark(MakeAggregate<SampleStdDev<int>, time, minDegree, kind>()(SampleStdDev<int>::identity), exp);
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "argmax") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time, minDegree, kind>()(ArgMax<int, int, IdentityLifter<int>>::identity), 
                           MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time, minDegree, kind>()(ArgMax<int, int, IdentityLifter<int>>::identity),
                           exp);
        }
        else {
            range_query_benchmark(MakeAggregate<ArgMax<int, int, IdentityLifter<int>>, time, minDegree, kind>()(ArgMax<int, int, IdentityLifter<int>>::identity), exp);
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<BloomFilter<int>, time, minDegree, kind>()(BloomFilter<int>::identity), 
                           MakeAggregate<BloomFilter<int>, time, minDegree, kind>()(BloomFilter<int>::identity), exp);
        }
        else {
            range_query_benchmark(MakeAggregate<BloomFilter<int>, time, minDegree, kind>()(BloomFilter<int>::identity), exp);
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "collect") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<Collect<int>, time, minDegree, kind>()(Collect<int>::identity), 
                           MakeAggregate<Collect<int>, time, minDegree, kind>()(Collect<int>::identity), exp);
        }
        else {
            range_query_benchmark(MakeAggregate<Collect<int>, time, minDegree, kind>()(Collect<int>::identity), exp);
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "mincount") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<MinCount<int>, time, minDegree, kind>()(MinCount<int>::identity), 
                           MakeAggregate<MinCount<int>, time, minDegree, kind>()(MinCount<int>::identity), exp);
        }
        else {
            range_query_benchmark(MakeAggregate<MinCount<int>, time, minDegree, kind>()(MinCount<int>::identity), exp);
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<GeometricMean<int>, time, minDegree, kind>()(GeometricMean<int>::identity), 
                           MakeAggregate<GeometricMean<int>, time, minDegree, kind>()(GeometricMean<int>::identity), exp);
        }
        else {
            range_query_benchmark(MakeAggregate<GeometricMean<int>, time, minDegree, kind>()(GeometricMean<int>::identity), exp);
        }
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "busyloop") {
        if (exp.twin) {
            twin_benchmark(MakeAggregate<BusyLoop<int>, time, minDegree, kind>()(0), 
                           MakeAggregate<BusyLoop<int>, time, minDegree, kind>()(0), exp);
        }
        else {
            range_query_benchmark(MakeAggregate<BusyLoop<int>, time, minDegree, kind>()(0), exp);
        }
        return true;
    }
    return false;
}

template <
    typename Data, 
    template <
        typename,
        typename
    > class MakeTimeAggregate, 
    template <
        typename,
        bool caching,
        typename...
    > class FifoAggregate,
    bool caching,
    typename Generator
>
bool query_call_data_benchmark(const std::string& aggregator, 
                               const std::string& aggregator_req, 
                               const std::string& function_req, 
                               const DataExperiment& exp, 
                               Generator& gen, 
                               std::ostream& out) {
    if (aggregator_req == aggregator && function_req == "sum") {
        using SumOp = Sum<Data,int,int>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<SumOp, caching>>()(SumOp(), SumOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        using MaxOp = Max<Data,typename Data::timestamp,typename Data::timestamp>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<MaxOp, caching>>()(MaxOp(), MaxOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        using GeoOp = GeometricMean<Data,double>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<GeoOp, caching>>()(GeoOp(), GeoOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        using BloomOp = BloomFilter<Data,uint64_t>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<BloomOp, caching>>()(BloomOp(), BloomOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "relvar") {
        using RelVar = RelativeVariation<Data,double>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<RelVar, caching>>()(RelVar(), RelVar::identity), exp, gen, out);
        return true;
    }
    return false;
}

template <
    typename Data, 
    template <
        typename,
        typename
    > class MakeTimeAggregate, 
    template <
        typename...
    > class FifoAggregate,
    typename Generator
>
bool query_call_data_benchmark(const std::string& aggregator, 
                               const std::string& aggregator_req, 
                               const std::string& function_req, 
                               const DataExperiment& exp, 
                               Generator& gen, 
                               std::ostream& out) {
    if (aggregator_req == aggregator && function_req == "sum") {
        using SumOp = Sum<Data,int,int>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<SumOp>>()(SumOp(), SumOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        using MaxOp = Max<Data,typename Data::timestamp,typename Data::timestamp>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<MaxOp>>()(MaxOp(), MaxOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        using GeoOp = GeometricMean<Data,double>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<GeoOp>>()(GeoOp(), GeoOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        using BloomOp = BloomFilter<Data,uint64_t>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<BloomOp>>()(BloomOp(), BloomOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "relvar") {
        using RelVar = RelativeVariation<Data,double>;
        data_benchmark<Data>(MakeTimeAggregate<typename Data::timestamp, FifoAggregate<RelVar>>()(RelVar(), RelVar::identity), exp, gen, out);
        return true;
    }
    return false;
}

template <
    typename Data, 
    template <
        typename, 
        typename, 
        int minDegree, 
        btree::Kind kind
    > class MakeAggregate, 
    int minDegree, 
    btree::Kind kind,
    typename Generator
>
bool query_call_data_benchmark(const std::string& aggregator, 
                               const std::string& aggregator_req, 
                               const std::string& function_req, 
                               const DataExperiment& exp, 
                               Generator& gen, 
                               std::ostream& out) {
    if (aggregator_req == aggregator && function_req == "sum") {
        using SumOp = Sum<Data,int,int>;
        data_benchmark<Data>(MakeAggregate<SumOp, typename Data::timestamp, minDegree, kind>()(SumOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        using MaxOp = Max<Data,typename Data::timestamp,typename Data::timestamp>;
        data_benchmark<Data>(MakeAggregate<MaxOp, typename Data::timestamp, minDegree, kind>()(MaxOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        using GeoOp = GeometricMean<Data,double>;
        data_benchmark<Data>(MakeAggregate<GeoOp, typename Data::timestamp, minDegree, kind>()(GeoOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        using BloomOp = BloomFilter<Data,uint64_t>;
        data_benchmark<Data>(MakeAggregate<BloomOp, typename Data::timestamp, minDegree, kind>()(BloomOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "relvar") {
        using RelVar = RelativeVariation<Data,double>;
        data_benchmark<Data>(MakeAggregate<RelVar, typename Data::timestamp, minDegree, kind>()(RelVar::identity), exp, gen, out);
        return true;
    }

    return false;
}

template <
    typename Data, 
    template <
        typename, 
        typename
    > class MakeAggregate, 
    typename Generator
>
bool query_call_data_benchmark(const std::string& aggregator, 
                               const std::string& aggregator_req, 
                               const std::string& function_req, 
                               const DataExperiment& exp, 
                               Generator& gen, 
                               std::ostream& out) {
    if (aggregator_req == aggregator && function_req == "sum") {
        using SumOp = Sum<Data,int,int>;
        data_benchmark<Data>(MakeAggregate<SumOp, typename Data::timestamp>()(SumOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        using MaxOp = Max<Data,typename Data::timestamp,typename Data::timestamp>;
        data_benchmark<Data>(MakeAggregate<MaxOp, typename Data::timestamp>()(MaxOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        using GeoOp = GeometricMean<Data,double>;
        data_benchmark<Data>(MakeAggregate<GeoOp, typename Data::timestamp>()(GeoOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        using BloomOp = BloomFilter<Data,uint64_t>;
        data_benchmark<Data>(MakeAggregate<BloomOp, typename Data::timestamp>()(BloomOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "relvar") {
        using RelVar = RelativeVariation<Data,double>;
        data_benchmark<Data>(MakeAggregate<RelVar, typename Data::timestamp>()(RelVar::identity), exp, gen, out);
        return true;
    }

    return false;
}

template <
    typename Data,
    template <
        typename,
        typename,
        bool caching
    > class MakeAggregate,
    bool caching,
    typename Generator
>
bool query_call_data_benchmark(const std::string& aggregator,
                               const std::string& aggregator_req,
                               const std::string& function_req,
                               const DataExperiment& exp,
                               Generator& gen,
                               std::ostream& out) {
    if (aggregator_req == aggregator && function_req == "sum") {
        using SumOp = Sum<Data,int,int>;
        data_benchmark<Data>(MakeAggregate<SumOp, typename Data::timestamp, caching>()(SumOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        using MaxOp = Max<Data,typename Data::timestamp,typename Data::timestamp>;
        data_benchmark<Data>(MakeAggregate<MaxOp, typename Data::timestamp, caching>()(MaxOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        using GeoOp = GeometricMean<Data,double>;
        data_benchmark<Data>(MakeAggregate<GeoOp, typename Data::timestamp, caching>()(GeoOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        using BloomOp = BloomFilter<Data,uint64_t>;
        data_benchmark<Data>(MakeAggregate<BloomOp, typename Data::timestamp, caching>()(BloomOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "relvar") {
        using RelVar = RelativeVariation<Data,double>;
        data_benchmark<Data>(MakeAggregate<RelVar, typename Data::timestamp, caching>()(RelVar::identity), exp, gen, out);
        return true;
    }

    return false;
}

template <
    typename Data,
    template <
        typename,
        typename,
        size_t MAX_CAPACITY
    > class MakeAggregate,
    size_t MAX_CAPACITY,
    int magic_no,
    typename Generator
>
bool query_call_data_benchmark(const std::string& aggregator,
                               const std::string& aggregator_req,
                               const std::string& function_req,
                               const DataExperiment& exp,
                               Generator& gen,
                               std::ostream& out) {
    if (aggregator_req == aggregator && function_req == "sum") {
        using SumOp = Sum<Data,int,int>;
        data_benchmark<Data>(MakeAggregate<SumOp, typename Data::timestamp, MAX_CAPACITY>()(SumOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        using MaxOp = Max<Data,typename Data::timestamp,typename Data::timestamp>;
        data_benchmark<Data>(MakeAggregate<MaxOp, typename Data::timestamp, MAX_CAPACITY>()(MaxOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        using GeoOp = GeometricMean<Data,double>;
        data_benchmark<Data>(MakeAggregate<GeoOp, typename Data::timestamp, MAX_CAPACITY>()(GeoOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        using BloomOp = BloomFilter<Data,uint64_t>;
        data_benchmark<Data>(MakeAggregate<BloomOp, typename Data::timestamp, MAX_CAPACITY>()(BloomOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "relvar") {
        using RelVar = RelativeVariation<Data,double>;
        data_benchmark<Data>(MakeAggregate<RelVar, typename Data::timestamp, MAX_CAPACITY>()(RelVar::identity), exp, gen, out);
        return true;
    }

    return false;
}

template <
    typename Data, 
    template <
        typename, 
        typename, 
        int minDegree, 
        btree::Kind kind
    > class MakeAggregate, 
    int minDegree, 
    btree::Kind kind,
    typename Generator
>
bool query_call_bulk_data_benchmark(const std::string& aggregator, 
                               const std::string& aggregator_req, 
                               const std::string& function_req, 
                               const DataExperiment& exp, 
                               Generator& gen, 
                               std::ostream& out) {
    if (aggregator_req == aggregator && function_req == "sum") {
        using SumOp = Sum<Data,int,int>;
        bulk_data_benchmark<Data>(MakeAggregate<SumOp, typename Data::timestamp, minDegree, kind>()(SumOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "max") {
        using MaxOp = Max<Data,typename Data::timestamp,typename Data::timestamp>;
        bulk_data_benchmark<Data>(MakeAggregate<MaxOp, typename Data::timestamp, minDegree, kind>()(MaxOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "geomean") {
        using GeoOp = GeometricMean<Data,double>;
        bulk_data_benchmark<Data>(MakeAggregate<GeoOp, typename Data::timestamp, minDegree, kind>()(GeoOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "bloom") {
        using BloomOp = BloomFilter<Data,uint64_t>;
        bulk_data_benchmark<Data>(MakeAggregate<BloomOp, typename Data::timestamp, minDegree, kind>()(BloomOp::identity), exp, gen, out);
        return true;
    }
    else if (aggregator_req == aggregator && function_req == "relvar") {
        using RelVar = RelativeVariation<Data,double>;
        bulk_data_benchmark<Data>(MakeAggregate<RelVar, typename Data::timestamp, minDegree, kind>()(RelVar::identity), exp, gen, out);
        return true;
    }

    return false;
}

template <typename T=cycle_duration>
void write_latency(std::string filespec, std::vector<T> &latencies) {
    std::ofstream out(filespec);
    for (auto e: latencies) {
        out << e << std::endl;
    }
}


#endif // _BENCHMARK_CORE_H
