#!/usr/bin/env python3

import matplotlib.pyplot as plt
import process_utility as u
from run_bulk_evict_latency import functions, base_window_sizes, bulk_sizes
import math
import process_bulk_data_latency as pbdl

aggregators = [
    "daba_lite",
    # "two_stacks_lite",
    "chunked_two_stacks_lite",
    "amta",
    "bfinger4",
    "bfinger8",
    "nbfinger4",
    "nbfinger8",
]
windows = base_window_sizes
bulk_sizes = bulk_sizes  # from the run program
bulk_sizes = [1024]

def make_bulk_evict_latency_plots():
    file_preamble = 'latency_bulk_evict_opevict'

    for f in functions.keys():
        for w in windows:
            for b in bulk_sizes:
                data = {}
                exp_name = f + '_w' + str(w) + f'_d0_b{b}'
                window_size = '$2^{' + str(int(math.log(w, 2))) + '}$'
                display_bulk_size = '$2^{' + str(int(math.log(b, 2))) + '}$'
                exp_title = f + ', window ' + window_size + ', bulk_size ' + display_bulk_size
                exp_title = f 
                for agg in aggregators:
                    data[agg] = u.read_latency_data('results/' + file_preamble + '_' + agg + '_' + exp_name + '.txt')

                print(exp_name)
                our_max_y_axis = {
                    "sum": 1e8,
                    "geomean": 1e8,
                    "bloom": 1.1e9,
                }
                our_min_y_axis = {
                    "sum": 1e2,
                    "geomean": 1e2,
                    "bloom": 5e3,

                }
                pbdl.make_violin_graph(data, aggregators, exp_name, exp_title, file_preamble, f, force_bottom=False, 
                                       use_custom_y={"max_yaxis": our_max_y_axis, "min_yaxis": our_min_y_axis}, grid_line=True)
            
def main():
    u.create_figures_dir()
    u.set_fonts()
    plt.rcParams['xtick.labelsize'] = 12

    make_bulk_evict_latency_plots()
if __name__ == "__main__":
    main()
