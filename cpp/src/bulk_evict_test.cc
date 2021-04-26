#include "FiBA.hpp"
#include "AggregationFunctions.hpp"

int main() {
  for (int i=0, n=1000; i<n; i++) {
    auto tree = btree::Aggregate<int, 2, btree::finger, Collect<int>>::makeRandomTree(Collect<int>(), 4);
    int maxTime = tree->youngest();
    int minTime = rand() % maxTime;
    std::cout << "iteration " << i << ", minTime " << minTime << ", maxTime " << maxTime << std::endl;
    tree->evictUpTo(minTime);
    auto collected = tree->query();
    int pred = minTime;
    for (auto j=collected.begin(); j!=collected.end(); j++) {
      if (pred + 1 != *j) {
	for (auto k=collected.begin(); k!=collected.end(); k++)
	  std::cout << *k << " ";
	std::cout << *tree;
	assert(false);
      }
      pred = *j;
    }
    assert(pred == maxTime);
    delete tree;
  }
  std::cout << "bulk_evict_test passed" << std::endl;
  return 0;
}
