#!/usr/bin/env python

import sys, collections
import process_utility as u

base_iterations = 200 * u.MILLION

aggregators = [u.Aggregator("daba", '-', 'black'),
               u.Aggregator("daba_lite", '--', 'black'),
               u.Aggregator("two_stacks", '-', 'red'),
               u.Aggregator("two_stacks_lite", '--', 'red'),
               u.Aggregator("flatfit", '-', 'blue'),
               u.Aggregator("bfinger4", '-', 'green'),
              ]

windows = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 
           1*u.KB, 2*u.KB, 4*u.KB, 8*u.KB, 16*u.KB, 32*u.KB, 64*u.KB, 128*u.KB, 256*u.KB, 512*u.KB,
           1*u.MB, 2*u.MB, 4*u.MB
          ]

functions = {"sum": base_iterations,
             "geomean": base_iterations,
             "bloom": base_iterations/50,
            }

def main():
    u.create_figures_dir()
    u.set_fonts()

    agg_to_func = collections.defaultdict(lambda: collections.defaultdict(dict))
    for agg in aggregators:
        agg_to_func[agg.name] = u.read_throughput('fifo', agg.name, functions)

    u.make_small_window_varying_graphs('FIFO', 'fifo', 'window size in data items', aggregators, 
                                       u.func_varying_small_window(agg_to_func))
if __name__ == "__main__":
    main()
