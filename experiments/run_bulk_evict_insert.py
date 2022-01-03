#!/usr/bin/env python2.7

import run_utility as u

aggregators = [
                "bfinger2",
                "bfinger4",
                "bfinger8",
                "amta",
                "two_stacks_lite",
                "daba_lite",
              ]

degrees = [0]

# the current window sizes and iterations take an enormously long time

base_window_sizes = [4*u.MB]

base_iterations = 1 * u.MILLION

bulk_sizes = [1, 256]

functions = { 
             "sum": (base_iterations, base_window_sizes),
             "geomean": (base_iterations, base_window_sizes),
             "bloom": (base_iterations/100, base_window_sizes),
            }

def main():
    u.run_bulk(aggregators, functions, degrees, bulk_sizes, 'bulk_evict_insert')

if __name__ == "__main__":
    main()
