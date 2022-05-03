#!/usr/bin/env python3

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
               "rb_two_stacks_lite", "chunked_two_stacks_lite",
               "flatfit",
               "bfinger4",
              ]

windows = [16*u.KB]

functions = [
             "sum",
             "geomean",
             "bloom",
            ]

def main():
    u.create_figures_dir()
    u.set_fonts()
    plt.rcParams['xtick.labelsize'] = 12

    preamble = 'latency'

    for f in functions:
        for w in windows:
            data = {}
            exp_name = f + '_w' + str(w)
            window_size = '$2^{' + str(int(math.log(w, 2))) + '}$'
            exp_title = f + ', window ' + window_size
            for agg in aggs_sorted:
                data[agg] = u.read_latency_data('results/' + preamble + '_' + agg + '_' + exp_name + '.txt')
            print(exp_name)
            u.make_violin_graph(data, aggs_sorted, exp_name, exp_title, preamble)

if __name__ == "__main__":
    main()
