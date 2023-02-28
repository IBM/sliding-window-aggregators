#!/usr/bin/env python3
import run_utility as u

aggregators = [
#                "bfinger2",
                "bfinger4",
                "bfinger8",
#                "nbfinger2",
                "nbfinger4",
                "nbfinger8",
#                "nbclassic2",
                "nbclassic4",
                "nbclassic8",
              ]

aggregators = aggregators[::-1]  ## REMOVE ME!


DAY = 60 * 60 * 24 # one day is 60 seconds * 60 minutes * 24 hours
# durations = [DAY/2, 1*DAY, 2*DAY, 16*DAY, 32*DAY]
durations = [1*DAY, 16*DAY]


data_sets = {
    "bike": "../experiments/data/NYC-Citi-Bike/catted-citibike-tripdata.csv",
}

functions = [ 
             "sum",
             "geomean",
             "bloom",
            ] 

MODE = "individual"

def main():
    print(f"mode: {MODE}")
    if MODE == "individual":
        for agg in aggregators:
            for fun in functions:
                for dur in durations:
                    print(f"{agg} {fun} {dur}", flush=True)
                    u.run_data([agg], [fun], [dur], data_sets, 'bulk_data', latency='latency', sample_size=1)
    else:
        u.run_data(aggregators, functions, durations, data_sets, 'bulk_data', latency='latency', sample_size=1)

if __name__ == "__main__":
    main()
