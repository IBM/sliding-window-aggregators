#include "FiBA.hpp"
#include "AggregationFunctions.hpp"

template<int minArity>
void test_one(int height, int iteration, bool verbose) {
  typedef btree::Aggregate<int, minArity, btree::finger, Collect<int>> Tree;
  auto tree = Tree::makeRandomTree(Collect<int>(), height);
  int maxTime = tree->youngest();
  int minTime = rand() % maxTime;
  if (verbose) std::cout << "iteration " << iteration << ", minTime " << minTime << ", maxTime " << maxTime << std::endl;
  if (verbose) std::cout << *tree;
  tree->evictUpTo(minTime);
  auto collected = tree->query();
  int pred = minTime;
  for (auto j=collected.begin(); j!=collected.end(); j++) {
    if (pred + 1 != *j) {
	std::cerr << "iteration " << iteration << " collected items and tree ";
	for (auto k=collected.begin(); k!=collected.end(); k++)
	  std::cerr << *k << " ";
	std::cerr << std::endl << *tree;
	assert(false);
    }
    pred = *j;
  }
  assert(pred == maxTime);
  delete tree;
}

int main() {
  for (int i=0, n=100; i<n; i++)
    test_one<2>(3, i, false);
  for (int i=0, n=1000; i<n; i++)
    test_one<2>(4, i, false);
  for (int i=0, n=100; i<n; i++)
    test_one<3>(4, i, false);
  for (int i=0, n=100; i<n; i++)
    test_one<4>(4, i, false);
  for (int i=0, n=500; i<n; i++)
    test_one<2>(5, i, false);
  for (int i=0, n=250; i<n; i++)
    test_one<2>(6, i, false);
  for (int i=0, n=100; i<n; i++)
    test_one<2>(7, i, false);
  std::cout << "bulk_evict_test passed" << std::endl;
  return 0;
}
