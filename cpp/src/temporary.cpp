#define COLLECT_STATS

#include <iostream>
#include "AggregationFunctions.hpp"
#include "FiBA.hpp"

// This is a temporary file to detect bugs of FiBA.
// It will be removed in the future.

typedef uint64_t timestamp;
const int min_arity = 2;

int main() {
  auto fiba_btree =
      btree::MakeAggregate<Sum<int>, timestamp, min_arity, btree::finger>()(0);

  const int end = 4;

  // insert (timestamp, value) = (1, 1), (2, 2), ..., (end, end) into fiba_btree
  for (int i = 1; i <= end; ++i) {
    std::cout << "insert (" << i << ", " << i << ")...." << std::endl;
    fiba_btree.insert(i, i);
    std::cout << fiba_btree << std::endl;
  }

  // aggregation_result: 1 + 2 + ... + end
  int aggregation_result = (end + 1) * end / 2;
  for (int i = end; i >= 1; --i) {
    // check if the aggregation result is correct
    int result = fiba_btree.query();
    if (result != aggregation_result) {
      std::cout << "aggregation result is wrong: " << aggregation_result
                << "(expected) " << result << "(actual)." << std::endl;
    } else {
      std::cout << "[";
      for (int j = 1; j <= i; ++j)
        std::cout << j << (j == i ? "" : ", ");
      std::cout << "] is aggregated correctly" << std::endl;
    }

    std::cout << "evict " << i << "/" << i << "... " << std::endl;
    fiba_btree.evict(i);
    std::cout << fiba_btree << std::endl;
    aggregation_result -= i;
  }

  return 0;
}
