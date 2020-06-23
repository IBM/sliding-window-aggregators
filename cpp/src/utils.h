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

inline uint64_t rdtscp() {
  uint64_t tsc;
  __asm__ __volatile__("rdtscp; "         // serializing read of tsc
                       "shl $32,%%rdx; "  // shift higher 32 bits stored in rdx up
                       "or %%rdx,%%rax"   // and or onto rax
                       : "=a"(tsc)        // output to tsc variable
                       :
                       : "%rcx", "%rdx"); // rcx and rdx are clobbered
  return tsc;
};

static inline uint64_t rdtsc()
{
  uint32_t lo, hi;
  __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
  return (((uint64_t)hi) << 32) | ((uint64_t)lo);
}

#endif // _UTILS_H_
