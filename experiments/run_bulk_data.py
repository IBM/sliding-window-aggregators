#!/usr/bin/env python3

import run_utility as u

aggregators = [
                "bfinger2",
                "bfinger4",
                "bfinger8",
                "nbfinger2",
                "nbfinger4",
                "nbfinger8",
                "nbclassic2",
                "nbclassic4",
                "nbclassic8",
              ]

DAY = 60 * 60 * 24 # one day is 60 seconds * 60 minutes * 24 hours
durations = [DAY/4, DAY/2, 1*DAY, 2*DAY, 4*DAY, 8*DAY, 12*DAY, 16*DAY, 24*DAY, 32*DAY]


data_sets = {
    "bike": "../experiments/data/NYC-Citi-Bike/catted-citibike-tripdata.csv",
}

functions = [ 
             "sum",
             "geomean",
             "bloom",
            ] 

def main():
    u.run_data(aggregators, functions, durations, data_sets, 'bulk_data', latency='', sample_size=5)

if __name__ == "__main__":
    main()
