#!/usr/bin/env python3

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
             10*MINUTE,
             819*SECOND,
             6*HOUR
            ]

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
    u.run_data(aggregators, functions, durations, data_sets, 'data', 'latency', 1)

if __name__ == "__main__":
    main()
