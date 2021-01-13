#!/usr/bin/env python2.7

import run_utility as u

aggregators = [
    "daba",
    "daba_lite",
    "two_stacks",
    "two_stacks_lite",
    "rb_two_stacks_lite",
    "chunked_two_stacks_lite",
    "flatfit",
    "bclassic4",
    "bfinger4",
]

base_window_size = 16 * u.KB

base_iterations = 10 * u.MILLION

functions = { 
                "sum": (base_iterations, base_window_size),
                "geomean": (base_iterations, base_window_size),
                "bloom": (base_iterations, base_window_size),
            }

def main():
    u.run_fifo_latency(aggregators, functions)

if __name__ == "__main__":
    main()
