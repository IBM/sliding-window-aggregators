#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "AggregationFunctions.hpp"
#include "FiBA.hpp"
#include "ReCalc.hpp"
#include "BulkAdapter.hpp"

typedef long long int timestamp;

using namespace std;
template <class AGG>
void brute_bulkInsert(AGG &agg, vector<pair<timestamp, int>> &bulk) {
  for (auto [time, val] : bulk) {
    agg.insert(time, val);
  }
}

/*
 * simple_fixed_bulk - inserts two bulks of items into various locations
 * throughout the tree.
 */
template <int minArity> void simple_fixed_bulk() {
  // auto f = Sum<int>();
  // auto bfinger_agg = btree::make_aggregate<timestamp, minArity,
  // btree::finger>(f, identity); auto ref_agg =
  // btree::make_aggregate<timestamp, minArity, btree::finger>(f, identity);
  auto f = Collect<int>();
  auto identity = f.identity;
  auto bfinger_agg =
      btree::make_aggregate<timestamp, minArity, btree::finger>(f, identity);
  auto ref_agg =
      btree::make_aggregate<timestamp, minArity, btree::finger>(f, identity);

  vector<pair<timestamp, int>> initial{
      make_pair(1, 101), make_pair(500, 100 + 500), make_pair(1000, 100 + 1000),
      make_pair(1500, 100 + 1500)};
  for (auto [time, val] : initial) {
    bfinger_agg.insert(time, val);
    ref_agg.insert(time, val);
  }
  assert(ref_agg.query() == bfinger_agg.query());

  vector<pair<timestamp, int>> bulkOne{
      make_pair(5, 105),           make_pair(507, 100 + 507),
      make_pair(509, 100 + 509),   make_pair(511, 100 + 511),
      make_pair(515, 100 + 515),   make_pair(516, 100 + 516),
      make_pair(517, 100 + 517),   make_pair(518, 100 + 518),
      make_pair(1700, 100 + 1700), make_pair(1701, 100 + 1701),
      make_pair(1702, 100 + 1702), make_pair(1703, 100 + 1703)};

  bfinger_agg.bulkInsert(bulkOne);
  brute_bulkInsert(ref_agg, bulkOne);

  assert(ref_agg.query() == bfinger_agg.query());

  vector<pair<timestamp, int>> bulkTwo{
      make_pair(6, 100 + 6),       make_pair(7, 100 + 7),
      make_pair(8, 100 + 8),       make_pair(9, 100 + 9),
      make_pair(1705, 100 + 1705), make_pair(1706, 100 + 1706),
      make_pair(1707, 100 + 1707), make_pair(1708, 100 + 1708),
      make_pair(1709, 100 + 1709), make_pair(1710, 100 + 1710),
      make_pair(1711, 100 + 1711)};
  bfinger_agg.bulkInsert(bulkTwo);
  brute_bulkInsert(ref_agg, bulkTwo);

  auto ans1 = bfinger_agg.query();
  std::cout << "ans = " << ans1 << std::endl;
  auto ans0 = ref_agg.query();
  std::cout << "ref = " << ans0 << std::endl;

  assert(ref_agg.query() == bfinger_agg.query());
}

/*
 * simple_fixed_extreme - inserts bulks of items into either left or right ends
 * to test inserting in the extreme cases.
 */
