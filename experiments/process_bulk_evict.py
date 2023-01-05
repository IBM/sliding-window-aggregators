#!/usr/bin/env python3

import sys, collections
import matplotlib.pyplot as plt
import math
import numpy as np
import process_utility as u
from run_bulk_evict import functions
from bulk_data_common import *

Line = collections.namedtuple("Line", ["style", "color"])

aggregators = {  # "bfinger2":   Line('-',  'red'),
    "bfinger4": Line("-", "blue"),
    "bfinger8": Line("-", "green"),
    # "nbfinger2":  Line('--',  'red'),
    "nbfinger4": Line("--", "blue"),
    "nbfinger8": Line("--", "green"),
    "amta": Line("-", "red"),
    #"two_stacks_lite": Line("-", "maroon"),
    "chunked_two_stacks_lite": Line("-", "maroon"),
    "daba_lite": Line("--", "maroon"),
}
aggs_sorted = [
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

bulk_axis = [2**i for i in range(0, 22)]
X_AXIS = [
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


def make_throughput_graph(preamble, function, i, mapping, constant, varying):
    graph = plt.figure()
    ax = graph.add_subplot(111)
    ax.set_title(
        "bulkEvict "
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
    ax.set_xticks(X_AXIS)
    ax.set_ylabel("throughput [million items/s]")
    # for key, data in mapping.items():
    for agg in aggs_sorted:
        data = mapping[agg]
        if len(data) <= 1:
            return
        x_axis = np.array(sorted(data.keys()))
        throughput = np.array([data[w].avg for w in sorted(data.keys())]) / 1e6

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

    # shrink axis by 25% and put legend outside and to the right
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 1.1, box.height * 0.85])
    ax.legend(
        frameon=False,
        ncol=3,
        loc="upper center",
        columnspacing=0.25,
        bbox_to_anchor=(0.5, 1.4),
    )

    graph.savefig(
        "figures/" + preamble + "_" + constant + str(i) + "_" + function + ".pdf"
    )
    plt.close()


def make_throughput_panel(preamble, functions, ii, mappings, constant, varying, sorted_keys=aggs_sorted, x_ticks=X_AXIS, x_labels=None):
    # graph, axes = plt.subplots(1, 3, figsize=(24, 6))
    #graph, axes = plt.subplots(1, 3, figsize=(24, 6))
    #graph, axes = plt.subplots(1, 3, figsize=(16, 4))
    graph, axes = plt.subplots(1, 3, figsize=(24*0.75, 6*0.75))
    axes[0].set_ylabel("throughput [million items/s]")
    axes[len(axes)//2].set_xlabel(varying)
    for i, ax in enumerate(axes):
        ax.set_title(functions[i])
        ax.set_xscale("log", base=2)
        if x_labels:
            ax.set_xticks(x_ticks,  x_labels)
        else:
            ax.set_xticks(x_ticks)
        for agg in sorted_keys:
            data = mappings[i][agg]
            if len(data) <= 1:
                return
            x_axis = np.array(sorted(data.keys()))
            throughput = np.array([data[w].avg for w in sorted(data.keys())]) / 1e6

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
        ax.grid()
        ax.set_ylim(bottom=0)
    lines, labels = axes[0].get_legend_handles_labels()
    margin = 0.04
    graph.subplots_adjust(top=0.8, left=0.0+margin, right=1.0-margin, bottom=0.2)
    graph.legend(
        lines,
        labels,
        frameon=False,
        ncol=len(labels),
        loc="upper center",
    )
    # graph.tight_layout()
    name =  "figures/" + preamble + "_" + constant + str(ii) + "_panel.pdf"
    print(f"Writing to {name}")
    graph.savefig(name)
    plt.close("all")



def make_bulk_size_varying_graph(preamble, func_to_data):
    ww = None
    for f, windows in func_to_data.items():
        for w, aggs in windows.items():
            ww = w
            print(f)
            make_throughput_graph(
                preamble, f, w, aggs, constant="window", varying="bulk size"
            )
    functions = ["sum", "geomean", "bloom"]
    aggs_array = [func_to_data[f][ww] for f in functions] 
    make_throughput_panel(preamble, functions, ww, aggs_array, constant="window", varying="bulk size")


def func_varying_bulk_size(agg_to_func):
    ret = collections.defaultdict(
        lambda: collections.defaultdict(lambda: collections.defaultdict(dict))
    )
    for agg, func in agg_to_func.items():
        for f, distance in func.items():
            for _, window in distance.items():
                for w, bulk_sizes in window.items():
                    for b, data in bulk_sizes.items():
                        ret[f][w][agg][b] = data
    return ret


def main():
    u.create_figures_dir()
    u.set_fonts()

    agg_to_func = collections.defaultdict(lambda: collections.defaultdict(dict))
    for agg in aggregators.keys():
        agg_to_func[agg] = u.read_bulk_throughput_data("bulk_evict", agg, functions)

    make_bulk_size_varying_graph("bulk_evict", func_varying_bulk_size(agg_to_func))


if __name__ == "__main__":
    main()
