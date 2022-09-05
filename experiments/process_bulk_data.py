#!/usr/bin/env python3

import sys, collections
import process_utility as u
import run_bulk_data  as rb
import matplotlib.pyplot as plt
import numpy as np

    # "bfinger2",
    # "bfinger4",
    # "bfinger8",
    # "nbfinger2",
    # "nbfinger4",
    # "nbfinger8",
    # "nbclassic2",
    # "nbclassic4",
    # "nbclassic8",
aggregators = [
               u.Aggregator("bfinger4", '-', 'black'),
               u.Aggregator("nbfinger4", '--', 'black'),
               u.Aggregator("bfinger8", '-', 'red'),
               u.Aggregator("nbfinger8", '--', 'red'),
               u.Aggregator("nbclassic4", '-', 'darkorchid'),
               u.Aggregator("nbclassic8", '-', 'green'),
              ]

durations = rb.durations

functions = [ 
             "sum",
             "geomean",
             "bloom",
            ]

def func_varying_duration(agg_to_func):
    ret = collections.defaultdict(
        lambda: collections.defaultdict(lambda: collections.defaultdict(dict))
    )
    for agg, func in agg_to_func.items():
        for f, durations in func.items():
            for dr, data in durations.items():
                ret[f][agg][dr] = data
    return ret
    
def main():
    u.create_figures_dir()
    u.set_fonts()

    agg_to_func = collections.defaultdict(lambda: collections.defaultdict(dict))
    for agg in aggregators:
        agg_to_func[agg.name] = u.read_citibike_throughput(agg.name, functions)

    for f, aggs in func_varying_duration(agg_to_func).items():
        print(f"--- {f}")
        u.make_data_throughput_graph('Citi Bike', 'bike', 'window size in seconds', f, aggs, aggregators)

if __name__ == "__main__":
    main()