template <int minArity> void simple_fixed_extreme() {
  cout << "=== simple_fixed_extreme:" << endl;
  auto f = Collect<int>();
  auto identity = f.identity;
  auto bfinger_agg =
      btree::make_aggregate<timestamp, minArity, btree::finger>(f, identity);
  auto ref_agg =
      btree::make_aggregate<timestamp, minArity, btree::finger>(f, identity);

  vector<pair<timestamp, int>> initial{
      make_pair(400, 101), make_pair(500, 100 + 500),
      make_pair(1000, 100 + 1000), make_pair(1500, 100 + 1500)};
  for (auto [time, val] : initial) {
    bfinger_agg.insert(time, val);
    ref_agg.insert(time, val);
  }
  assert(ref_agg.query() == bfinger_agg.query());

  cout << "---- Bulk #1" << endl;
  // Bulk 1: Insert at front (touching left spine)
  {
    vector<pair<timestamp, int>> bulk;
    for (int t = 0; t < 200; t += 3)
      bulk.push_back(make_pair(t, 100 + t));
    bfinger_agg.bulkInsert(bulk.begin(), bulk.end());
    brute_bulkInsert(ref_agg, bulk);
  }
  assert(ref_agg.query() == bfinger_agg.query());

  cout << "---- Bulk #2" << endl;
  // Bulk 2: Front (but interleaved with earlier)
  {
    vector<pair<timestamp, int>> bulk;
    for (int t = 1; t < 200; t += 3)
      bulk.push_back(make_pair(t, 100 + t));
    bfinger_agg.bulkInsert(bulk.begin(), bulk.end());
    brute_bulkInsert(ref_agg, bulk);
  }
  assert(ref_agg.query() == bfinger_agg.query());

  cout << "---- Bulk #3" << endl;
  // Bulk 3: Far right
  {
    vector<pair<timestamp, int>> bulk;
    for (int t = 2000; t < 5000; t += 2)
      bulk.push_back(make_pair(t, 100 + t));
    bfinger_agg.bulkInsert(bulk);
    brute_bulkInsert(ref_agg, bulk);
  }
  assert(ref_agg.query() == bfinger_agg.query());

  cout << "---- Bulk #4" << endl;
  // Bulk 4: Interleaving
  {
    vector<pair<timestamp, int>> bulk;
    for (int t = 501; t < 1000; t += 2)
      bulk.push_back(make_pair(t, 100 + t));
    for (int t = 1100; t < 1500; t += 2)
      bulk.push_back(make_pair(t, 100 + t));
    for (int t = 2001; t < 4000; t += 2)
      bulk.push_back(make_pair(t, 100 + t));
    bfinger_agg.bulkInsert(bulk);
    brute_bulkInsert(ref_agg, bulk);
  }
  assert(ref_agg.query() == bfinger_agg.query());
}

template <int minArity, bool allowRepeats = false, class F, class Tree>
void execute_random_inserts(F f, Tree& bfinger_agg_, Tree& ref_agg_,
                            int numReps, int seed=0x212) {
  using OurTree = btree::Aggregate<timestamp, minArity, btree::finger, F>;
  OurTree& bfinger_agg = bfinger_agg_;
  OurTree& ref_agg = ref_agg_;

  const timestamp tLow = -10, tHigh = 100000;
  const int nLow = 1, nHigh = 1000;
  set<timestamp> used;
  auto identity = f.identity;

  srand(seed);
  for (int repNo = 0; repNo < numReps; repNo++) {
    // cout << "----- Round #" << repNo << endl;
    auto available = set<timestamp>();
    for (timestamp t = tLow; t <= tHigh; t++) {
      if (allowRepeats || used.find(t) == used.end())
        available.insert(t);
    }
    int count = nLow + rand() % (nHigh - nLow);
    int expectedCount = count;
    vector<pair<timestamp, int>> bulk;
    while (count > 0) {
      int tgt = rand() % available.size();
      auto it = available.begin();
      for (; tgt > 0; it++, tgt--) {
      }

      bulk.push_back(make_pair(*it, *it % 101));
      if (!allowRepeats)
        available.erase(it);
      count--;
    }
    sort(bulk.begin(), bulk.end());
    assert(bulk.size() == expectedCount);
    assert(bulk.size() > 0);
    // cout << "About to insert: n = " << bulk.size() << endl;
    bfinger_agg.bulkInsert(bulk);
    brute_bulkInsert(ref_agg, bulk);
    auto refAns = ref_agg.query();
    auto aggAns = bfinger_agg.query();
    // cout << "refAns = " << refAns << ", aggAns = " << aggAns << endl;
    bool ok = refAns == aggAns;
    // cout << "--> " << ok << endl;
    assert(ok);
    if (!allowRepeats)
      for (auto [t, _val] : bulk) {
        used.insert(t);
      }
  }
}

template <int minArity, bool allowRepeats = false, class F>
void random_bulk_inserts(F f, int numReps = 30) {
  auto bfinger_agg =
      btree::make_aggregate<timestamp, minArity, btree::finger>(f, f.identity);
  auto ref_agg =
      btree::make_aggregate<timestamp, minArity, btree::finger>(f, f.identity);

  execute_random_inserts<minArity>(f, bfinger_agg, ref_agg, numReps);
}

