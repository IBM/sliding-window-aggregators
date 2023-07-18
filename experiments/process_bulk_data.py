#!/usr/bin/env python3

import sys, collections
import process_utility as u
import run_bulk_data  as rb
import matplotlib.pyplot as plt
import numpy as np
from typing import Dict
from bulk_data_common import *
from process_bulk_evict import make_throughput_panel

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
               u.Aggregator("bfinger4", '-', 'blue'),
               u.Aggregator("nbfinger4", '--', 'blue'),
               u.Aggregator("bfinger8", '-', 'green'),
               u.Aggregator("nbfinger8", '--', 'green'),
#               u.Aggregator("nbclassic4", '-.', 'darkblue'),
#               u.Aggregator("nbclassic8", '-.', 'darkgreen'),
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
    
def make_data_throughput_graph(
    title: str, preamble: str, varying: str, function: str, aggs: Dict[str, Dict[str, u.BulkData]], aggregators: list[str]
):
    graph = plt.figure()
    ax = graph.add_subplot(111)
    ax.set_title(title + " " + function)
    ax.set_xlabel(varying)
    ax.set_xscale("log", base=10)
    ax.set_ylabel("throughput [million items/s]")

    for aggregator in aggregators:
        data = aggs[aggregator.name]
        x_axis = np.array(sorted([int(w) for w in data.keys()]))
        throughput = np.array([data[w].avg for w in x_axis]) / 1e6

        stddev = np.array([data[w].std for w in x_axis]) / 1e6
        ax.errorbar(
            x_axis,
            throughput,
            yerr=stddev,
            label=rename_list.get(aggregator.name, aggregator.name),
            linewidth=2,
            linestyle=aggregator.style,
            color=aggregator.color,
        )

    # shrink axis by 25% and put legend outside and to the right
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 1.1, box.height * 0.75])
    ax.legend(
        frameon=False,
        ncol=3,
        loc="upper center",
        columnspacing=0.5,
        bbox_to_anchor=(0.45, 1.55),
    )

    graph.savefig("figures/" + preamble + "_" + function + ".pdf")

def main():
    u.create_figures_dir()
    u.set_fonts()

    agg_to_func = collections.defaultdict(lambda: collections.defaultdict(dict))
    for agg in aggregators:
        agg_to_func[agg.name] = u.read_citibike_throughput(agg.name, functions)

    for f, aggs in func_varying_duration(agg_to_func).items():
        print(f"--- {f}")
        make_data_throughput_graph('Citi Bike', 'bike', 'window size in seconds', f, aggs, aggregators)

    func_to_data = func_varying_duration(agg_to_func)
    aggs_array = [func_to_data[f] for f in functions]
    DAY = 60 * 60 * 24 # one day is 60 seconds * 60 minutes * 24 hours
    durations = [DAY/4, DAY/2, 1*DAY, 2*DAY, 4*DAY, 8*DAY, 12*DAY, 16*DAY, 24*DAY, 32*DAY]
    labels = ['1/4', '1/2', '1', '2', '4', '8', '12', '16', '24', '32']
    make_throughput_panel("bike", functions, 0, aggs_array, constant="bike", varying="window size in days", sorted_keys=[a.name for a in aggregators], x_ticks=durations, x_labels=labels)

if __name__ == "__main__":
    main()
