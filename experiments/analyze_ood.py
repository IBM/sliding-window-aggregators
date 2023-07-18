#!/usr/bin/env python3
from data_tools import compute_out_of_order_degree
import pandas as pd
import numpy as np 

DATAFILE = "data/NYC-Citi-Bike/catted-citibike-tripdata.csv"
OUTPUT = "results/citi-ood.txt"
def citibike_start_timestamps(pathspec):
    df = pd.read_csv(pathspec, header=0) # row 0 is the header
    # divide by 10^9 so the unit is second.
    start_times = pd.to_datetime(df['starttime']).astype(np.int64)//1e9
    return start_times

def main():
    print(f"Loading CitiBike ({DATAFILE = })")
    start_times = citibike_start_timestamps(DATAFILE) 
    print(f"Computing out of order degress (n={len(start_times)})")
    out_of_order_degrees = compute_out_of_order_degree(start_times)
    with open(OUTPUT, "wt") as writer:
        for oo_degree in out_of_order_degrees:
            print(oo_degree, file=writer)
    

if __name__ == "__main__":
    main()
