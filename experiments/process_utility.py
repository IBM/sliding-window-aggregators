import csv, time, sys, math, collections, os
import numpy as np
import matplotlib.pyplot as plt
from typing import Dict

KB = 1024
MB = KB * KB
MILLION = 1000000

Data = collections.namedtuple("Data", ["agg", "degree", "window", "avg", "std"])
BulkData = collections.namedtuple(
    "BulkData", ["agg", "degree", "window", "bulk_size", "avg", "std"]
)
SharedData = collections.namedtuple(
    "SharedData", ["agg", "big_window", "small_window", "avg", "std"]
)
Aggregator = collections.namedtuple("Aggregator", ["name", "style", "color"])


def get_screen_name(name):
    if name == "bfinger4":
        return "fiba4"
    return name


def set_fonts():
    plt.rcParams["pdf.fonttype"] = 42  # avoid type 3 fonts
    plt.rcParams["ps.fonttype"] = 42  # avoid type 3 fonts

    plt.rcParams["font.size"] = 18
    plt.rcParams["ytick.labelsize"] = 13
    plt.rcParams["xtick.labelsize"] = 13
    plt.rcParams["legend.fontsize"] = 16
    plt.rcParams["axes.titlesize"] = 22


def read_latency_data(name):
    with open(name, "r") as file:
        data = []
        for line in file:
            data.append(int(line))
    return data


def create_figures_dir():
    if not os.path.exists("figures"):
        os.makedirs("figures")


def read_bulk_throughput_data(preamble, agg, functions):
    with open("results/" + preamble + "_" + agg + ".csv", "r") as file:
        reader = csv.reader(file)
        recursive_dict = lambda: collections.defaultdict(recursive_dict)
        data = collections.defaultdict(recursive_dict)

        for row in reader:
            f = row[0]
            w = int(row[1])
            d = int(row[2])
            bulk_size = int(row[3])
            data[f][d][w][bulk_size] = BulkData(
                agg,
                d,
                w,
                bulk_size,
                np.average([functions[f][0] / float(x) for x in row[4:]]),
                np.std([functions[f][0] / float(x) for x in row[4:]]),
            )
    return data


def read_throughput_data(preamble, agg, functions):
    with open("results/" + preamble + "_" + agg + ".csv", "r") as file:
        reader = csv.reader(file)
        data = collections.defaultdict(lambda: collections.defaultdict(dict))

        for row in reader:
            f = row[0]
            w = int(row[1])
            d = int(row[2])
            data[f][d][w] = Data(
                agg,
                d,
                w,
                np.average([functions[f] / float(x) for x in row[3:]]),
                np.std([functions[f] / float(x) for x in row[3:]]),
            )
    return data


def read_throughput(preamble, agg, functions):
    with open("results/" + preamble + "_" + agg + ".csv", "r") as file:
        reader = csv.reader(file)
        data = collections.defaultdict(lambda: collections.defaultdict(dict))

        for row in reader:
            f = row[0]
            w = int(row[1])
            data[f][w] = Data(
                agg,
                w,
                -1,  # no degree
                np.average([functions[f] / float(x) for x in row[2:]]),
                np.std([functions[f] / float(x) for x in row[2:]]),
            )
    return data


def read_citibike_throughput(agg, functions):
    with open("results/bike_data" + "_" + agg + ".csv", "r") as file:
        reader = csv.reader(file)
        data = collections.defaultdict(lambda: collections.defaultdict(dict))

        for row in reader:
            f = row[0]
            w = str(int(row[1]))  # already in seconds

            # the remainder of the row is a flat list of (num items processed, exec time) pairs
            remainder = zip(
                *[iter(row[2:])] * 2
            )  # tuples of (num items processed, execution time)
            throughputs = [int(num) / float(time) for num, time in remainder]
            data[f][w] = Data(
                agg, w, -1, np.average(throughputs), np.std(throughputs)  # no degree
            )
    return data

def read_mfgdebs_throughput(agg, functions):
    with open("results/mfgdebs_data" + "_" + agg + ".csv", "r") as file:
        reader = csv.reader(file)
        data = collections.defaultdict(lambda: collections.defaultdict(dict))

        for row in reader:
            f = row[0]
            w = str(int(row[1]) / 1000.0)  # convert milliseconds to seconds

            # the remainder of the row is a flat list of (num items processed, exec time) pairs
            remainder = zip(
                *[iter(row[2:])] * 2
            )  # tuples of (num items processed, execution time)
            throughputs = [int(num) / float(time) for num, time in remainder]
            data[f][w] = Data(
                agg, w, -1, np.average(throughputs), np.std(throughputs)  # no degree
            )
    return data


