#!/usr/bin/env python2.7

import run_utility as u

aggregators = [
    "daba",
    "daba_lite",
    "rb_daba_lite",
    "two_stacks",
    "two_stacks_lite",
    "rb_two_stacks_lite",
    "chunked_two_stacks_lite",
    "flatfit",
    "bclassic4",
    "bfinger4",
]

# time is in milliseconds
# we want to go up to a 6 hour window
SECOND = 10**3
MINUTE = 60*SECOND
HOUR = 60*MINUTE
durations = [10,
             100,
             SECOND,
             5*SECOND,
             10*SECOND,
             30*SECOND,
             MINUTE,
             100*SECOND,
             15*MINUTE,
             1000*SECOND,
             30*MINUTE,
             HOUR, 2*HOUR,
             10000*SECOND,
             3*HOUR, 4*HOUR, 5*HOUR, 6*HOUR]

data_sets = {
                "mfgdebs": "../experiments/data/DEBS2012-cleaned-v3.txt",
            }

functions = [
             "sum",
             "geomean",
             "bloom",
             "relvar"
            ]

def main():
    u.run_data(aggregators, functions, durations, data_sets, 'data', latency='', sample_size=10)

if __name__ == "__main__":
    main()