template <int minArity, class F>
void random_bulk_inserts_from_random_tree(F f, int height, int iter, int numReps=10) {
  using Tree = btree::Aggregate<timestamp, minArity, btree::finger, F>;
  auto identity = f.identity;
  auto bfinger_aggP = Tree::makeRandomTree(f, height);
  auto ref_agg =
      btree::make_aggregate<timestamp, minArity, btree::finger>(f, identity);

  // dump everything we have in the random tree into the reference tree
  bfinger_aggP->walk(
      [&](auto const t, auto const v) { ref_agg.insert_lifted(t, v); });

  execute_random_inserts<minArity>(f, *bfinger_aggP, ref_agg, numReps, iter);

  delete bfinger_aggP;
}

template <int minArity>
bool checkContents(
    btree::Aggregate<int, minArity, btree::finger, Collect<int>> *tree,
    int minTime, int maxTime) {
  auto collected = tree->query();
  int pred = minTime;
  for (auto j = collected.begin(); j != collected.end(); j++) {
    if (pred + 1 != *j) {
      std::cerr << "collected items ";
      for (auto k = collected.begin(); k != collected.end(); k++)
        std::cerr << *k << " ";
      std::cerr << std::endl << "do not match tree " << *tree;
      return false;
    }
    pred = *j;
  }
  return pred == maxTime;
}

template <int minArity> void test_one(int height, int iteration, bool verbose) {
  typedef btree::Aggregate<int, minArity, btree::finger, Collect<int>> Tree;
  auto tree = Tree::makeRandomTree(Collect<int>(), height);
  int maxTime = tree->youngest();
  int minTime = rand() % maxTime;
  if (verbose)
    std::cout << "iteration " << iteration << ", minTime " << minTime
              << ", maxTime " << maxTime << std::endl;
  if (verbose)
    std::cout << *tree;
  tree->bulkEvict(minTime);
  if (!checkContents(tree, minTime, maxTime)) {
    std::cerr << "iteration " << iteration << " failed bulkEvict" << std::endl;
    assert(false);
  }
  if (iteration < 3) {
    if (true)
      std::cout << "height " << height << ", iteration " << iteration
                << ", minTime " << minTime << ", maxTime " << maxTime
                << std::endl;
    for (int i = 1 + maxTime, n = 2 * maxTime; i <= n; i++)
      tree->insert(i, i);
    if (!checkContents(tree, minTime, 2 * maxTime)) {
      std::cerr << "iteration " << iteration << " failed inserts" << std::endl;
      assert(false);
    }
  }
  delete tree;
}
void bulk_evict_tests() {
  for (int i = 0, n = 100; i < n; i++)
    test_one<2>(3, i, false);
  for (int i = 0, n = 1000; i < n; i++)
    test_one<2>(4, i, false);
  for (int i = 0, n = 100; i < n; i++)
    test_one<3>(4, i, false);
  for (int i = 0, n = 100; i < n; i++)
    test_one<4>(4, i, false);
  for (int i = 0, n = 500; i < n; i++)
    test_one<2>(5, i, false);
  for (int i = 0, n = 250; i < n; i++)
    test_one<2>(6, i, false);
  for (int i = 0, n = 100; i < n; i++)
    test_one<2>(7, i, false);
  std::cout << "bulk_evict_test passed" << std::endl;
}

void bulk_insert_tests() {
  simple_fixed_bulk<2>();
  simple_fixed_extreme<2>();
  random_bulk_inserts<2>(Sum<int>(), 30);
  random_bulk_inserts<2>(Collect<int>(), 10);
  simple_fixed_bulk<4>();
  simple_fixed_extreme<4>();
  random_bulk_inserts<4>(Sum<int>(), 30);
  random_bulk_inserts<4>(Collect<int>(), 10);
  simple_fixed_bulk<5>();
  simple_fixed_extreme<5>();
  random_bulk_inserts<5>(Sum<int>(), 30);
  random_bulk_inserts<5>(Collect<int>(), 10);
  simple_fixed_bulk<8>();
  simple_fixed_extreme<8>();
  random_bulk_inserts<8>(Sum<int>(), 30);
  random_bulk_inserts<8>(Collect<int>(), 10);
  std::cout << "bulk_insert_test passed" << std::endl;
}