def read_shared_throughput_data(preamble, agg, functions):
    with open("results/" + preamble + "_" + agg + ".csv", "r") as file:
        reader = csv.reader(file)
        data = collections.defaultdict(lambda: collections.defaultdict(dict))

        for row in reader:
            f = row[0]
            bw = int(row[1])
            sw = int(row[2])
            data[f][bw][sw] = SharedData(
                agg,
                bw,
                sw,
                np.average([(functions[f]) / float(x) for x in row[2:]]),
                np.std([(functions[f]) / float(x) for x in row[2:]]),
            )
    return data


def func_varying_small_window(agg_to_func):
    ret = collections.defaultdict(
        lambda: collections.defaultdict(lambda: collections.defaultdict(dict))
    )
    for agg, func in agg_to_func.items():
        for f, small_window in func.items():
            for sw, data in small_window.items():
                ret[f][agg][sw] = data
    return ret


def make_throughput_graph(title, preamble, varying, function, aggs, aggregators):
    graph = plt.figure()
    ax = graph.add_subplot(111)
    ax.set_title(title + " " + function)
    ax.set_xlabel(varying)
    ax.set_xscale("log", basex=2)
    ax.set_xticks(
        [
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

    for aggregator in aggregators:
        data = aggs[aggregator.name]
        x_axis = np.array(sorted(data.keys()))
        throughput = np.array([data[w].avg for w in sorted(data.keys())]) / 1e6

        stddev = np.array([data[w].std for w in sorted(data.keys())]) / 1e6
        ax.errorbar(
            x_axis,
            throughput,
            yerr=stddev,
            label=get_screen_name(aggregator.name),
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
        bbox_to_anchor=(0.5, 1.55),
    )

    graph.savefig("figures/" + preamble + "_" + function + ".pdf")



def make_small_window_varying_graphs(
    title, preamble, varying, aggregators, func_to_data
):
    for f, aggs in func_to_data.items():
        make_throughput_graph(title, preamble, varying, f, aggs, aggregators)


def make_mfgdebs_small_window_varying_graphs(title, varying, aggregators, func_to_data):
    for f, aggs in func_to_data.items():
        make_data_throughput_graph(title, "mfgdebs", varying, f, aggs, aggregators)


def make_violin_graph(data, aggs_sorted, name, title, preamble):
    graph = plt.figure(figsize=(6, 0.75 * 4))
    ax = graph.add_subplot(111)
    ax.set_title(title)
    ax.set_ylabel("processor cycles")
    ax.semilogy()
    all_latencies = []
    all_perc99 = []
    all_perc99999 = []
    pos = range(1, len(aggs_sorted) + 1)
    for agg in aggs_sorted:
        latencies = np.array(data[agg], dtype=np.uint64)
        all_latencies.append(latencies)
        all_perc99.append(np.percentile(latencies, 99.9))
        all_perc99999.append(np.percentile(latencies, 99.999))
    parts = ax.violinplot(all_latencies, pos, points=200, showmedians=True)
    for p in parts["bodies"]:
        p.set_facecolor("#becae4")
    parts["cmedians"].set_edgecolor("#894a2f")

    ax.hlines(all_perc99, xmin=[x - 0.1 for x in pos], xmax=[x + 0.1 for x in pos])
    for x, y in zip(pos, all_perc99):
        plt.text(x + 0.15, y, "99.9%", va="center", fontsize=10)

    ax.hlines(all_perc99999, xmin=[x - 0.1 for x in pos], xmax=[x + 0.1 for x in pos])
    for x, y in zip(pos, all_perc99999):
        plt.text(x + 0.15, y, "99.999%", va="center", fontsize=10)

    plt.setp(
        ax,
        xticks=[x + 1 for x in range(len(aggs_sorted))],
        xticklabels=[get_screen_name(agg) for agg in aggs_sorted],
    )
    graph.autofmt_xdate()
    graph.savefig(
        "figures/" + preamble + "_violin_" + name + ".pdf", bbox_inches="tight"
    )
    plt.close(graph)
