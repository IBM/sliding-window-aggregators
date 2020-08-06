#!/usr/bin/env python

import sys
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.lines as plt_lines
import numpy as np
import math
import process_utility as u

aggs_sorted = [
               "daba", "daba_lite",
               "two_stacks", "two_stacks_lite",
               "flatfit",
               "bfinger4",
              ]

# in milliseconds
windows = [10,
           600000,
           819000,
           21600000
           ]

functions = [
             "sum",
             "geomean",
             "bloom",
             "relvar"
            ]

def main():
    u.create_figures_dir()
    u.set_fonts()
    plt.rcParams['xtick.labelsize'] = 12

    preamble = 'latency_mfgdebs_data'

    for f in functions:
        for w in windows:
            data = {}
            exp_name = f + '_w' + str(w)
            window_size = str(w / 1000.0)
            exp_title = f + ', window ' + window_size + ' seconds'
            for agg in aggs_sorted:
                data[agg] = u.read_latency_data('results/' + preamble + '_' + agg + '_' + exp_name + '.txt')
            print(exp_name)
            u.make_violin_graph(data, aggs_sorted, exp_name, exp_title, preamble)

if __name__ == "__main__":
    main()