/*
 * bulk_insert_with_repeats_tests - bulk insert tests with potentially duplicate
 * timestamps.
 */
void bulk_insert_with_repeats_tests() {
  random_bulk_inserts<2, true>(Collect<int>(), 15);
  random_bulk_inserts<4, true>(Collect<int>(), 15);
  random_bulk_inserts<8, true>(Collect<int>(), 15);
  std::cout << "bulk_insert_with_repeats_tests passed" << std::endl;
}


/*
 * bulk_insert_from_random_trees - bulk insert tests starting from non-empty
 * randomly-generated trees of different arities.
 */
void bulk_insert_from_random_trees() {
  const int N = 20;
  for (int i=0;i<N;i++)
    random_bulk_inserts_from_random_tree<2>(Collect<int>(), 3, i, 2);
  for (int i=0;i<N;i++)
    random_bulk_inserts_from_random_tree<2>(Collect<int>(), 4, i, 2);
  for (int i=0;i<N;i++)
    random_bulk_inserts_from_random_tree<2>(Collect<int>(), 5, i, 2);
  for (int i=0;i<N;i++)
    random_bulk_inserts_from_random_tree<2>(Collect<int>(), 7, i, 2);
  for (int i=0;i<N;i++)
    random_bulk_inserts_from_random_tree<3>(Collect<int>(), 4, i, 2);
  for (int i=0;i<N;i++)
    random_bulk_inserts_from_random_tree<4>(Collect<int>(), 3, i, 2);
  for (int i=0;i<N;i++)
    random_bulk_inserts_from_random_tree<8>(Collect<int>(), 3, i, 2);
  std::cout << "bulk_insert_from_random_trees passed" << std::endl;
}

template <int minArity, class F>
void bulk_insert_from_adapter(F f) {
  auto bfinger_wrapped = btree::make_bulk_aggregate<timestamp, minArity, btree::finger, F, typename F::Partial>(f, f.identity);

  vector<pair<timestamp, int>> bulkOne{
      make_pair(5, 105),           make_pair(507, 100 + 507),
      make_pair(509, 100 + 509),   make_pair(511, 100 + 511),
      make_pair(515, 100 + 515),   make_pair(516, 100 + 516),
      make_pair(517, 100 + 517),   make_pair(518, 100 + 518),
      make_pair(1700, 100 + 1700), make_pair(1701, 100 + 1701),
      make_pair(1702, 100 + 1702), make_pair(1703, 100 + 1703)};

  bfinger_wrapped.bulkInsert(bulkOne);
  // reset
  bfinger_wrapped = btree::make_aggregate<timestamp, minArity, btree::finger, F, typename F::Partial, true>(f, f.identity);

  bfinger_wrapped.bulkInsert(bulkOne.begin(), bulkOne.end());
}

template <int minArity, class F>
void bulk_evict_from_adapter(F f) {
  auto bfinger_wrapped = btree::make_bulk_aggregate<timestamp, minArity, btree::finger, F, typename F::Partial>(f, f.identity);
  vector<pair<timestamp, int>> bulkOne{
      make_pair(5, 105),           make_pair(507, 100 + 507),
      make_pair(509, 100 + 509),   make_pair(511, 100 + 511),
      make_pair(515, 100 + 515),   make_pair(516, 100 + 516),
      make_pair(517, 100 + 517),   make_pair(518, 100 + 518),
      make_pair(1700, 100 + 1700), make_pair(1701, 100 + 1701),
      make_pair(1702, 100 + 1702), make_pair(1703, 100 + 1703)};

  bfinger_wrapped.bulkEvict(1000);
  std::cout << "bulk_evict_from_adapter<" << minArity << "> passed" << std::endl;
}

int main(int argc, char *argv[]) {
  bulk_evict_tests();
  bulk_insert_tests();
  bulk_insert_with_repeats_tests();
  bulk_insert_from_random_trees();
  bulk_insert_from_adapter<3>(Collect<int>());
  bulk_evict_from_adapter<3>(Collect<int>());

  return 0;
}
