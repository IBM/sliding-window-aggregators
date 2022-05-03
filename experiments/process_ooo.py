#!/usr/bin/env python3

import sys, collections
import matplotlib.pyplot as plt
import math
import numpy as np
import process_utility as u
from run_ooo import base_iterations

Line = collections.namedtuple('Line', ['style', 'color'])

aggregators = {#"bfinger2":   Line('-',  'red'),
               "bfinger4":   Line('-',  'blue'),
               "bfinger8":   Line('-',  'green'),
               #"bclassic2":  Line('--',  'red'),
               "bclassic4":  Line('--',  'blue'),
               "bclassic8":  Line('--',  'green'),
               }
aggs_sorted = [#"bclassic2", 
        "bclassic4", "bclassic8", 
        #"bfinger2", 
        "bfinger4", "bfinger8"]

distances = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 
           1*u.KB, 2*u.KB, 4*u.KB, 8*u.KB, 16*u.KB, 32*u.KB, 64*u.KB, 128*u.KB, 256*u.KB, 512*u.KB,
           1*u.MB, 2*u.MB, 4*u.MB]

windows = [4*u.MB] #[1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 
           #1*u.KB, 2*u.KB, 4*u.KB, 8*u.KB, 16*u.KB, 32*u.KB, 64*u.KB, 128*u.KB, 256*u.KB, 512*u.KB,
           #1*u.MB, 2*u.MB, 4*u.MB]

functions = {"sum": base_iterations,
             "geomean": base_iterations,
             "bloom": base_iterations/100,
            }

def make_throughput_graph(preamble, function, i, mapping, constant, varying):
    graph = plt.figure()
    ax = graph.add_subplot(111)
    ax.set_title('OoO '+ function + ', ' + constant + ' ' + '$2^{' + str(int(math.log(i, 2))) + '}$')
    ax.set_xlabel(varying)
    ax.set_xscale('log', basex=2)
    ax.set_xlim(right=2**22)
    ax.set_xticks([2, 2**3, 2**5, 2**7, 2**9, 2**11, 2**13, 2**15, 2**17, 2**19, 2**21]) 
    ax.set_ylabel('throughput [million items/s]')
    #for key, data in mapping.items():
    for agg in aggs_sorted:
        data = mapping[agg]
        if len(data) <= 1:
            return
        x_axis = np.array(sorted(data.keys()))
        throughput = np.array([data[w].avg for w in sorted(data.keys())])/1e6

        stddev = np.array([data[w].std for w in sorted(data.keys())])/1e6
        ax.errorbar(x_axis, throughput, yerr=stddev, 
                    label=agg, linewidth=2, linestyle=aggregators[agg].style, color=aggregators[agg].color)

    # shrink axis by 25% and put legend outside and to the right
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 1.1, box.height * 0.85])
    ax.legend(frameon=False, ncol=3, loc='upper center', columnspacing=0.5, bbox_to_anchor=(0.5, 1.4))

    graph.savefig('figures/' + preamble + '_' + constant + str(i) + '_' + function + '.pdf')
    plt.close(graph)

def make_window_varying_graph(preamble, func_to_data):
    for f, distances in func_to_data.items():
        for d, aggs in distances.items():
            make_throughput_graph(preamble, f, d, aggs, constant='distance', varying='window size in data items')

def make_distance_varying_graph(preamble, func_to_data):
    for f, windows in func_to_data.items():
        for w, aggs in windows.items():
            make_throughput_graph(preamble, f, w, aggs, constant='window', varying='out-of-order distance')

def func_varying_window(agg_to_func):
    ret = collections.defaultdict(lambda: collections.defaultdict(lambda: collections.defaultdict(dict)))
    for agg, func in agg_to_func.items():
        for f, distance in func.items():
            for d, window in distance.items():
                for w, data in window.items():
                    ret[f][d][agg][w] = data
    return ret

def func_varying_distance(agg_to_func):
    ret = collections.defaultdict(lambda: collections.defaultdict(lambda: collections.defaultdict(dict)))
    for agg, func in agg_to_func.items():
        for f, distance in func.items():
            for d, window in distance.items():
                for w, data in window.items():
                    ret[f][w][agg][d] = data
    return ret

def main():
    u.create_figures_dir()
    u.set_fonts()

    agg_to_func = collections.defaultdict(lambda: collections.defaultdict(dict))
    for agg in aggregators.keys():
        agg_to_func[agg] = u.read_throughput_data('ooo', agg, functions)

    make_window_varying_graph('ooo_window', func_varying_window(agg_to_func))
    make_distance_varying_graph('ooo_distance', func_varying_distance(agg_to_func))

if __name__ == "__main__":
    main()
