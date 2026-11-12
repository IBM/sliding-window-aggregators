#ifndef _UTILS_H_
#define _UTILS_H_

#include <chrono>
#include <ctime>
#include <list>
#include <iostream>
#include <string>
#include <sstream>

// Provides sane time_point use. See:
//   - https://howardhinnant.github.io/date/date.html
//   - https://stackoverflow.com/questions/26537656/what-is-the-best-way-to-parse-a-millisecond-date-time-in-c11
#include "date.h"

const std::string YmdHMS = "%Y-%m-%d %H:%M:%S";

// See:
//   - https://howardhinnant.github.io/date/date.html
std::ostream& operator<<(std::ostream& out, const std::chrono::system_clock::time_point& tp) {
    return date::operator<<(out, tp);
}

template <typename T>
T from_string(const std::string& s) {
    std::stringstream ss(s);
    T t;
    ss >> t;
    return t;
}

// See:
//   - https://howardhinnant.github.io/date/date.html
template <>
std::chrono::system_clock::time_point from_string(const std::string& s) {
    std::stringstream ss(s);
    std::chrono::system_clock::time_point t;
    ss >> date::parse(YmdHMS.c_str(), t);
    return t;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::list<T>& lst) {
    for (const T& e: lst) {
        out << e;
    }
    return out;
}

// Originally, cycle_duration was a high_resolution_clock's duration
// typedef std::chrono::high_resolution_clock::duration cycle_duration;
// Most recently, we changed this to just a uint64_t to reflect TSC readings
typedef uint64_t cycle_duration;

// rdtsc() returns a fine-grained, monotonic tick count. The unit depends on
// the platform:
//   - x86 / x86_64: TSC ticks (invariant TSC on modern CPUs)
//   - aarch64 (incl. Apple M-series): ticks of the ARM generic timer
//     (CNTVCT_EL0), whose frequency is in CNTFRQ_EL0 -- e.g., 24MHz on
//     M1-era macOS, 1GHz on newer macOS. The true cycle counter
//     (PMCCNTR_EL0) is not readable from user space on macOS.
//   - anything else: nanoseconds from std::chrono::steady_clock
#if defined(__x86_64__) || defined(__i386__)

inline uint64_t rdtscp() {
  uint32_t lo, hi;
  __asm__ __volatile__("rdtscp"            // serializing read of tsc
                       : "=a"(lo), "=d"(hi)
                       :
                       : "%ecx");          // ecx (TSC_AUX) is clobbered
  return (((uint64_t)hi) << 32) | ((uint64_t)lo);
};

static inline uint64_t rdtsc()
{
  uint32_t lo, hi;
  __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
  return (((uint64_t)hi) << 32) | ((uint64_t)lo);
}

#elif defined(__aarch64__)

// Note: separate instructions with newlines, not ';' -- on Apple's arm64
// assembler, ';' starts a comment and would silently drop the mrs.
static inline uint64_t rdtsc()
{
  uint64_t val;
  // isb keeps the counter read from being speculated ahead of earlier code
  __asm__ volatile("isb\n\t"
                   "mrs %0, cntvct_el0"
                   : "=r" (val) : : "memory");
  return val;
}

inline uint64_t rdtscp() { return rdtsc(); }

#else

static inline uint64_t rdtsc()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline uint64_t rdtscp() { return rdtsc(); }

#endif

#endif // _UTILS_H_
