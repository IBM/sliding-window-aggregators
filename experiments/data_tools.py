#!/usr/bin/env python3
import random
import math
from bisect import bisect_right
from itertools import accumulate
from typing import List

def compute_out_of_order_degree(timestamps: List[int]) -> List[int]:
    """For input a list-like array of timestamps, compute for each entry the out of
    order degree. The output is a list of out-of-order degrees.
    """

    def recursive_ood(ts, ood):
        if len(ts) > 1:
            m = len(ts)//2
            left, right = recursive_ood(ts[:m], ood), recursive_ood(ts[m:], ood)

            left_keys = [ key for _, key in left ]
            for index, key in right:
                d = len(left) - bisect_right(left_keys, key)
                ood[index] += d

            return sorted(left + right, key=lambda r: r[1])
        return ts

    ood = [0]*len(timestamps)
    indexed_ts = list(enumerate(timestamps))
    recursive_ood(indexed_ts, ood)
    return ood

def analyze_watermark(timestamps):
    """For input a list-like array of timestamps, compute for each entry how far
      (in time unit) it is out of order by (i.e., compared to the high-water
      mark)
    """

    wm_gaps = [0] * len(timestamps)
    high_wm, *_ = timestamps
    for index, ts in enumerate(timestamps):
        high_wm = max(ts, high_wm)
        wm_gaps[index] = high_wm - ts
    return wm_gaps

def watermarks(timestamps):
    """For input a list-like array of timestamps, compute for each entry the
      high-water mark at that point.
    """

    return list(accumulate(timestamps, max))

def threshold_sample(p, a, thres, series):
    """Generate samples from series where admission of x in series is made via the
    formula prob = p * (1 + a*log10(1+x)) or if x exceeds the threshold thres.
    """
    sampled = []
    for i, d in enumerate(series):
        if random.random() < p*(1+a*math.log10(d+1)) or d > thres:
            sampled.append((i,d))
    return sampled

if __name__ == '__main__':
    sample = [300, 1, 2, 125, 100, 303, 4, 9, 300]
    print(compute_out_of_order_degree(sample))
    print(analyze_watermark(sample))
    print(watermarks(sample))
