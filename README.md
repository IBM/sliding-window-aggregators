# Sliding Window Aggregators
This repo contains reference implementations of sliding window aggregation 
algorithms.

All of these algorithms require operators that are associative. We classify the
algorithms in two groups: those that require data to arrive in-order, and those
that allow data to arrive out-of-order. We refer to the algorithms that require
data to arrive in-order as FIFO algorithms, as they assume first-in, first-out
semantics. We refer to the algorithms that tolerate disordered data as general 
algorithms.

The algorithmic complexity of the algorithms is with respect to the size of the
window, *n*.

A [tutorial][swag_tutorial] and [encyclopedia][swag_encyclopedia] article
provide more background on sliding window aggregation algorithms.

# DABA
- **full name**: De-Amortized Banker's Aggregator
- **ordering**: in-order required
- **operator requirements**: associativity
- **time complexity**: worst-case O(1)
- **space requirements**: 2*n*
- **first appeared**: [Low-Latency Sliding-Window Aggregation in Worst-Case Constant Time][debs2017]
- **implementions**: [C++](cpp/src/DABA.hpp)

# DABA Lite
- **full name**: De-Amortized Banker's Aggregator Lite
- **ordering**: in-order required
- **operator requirements**: associativity
- **time complexity**: worst-case O(1)
- **space requirements**: *n* + 2
- **first appeared**: *In-Order Sliding-Window Aggregation in Worst-Case Constant Time*, under review
- **implementions**: [C++](cpp/src/DABALite.hpp)

# FiBA
- **full name**: Finger B-Tree Aggregator
- **ordering**: out-of-order allowed, assumes data is timestamped
- **operator requirements**: associativity
- **time complexity**: amortized O(log *d*) where *d* is distance newly arrived data is from 
                       being in-order, worst-case O(log *n*); for in-order data (*d* = 0),
                       amortized O(1) and worst-case O(log *n*)
- **space requirements**: O(*n*)
- **first appeared**: [Optimal and General Out-of-Order Sliding-Window Aggregation][vldb2019]
- **implementions**: [C++](cpp/src/FiBA.hpp)

# FlatFIT
- **full name**: Flat and Fast Index Traverser
- **ordering**: in-order required
- **operator requirements**: associativity
- **time complexity**: worst-case O(*n*), amortized O(1)
- **space requirements**: 2*n*
- **first appeared**: [FlatFIT: Accelerated Incremental Sliding-Window Aggregation For Real-Time Analytics][ssdbm2017]
- **implementions**: [C++](cpp/src/FlatFIT.hpp) (static windows), 
                     [C++](cpp/src/DynamicFlatFIT.hpp) (dynamic windows),
                     [Rust](rust/src/flatfit/mod.rs) (dynamic windows)

# IOA
- **full name**: Imperative Okasaki Aggregator
- **ordering**: in-order required
- **operator requirements**: associativity
- **time complexity**: worst-case O(1)
- **space requirements**: O(*n*)
- **first appeared**: [Low-Latency Sliding-Window Aggregation in Worst-Case Constant Time][debs2017]
- **implementions**: [C++](cpp/src/OkasakisQueue.hpp)

# Two-Stacks
- **full name**: Two-Stacks
- **ordering**: in-order required
- **operator requirements**: associativity
- **time complexity**: worst-case O(*n*), amortized O(1)
- **space requirements**: 2*n*
- **first appeared**: [adamax on Stack Overflow][adamax2011]
- **implementions**: [C++](cpp/src/TwoStacks.hpp),
                   [Rust](rust/src/two_stacks/mod.rs)

# Two-Stacks Lite
- **full name**: Two-Stacks Like
- **ordering**: in-order required
- **operator requirements**: associativity
- **time complexity**: worst-case O(*n*), amortized O(1)
- **space requirements**: *n* + 1 
- **first appeared**: *In-Order Sliding-Window Aggregation in Worst-Case Constant Time*, under review
- **implementions**: [C++](cpp/src/TwoStacksLite.hpp)

# Reactive
- **full name**: Reactive Aggregator
- **ordering**: out-of-order allowed
- **operator requirements**: associativity
- **time complexity**: worst-case O(log *n*)
- **space requirements**: O(*n*)
- **first appeared**: [General Incremental Sliding-Window Aggregation][vldb2015]
- **implementions**: [C++](cpp/src/Reactive.hpp),
                    [Rust](rust/src/reactive/mod.rs)

# Recalc
- **full name**: Re-Calculate From Scratch
- **ordering**: out-of-order allowed
- **operator requirements**: none
- **time complexity**: O(*n*)
- **space requirements**: *n*
- **first appeared**: no known source
- **implementations**: [C++](cpp/src/ReCalc.hpp),
                      [Rust](rust/src/recalc/mod.rs)

# SOE
- **full name**: Subtract on Evict
- **ordering**: out-of-order allowed
- **operator requirements**: associativity, invertability
- **time complexity**: worst-case O(1)
- **space requirements**: *n*
- **first appeared**: no known source
- **implementations**: [C++](cpp/src/SubtractOnEvict.hpp) (strictly in-order),
                      [Rust](rust/src/soe/mod.rs) (strictly in-order)

[swag_tutorial]: https://dl.acm.org/doi/abs/10.1145/3093742.3095107
[swag_encyclopedia]: http://hirzels.com/martin/papers/encyc18-sliding-window.pdf
[debs2017]: https://dl.acm.org/doi/abs/10.1145/3093742.3093925
[adamax2011]: https://stackoverflow.com/questions/4802038/implement-a-queue-in-which-push-rear-pop-front-and-get-min-are-all-consta
[ssdbm2017]: https://dl.acm.org/doi/abs/10.1145/3085504.3085509
[vldb2019]: http://www.vldb.org/pvldb/vol12/p1167-tangwongsan.pdf
[vldb2015]: http://www.vldb.org/pvldb/vol8/p702-tangwongsan.pdf
