#!/usr/bin/env python3
import run_bulk_data as rbd
import matplotlib.pyplot as plt
import numpy as np
import process_utility as u

durations = [int(d) for d in rbd.durations]

def plot_histogram(d: int, num_bins=20) -> None:
    with open(f"results/bike_evict_count_{d}.txt", "rt") as count_file:
        numbers = [int(count) for count in count_file]

    # title = f"Citi Bike"
    fig, axs = plt.subplots(1, 1, figsize=(6, 0.75*4))
    ax = axs
    # ax.set_title(title)
    ax.set_ylabel("frequency")
    ax.set_xlabel("# of records evicted")
    ax.semilogy()

    #    counts, bins = np.histogram(numbers, bins=num_bins)
    ax.hist(numbers, bins=num_bins)
    fig.savefig(
        "figures/" + "citi"+ "_evict_histo" + f"{d}" + ".pdf", bbox_inches="tight"
    )
    plt.close(fig)


def main() -> None:
    u.set_fonts()
    plt.rcParams["xtick.labelsize"] = 12
    for d in durations:
        print(f"Working on d={d}...")
        plot_histogram(d)

if __name__ == "__main__":
    main()
