#!/usr/bin/env python3

import sys, collections
import matplotlib.pyplot as plt
import math
import numpy as np
import process_utility as u
from run_bulk_evict import base_iterations, bulk_sizes, base_window_sizes, functions
from run_bulk_evict_insert import base_iterations
from process_bulk_evict import make_throughput_panel
from bulk_data_common import *

Line = collections.namedtuple("Line", ["style", "color"])

aggregators = {  # "bfinger2":   Line('-',  'red'),
    "bfinger4": Line("-", "blue"),
    "bfinger8": Line("-", "green"),
    # "nbfinger2":  Line('--',  'red'),
    "nbfinger4": Line("--", "blue"),
    "nbfinger8": Line("--", "green"),
    "amta": Line("-", "red"),
    # "two_stacks_lite": Line("-", "maroon"),
    "chunked_two_stacks_lite": Line("-", "maroon"),
    "daba_lite": Line("--", "maroon"),
}
fifo_aggs_sorted = [
    "bfinger4",
    "bfinger8",
    # "nbfinger2",
    "nbfinger4",
    "nbfinger8",
    "amta",
    # "two_stacks_lite",
    "chunked_two_stacks_lite",
    "daba_lite",
]

ooo_aggs_sorted = [
    "bfinger4",
    "bfinger8",
    "nbfinger4",
    "nbfinger8",
]

bulk_axis = [2**i for i in range(0, 22)]
windows = base_window_sizes


def make_throughput_graph(preamble, function, i, mapping, constant, varying, aggs_sorted):
    graph = plt.figure()
    ax = graph.add_subplot(111)
    ax.set_title(
        "Bulk Evict+Insert"
        + function
        + ", "
        + constant
        + " "
        + "$2^{"
        + str(int(math.log(i, 2)))
        + "}$"
    )
    ax.set_xlabel(varying)
    ax.set_xscale("log", base=2)
    ax.set_xlim(right=2**22)
    ax.set_xticks(
        [
            1,
            2,
            2**3,
            2**5,
            2**7,
            2**9,
            2**11,
            2**13,
            2**15,
            2**17,
            2**19,
            2**21,
        ]
    )
    ax.set_ylabel("throughput [million items/s]")
    # for key, data in mapping.items():
    themax = float('-INF')
    for agg in aggs_sorted:
        data = mapping[agg]
        if len(data) <= 1:
            return
        x_axis = np.array(sorted(data.keys()))
        throughput = np.array([data[w].avg for w in sorted(data.keys())]) / 1e6
        themax = max(themax, np.max(throughput))

        stddev = np.array([data[w].std for w in sorted(data.keys())]) / 1e6
        ax.errorbar(
            x_axis,
            throughput,
            yerr=stddev,
            label=rename_list.get(agg, agg),
            linewidth=2,
            linestyle=aggregators[agg].style,
            color=aggregators[agg].color,
        )
    print(f"function={function}, i = {i}, themax={themax}")
    ax.set_ylim(bottom=0)
    # shrink axis by 25% and put legend outside and to the right
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 1.1, box.height * 0.85])
    ax.legend(
        frameon=False,
        ncol=3,
        loc="upper center",
        columnspacing=0.5,
        bbox_to_anchor=(0.5, 1.4),
    )

    graph.savefig(
        "figures/" + preamble + "_" + constant + str(i) + "_" + function + ".pdf"
    )
    plt.close()


def make_bulk_size_varying_graph(preamble, func_to_data):
    ww = None
    for f, windows in func_to_data.items():
        for w, aggs in windows.items():
            ww = w
            make_throughput_graph(
                preamble, f, w, aggs, constant="window", varying="bulk size",
                aggs_sorted=fifo_aggs_sorted
            )
    functions = ["sum", "geomean", "bloom"]
    aggs_array = [func_to_data[f][ww] for f in functions] 
    make_throughput_panel(preamble, functions, ww, aggs_array, constant="window", varying="bulk size")

def make_ooo_distance_varying_graph(preamble, func_to_data):
    for f, windows in func_to_data.items():
        for b, aggs in windows.items():
            make_throughput_graph(
                preamble, f, b, aggs, constant="bulksize", varying="ooo distance",
                aggs_sorted=ooo_aggs_sorted,
            )
    functions = ["sum", "geomean", "bloom"]
    for bb in bulk_sizes:
        aggs_array = [func_to_data[f][bb] for f in functions]
        make_throughput_panel(preamble, functions, bb, aggs_array, constant="bulksize", varying="ooo distance", sorted_keys=ooo_aggs_sorted)


def func_varying_bulk_size(agg_to_func, for_distance: int = 0):
    ret = collections.defaultdict(
        lambda: collections.defaultdict(lambda: collections.defaultdict(dict))
    )
    for agg, func in agg_to_func.items():
        for f, distance in func.items():
            window = distance[for_distance]
            for w, bulk_sizes in window.items():
                for b, data in bulk_sizes.items():
                    ret[f][w][agg][b] = data
    return ret

def func_varying_distance(agg_to_func):
    ret = collections.defaultdict(
        lambda: collections.defaultdict(lambda: collections.defaultdict(dict))
    )
    for agg, func in agg_to_func.items():
        for f, distance in func.items():
            for d, window in distance.items():
                for _, bulk_sizes in window.items():
                    for b, data in bulk_sizes.items():
                        if d <= 0: pass
                        ret[f][b][agg][d] = data
    return ret
    

def main():
    u.create_figures_dir()
    u.set_fonts()

    agg_to_func = collections.defaultdict(lambda: collections.defaultdict(dict))
    for agg in aggregators.keys():
        agg_to_func[agg] = u.read_bulk_throughput_data("bulk_evict_insert", agg, functions)

    make_bulk_size_varying_graph("bulk_evict_insert", func_varying_bulk_size(agg_to_func))
    make_ooo_distance_varying_graph("bulk_evict_insert", func_varying_distance(agg_to_func))


if __name__ == "__main__":
    main()
 
