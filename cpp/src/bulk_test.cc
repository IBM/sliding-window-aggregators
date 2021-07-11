#include <functional>
#include <iostream>
#include <vector>
#include <utility>

#include "ReCalc.hpp"
#include "AggregationFunctions.hpp"
#include "FiBA.hpp"

typedef long long int timestamp;

using namespace std;
template <class AGG>
void brute_bulkInsert(AGG& agg, vector<pair<timestamp, int>>& bulk) {
    for (auto [time, val]: bulk) {
        agg.insert(time, val);
    }
}

/*
 * simple_fixed_bulk - inserts two bulks of items into various locations
 * throughout the tree.
 */
void simple_fixed_bulk() {
    auto identity = 0;
    auto f = Sum<int>();
    auto bfinger_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);
    auto ref_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);

    vector<pair<timestamp, int>> initial{
        make_pair(1, 101),
        make_pair(500, 100 + 500),
        make_pair(1000, 100 + 1000),
        make_pair(1500, 100 + 1500)
    };
    for (auto [time, val]: initial) {
        bfinger_agg.insert(time, val);
        ref_agg.insert(time, val);
    }
    assert(ref_agg.query() == bfinger_agg.query());

    vector<pair<timestamp, int>> bulkOne{
        make_pair(5, 105),
        make_pair(507, 100+507),
        make_pair(509, 100+509),
        make_pair(511, 100+511),
        make_pair(515, 100+515),
        make_pair(516, 100+516),
        make_pair(517, 100+517),
        make_pair(518, 100+518),
        make_pair(1700, 100+1700),
        make_pair(1701, 100+1701),
        make_pair(1702, 100+1702),
        make_pair(1703, 100+1703)
    };

    bfinger_agg.bulkInsert(bulkOne);
    brute_bulkInsert(ref_agg, bulkOne);

    assert(ref_agg.query() == bfinger_agg.query());

    vector<pair<timestamp, int>> bulkTwo {
        make_pair(6, 100+6),
        make_pair(7, 100+7),
        make_pair(8, 100+8),
        make_pair(9, 100+9),
        make_pair(1705, 100+1705),
        make_pair(1706, 100+1706),
        make_pair(1707, 100+1707),
        make_pair(1708, 100+1708),
        make_pair(1709, 100+1709),
        make_pair(1710, 100+1710),
        make_pair(1711, 100+1711)
    };
    bfinger_agg.bulkInsert(bulkTwo);
    brute_bulkInsert(ref_agg, bulkTwo);

    assert(ref_agg.query() == bfinger_agg.query());

    auto ans1 = bfinger_agg.query();
    std::cout << "ans = " << ans1 << std::endl;
}

/*
 * simple_fixed_extreme - inserts bulks of items into either left or right ends
 * to test inserting in the extreme cases.
 */
void simple_fixed_extreme() {
    auto identity = 0;
    auto f = Sum<int>();
    auto bfinger_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);
    auto ref_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);

    vector<pair<timestamp, int>> initial{
        make_pair(400, 101),
        make_pair(500, 100 + 500),
        make_pair(1000, 100 + 1000),
        make_pair(1500, 100 + 1500)
    };
    for (auto [time, val]: initial) {
        bfinger_agg.insert(time, val);
        ref_agg.insert(time, val);
    }
    assert(ref_agg.query() == bfinger_agg.query());

    // Bulk 1: Insert at front (touching left spine)
    {
      vector<pair<timestamp, int>> bulk;
      for (int t = 0; t < 200; t += 3)
        bulk.push_back(make_pair(t, 100 + t));
      bfinger_agg.bulkInsert(bulk);
      brute_bulkInsert(ref_agg, bulk);
    }
    assert(ref_agg.query() == bfinger_agg.query());

    // Bulk 2: Front (but interleaved with earlier)
    {
      vector<pair<timestamp, int>> bulk;
      for (int t = 1; t < 200; t += 3)
        bulk.push_back(make_pair(t, 100 + t));
      bfinger_agg.bulkInsert(bulk);
      brute_bulkInsert(ref_agg, bulk);
    }
    assert(ref_agg.query() == bfinger_agg.query());

    // Bulk 3: Far right
    {
      vector<pair<timestamp, int>> bulk;
      for (int t = 2000; t < 5000; t += 2)
        bulk.push_back(make_pair(t, 100 + t));
      bfinger_agg.bulkInsert(bulk);
      brute_bulkInsert(ref_agg, bulk);
    }
    assert(ref_agg.query() == bfinger_agg.query());

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

int main(int argc, char* argv[]) {
    simple_fixed_bulk();

    return 0;
}
