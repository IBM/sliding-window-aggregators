#!/usr/bin/env python3

import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, AutoMinorLocator
import numpy as np
from sys import stdout
import process_utility as u
from bulk_data_common import *

aggs_sorted = [
    "bfinger4",
    "nbfinger4",
    "nbclassic4",
    "bfinger8",
    "nbfinger8",
    "nbclassic8",
]

MAX_YAXIS = {
    "sum": 5e5,
    "geomean": 5e5,
    "bloom": 3e7,
}
Y_TICKS = [1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9]

# in seconds
windows = [
    86400,  # one day
    1382400,  # 16 days
]

functions = [
    "sum",
    "geomean",
    "bloom",
]
def get_screen_name(name: str) -> str:
    return rename_list.get(name, name)
    

def make_violin_graph(data, aggs_sorted, name, title, preamble, func, use_custom_y=None, force_bottom=True, grid_line=False):
    graph = plt.figure(figsize=(6, 0.75 * 4))
    ax = graph.add_subplot(111)
    ax.set_title(title)
    ax.set_ylabel("processor cycles")
    ax.semilogy()
    all_latencies = []
    all_perc99 = []
    all_perc99999 = []
    all_median = []
    all_mean = []
    pos = range(1, len(aggs_sorted) + 1)
    for agg in aggs_sorted:
        latencies = np.array(data[agg], dtype=np.uint64)
        all_latencies.append(latencies)
        all_perc99.append(np.percentile(latencies, 99.9))
        all_perc99999.append(np.percentile(latencies, 99.999))
        all_median.append(np.median(latencies))
        all_mean.append(np.average(latencies))

    VERT_LINE = "#33a"
    RED_DARK = "#850e00"
    LIGHT_GRAY = "#becae4"
    GREY="#555"
    DARK_GREY="#333"
    parts = ax.violinplot(all_latencies, pos, points=250, showmedians=False, showextrema=False)
    for p in parts["bodies"]:
        p.set_facecolor(LIGHT_GRAY)
        p.set_edgecolor(GREY) 
        p.set_linewidth(1)
        

    ax.hlines(all_perc99, xmin=[x - 0.1 for x in pos], xmax=[x + 0.1 for x in pos], linewidth=1, color=VERT_LINE)
    for x, y in zip(pos, all_perc99):
        plt.text(x + 0.15, y, "99.9%", va="center", fontsize=8)

    ax.hlines(all_perc99999, xmin=[x - 0.1 for x in pos], xmax=[x + 0.1 for x in pos], linewidth=1, color=VERT_LINE)
    
    for x, y in zip(pos, all_perc99999):
        plt.text(x + 0.15, y, "99.999%", va="center", fontsize=8)

    ax.hlines(all_median, xmin=[x - 0.15 for x in pos], xmax=[x + 0.15 for x in pos], linewidth=2, color=VERT_LINE)

    ax.scatter(pos, all_mean, s=25, color=RED_DARK, zorder=3)
    
    ax.vlines(pos, ymin=[np.min(l) for l in all_latencies], 
                   ymax=[np.max(l) for l in all_latencies], color=DARK_GREY, linewidth=1.25, capstyle="round")
    if force_bottom:
        plt.ylim(bottom=1)
    if use_custom_y:
        if "y_ticks" in use_custom_y  and use_custom_y["y_ticks"] is not None:
            y_ticks = use_custom_y.get("y_ticks", Y_TICKS)
            ax.set_yticks(y_ticks)
        # ax.yaxis.set_minor_locator(LogLocator())
        max_yaxis = use_custom_y.get("max_yaxis", MAX_YAXIS)
        plt.ylim(top=max_yaxis[func])
        if "min_yaxis" in use_custom_y:
            min_yaxis = use_custom_y["min_yaxis"]
            plt.ylim(bottom=min_yaxis[func])
    if grid_line:
        GRID_COLOR = '#ffd7c7'
        plt.grid(True, axis='y', linestyle=':', color=GRID_COLOR)

    plt.setp(
        ax,
        xticks=[x for x in range(1, 1+len(aggs_sorted))],
        xticklabels=[get_screen_name(agg) for agg in aggs_sorted],
    )
    graph.autofmt_xdate()
    graph.savefig(
        "figures/" + preamble + "_violin_" + name + ".pdf", bbox_inches="tight"
    )
    plt.close(graph)

def main():
    u.create_figures_dir()
    u.set_fonts()
    plt.rcParams["xtick.labelsize"] = 12

    preamble = "latency_bike_bulk_data"

    for f in functions:
        for w in windows:
            data = {}
            exp_name = f + "_w" + str(w)
            window_size = str(w)
            exp_title = f + ", window " + window_size + " seconds"
            print(exp_name, end='', flush=True)
            for agg in aggs_sorted:
                print('.', end='', flush=True)
                data[agg] = u.read_latency_data(
                    "results/" + preamble + "_" + agg + "_" + exp_name + ".txt"
                )
            print('#', flush=True) # next line
            make_violin_graph(data, aggs_sorted, exp_name, exp_title, preamble, f, use_custom_y=True)


if __name__ == "__main__":
    main()
