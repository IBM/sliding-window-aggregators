#!/usr/bin/env python3
import run_bulk_data as rbd
import matplotlib.pyplot as plt
import numpy as np
import process_utility as u

def plot_histogram(num_bins=20) -> None:
    with open(f"results/citi-ood.txt", "rt") as count_file:
        numbers = [int(count) for count in count_file]

    # title = f"Citi Bike"
    fig, axs = plt.subplots(1, 1, figsize=(6, 0.75*4))
    ax = axs
    # ax.set_title(title)
    ax.set_ylabel("frequency")
    ax.set_xlabel("out-of-order distance (ood)")
    ax.semilogy()
    ax.set_xlim(0, 9.99e5)

    #    counts, bins = np.histogram(numbers, bins=num_bins)
    ax.hist(numbers, bins=num_bins)
    ax.ticklabel_format(axis="x", style="sci", scilimits=(0,0), useMathText=True)
    fig.savefig(
        "figures/" + "citi"+ "_ood_histo" + ".pdf", bbox_inches="tight"
    )
    plt.close(fig)


def main() -> None:
    u.set_fonts()
    plt.rcParams["xtick.labelsize"] = 12
    plot_histogram()

if __name__ == "__main__":
    main()
