#!/usr/bin/env python3

SRC_LOG = "results/citi-annotated-4"

def flush_file(output_file, size_file):
    for f in [output_file, size_file]:
        if f:
            f.close()

def main():
    output_file = None
    size_file = None
    with open(SRC_LOG, "rt") as logfile:
        for line in logfile:
            line = line.strip()
            toks = line.split(" ")
            if len(toks) >= 2 and toks[0] == "0" and toks[1][:-1]=="nbfinger":
                time = int(toks[-1])  # extract time window 
                print("Marker detected:", toks)
                flush_file(output_file, size_file)
                output_file = open(f"results/bike_evict_count_{time}.txt", "wt")
                size_file = open(f"results/bike_size_{time}.txt", "wt")
                continue
            if toks[0] == "==" and toks[1] == "bulkEvict:":
                toks = toks[-1].split("=")
                actual_count = int(toks[-1])
                print(actual_count, file=output_file)
            if toks[0] == "==" and toks[1] == "size:":
                toks = toks[-1].split("=")
                actual_count = int(toks[-1])
                print(actual_count, file=size_file)
            
        flush_file(output_file, size_file)

if __name__ == "__main__":
    main()
