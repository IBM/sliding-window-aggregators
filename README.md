# Sliding Window Aggregators
This repo contains reference implementations of many different sliding window 
aggregation algorithms.

All of these algorithms require operators that are associative. We classify the
algorithms in two groups: those that require data to arrive in-order, and those
that allow data to arrive out-of-order. We call the algorithms that require
data to arrive in-order as FIFO algorithms, as they assume first-in, first-out
semantics. We call algorithms that tolerate disordered data as general 
algorithms.

The algorithmic complexity of the algorithms is with respect to the size of the
window.

A [tutorial](https://dl.acm.org/doi/abs/10.1145/3093742.3095107) and 
[encyclopedia](http://hirzels.com/martin/papers/encyc18-sliding-window.pdf)
article provide more background on sliding window aggregation algorithms.

## FIFO Algorithms
- [DABA](cpp/src/DABA.hpp) and [DABA Lite](cpp/src/DABALite.hpp) are worst-case 
  O(1). The reference paper for DABA is [Low-Latency Sliding-Window Aggregation in Worst-Case Constant Time](https://dl.acm.org/doi/abs/10.1145/3093742.3093925).
- [FlatFIT](cpp/src/FlatFIT.hpp) and [DynamicFlatFIT](cpp/src/DynamicFlatFIT.hpp) 
  are average-case O(1) and worst-case O(*n*). The reference paper for FlatFIT is 
  [FlatFIT: Accelerated Incremental Sliding-Window Aggregation For Real-Time Analytics](https://dl.acm.org/doi/abs/10.1145/3085504.3085509).
  Dynamic FlatFIT is an adaptation of FlatFIT that allows window resizing.
- [IOA](cpp/src/OkasakisQueue.hpp) is the Imperative Okasaki Aggregator and it is 
  worst-case O(1). It is based on Chris Okasaki's real time queues. The reference 
  paper for IOA is [Low-Latency Sliding-Window Aggregation in Worst-Case Constant Time](https://dl.acm.org/doi/abs/10.1145/3093742.3093925).
- [Two-Stacks](cpp/src/TwoStacks.hpp) and [Two-Stacks Lite](cpp/src/TwoStacksLite.hpp)
  are average-case O(1) and worst-case O(*n*). Two-Stacks was originally described by 
  [Jon Skeet on Stack Overflow](https://stackoverflow.com/questions/685060/design-a-stack-such-that-getminimum-should-be-o1).

## General Algorithms
- [FiBA](cpp/src/FiBA.hpp) is the Finger B-Tree Aggregator and it is 
  average-case O(log *d*) where *d* is the distance the newly arrived item is from 
  being in-order, and worst-case O(log *n*). For in-order data, this reduces to 
  average-case O(1) and worst-case O(log *n*). The reference paper for FiBA is 
  [Optimal and General Out-of-Order Sliding-Window Aggregation](http://www.vldb.org/pvldb/vol12/p1167-tangwongsan.pdf).
- [Reactive](cpp/src/Reactive.hpp) is worst-case O(log *n*). The reference paper is 
  [General Incremental Sliding-Window Aggregation](http://www.vldb.org/pvldb/vol8/p702-tangwongsan.pdf).
