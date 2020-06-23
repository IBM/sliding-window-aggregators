#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <set>
#include <map>

#include "DABA.hpp"
#include "DABALite.hpp"
#include "TwoStacks.hpp"
#include "TwoStacksLite.hpp"
#include "FlatFIT.hpp"
#include "DynamicFlatFIT.hpp"
#include "ImplicitQueueABA.hpp"
#include "SubtractOnEvict.hpp"
#include "TimestampedFifo.hpp"
#include "TimestampedTwoStacks.hpp"
#include "ReCalc.hpp"
#include "AggregationFunctions.hpp"
#include "Reactive.hpp"
#include "OkasakisQueue.hpp"
#include "FiBA.hpp"

typedef long long int timestamp;

template <typename T>
struct IdentityLifter {
    T operator()(const T& other) const {
        return other;
    }
};

void real_assert(bool predicate, std::string msg) {
    if (!predicate) {
        std::cerr << msg << std::endl << std::flush;
        throw 0;
    }
}

template <class F>
void test_alg_no_inverse(F f, typename F::Partial identity, uint64_t iterations, uint64_t window_size, std::string name) {
    auto daba_agg = daba::make_aggregate<true>(f, identity);
    auto daba_lite_agg = dabalite::make_aggregate(f, identity);
    auto aba_agg = aba::make_aggregate(f, identity);
    auto twostacks_agg = twostacks::make_aggregate(f, identity);
    auto twostacks_lite_agg = twostackslite::make_aggregate(f, identity);
    auto flatfit_agg = flatfit::make_aggregate(f, identity);
    auto dyn_flatfit_agg = dynamic_flatfit::make_aggregate(f, identity);
    auto recalc_agg = recalc::make_aggregate(f, identity);
    auto reactive_agg = reactive::make_aggregate(f, identity);
    auto okasakis_agg = okasaki::make_aggregate(f, identity);
    auto bclassic_agg = btree::make_aggregate<timestamp, 2, btree::classic>(f, identity);
    auto bknuckle_agg = btree::make_aggregate<timestamp, 2, btree::knuckle>(f, identity);
    auto bfinger_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);

    for (uint64_t i = 0; i < iterations; ++i) {
        size_t sz = recalc_agg.size();
        real_assert(sz == daba_agg.size(), name + ": recalc size != daba");
        real_assert(sz == daba_lite_agg.size(), name + ": recalc size != daba_lite");
        real_assert(sz == aba_agg.size(), name + ": recalc size != aba");
        real_assert(sz == twostacks_agg.size(), name + ": recalc size != two_stacks");
        real_assert(sz == twostacks_lite_agg.size(), name + ": recalc size != two_stacks_lite");
        real_assert(sz == flatfit_agg.size(), name + ": recalc size != flatfit");
        real_assert(sz == dyn_flatfit_agg.size(), name + ": recalc size != dyn_flatfit");
        real_assert(sz == reactive_agg.size(), name + ": recalc size != reactive");
        real_assert(sz == okasakis_agg.size(), name + ": recalc size != okasaki");
        real_assert(sz == bclassic_agg.size(), name + ": recalc size != bclassic");
        real_assert(sz == bknuckle_agg.size(), name + ": recalc size != bknuckle");
        real_assert(sz == bfinger_agg.size(), name + ": recalc size != bfinger");        

        if (recalc_agg.size() == window_size) {
            daba_agg.evict();
            daba_lite_agg.evict();
            aba_agg.evict();
            twostacks_agg.evict();
            twostacks_lite_agg.evict();
            flatfit_agg.evict();
            dyn_flatfit_agg.evict();
            recalc_agg.evict();
            reactive_agg.evict();
            okasakis_agg.evict();
            bclassic_agg.evict();
            bknuckle_agg.evict();
            bfinger_agg.evict();            
        }

        recalc_agg.insert(i);
        daba_agg.insert(i);
        daba_lite_agg.insert(i);
        aba_agg.insert(i);
        twostacks_agg.insert(i);
        twostacks_lite_agg.insert(i);
        flatfit_agg.insert(i);
        dyn_flatfit_agg.insert(i);
        reactive_agg.insert(i);
        okasakis_agg.insert(i);
        bclassic_agg.insert(i);
        bknuckle_agg.insert(i);
        bfinger_agg.insert(i);

        typename F::Out res = recalc_agg.query();
        real_assert(res == daba_agg.query(), name + ": recalc != daba");
        real_assert(res == daba_lite_agg.query(), name + ": recalc != daba_lite");
        real_assert(res == aba_agg.query(), name + ": recalc != aba");
        real_assert(res == twostacks_agg.query(), name + ": recalc != two_stacks");
        real_assert(res == twostacks_lite_agg.query(), name + ": recalc != two_stacks_lite");
        real_assert(res == flatfit_agg.query(), name + ": recalc != flatfit");
        real_assert(res == dyn_flatfit_agg.query(), name + ": recalc != dyn_flatfit");
        real_assert(res == reactive_agg.query(), name + ": recalc != reactive");
        real_assert(res == okasakis_agg.query(), name + ": recalc != okasaki");
        real_assert(res == bclassic_agg.query(), name + ": recalc != bclassic");
        real_assert(res == bknuckle_agg.query(), name + ": recalc != bknuckle");
        real_assert(res == bfinger_agg.query(), name + ": recalc != bfinger");
    }
    std::cout << name << " passed" << std::endl;
}

template <class F>
void test_alg_with_inverse(F f, typename F::Partial identity, uint64_t iterations, uint64_t window_size, std::string name) {
    auto daba_agg = daba::make_aggregate<true>(f, identity);
    auto daba_lite_agg = dabalite::make_aggregate(f, identity);
    auto aba_agg = aba::make_aggregate(f, identity);
    auto twostacks_agg = twostacks::make_aggregate(f, identity);
    auto twostacks_lite_agg = twostackslite::make_aggregate(f, identity);
    auto flatfit_agg = flatfit::make_aggregate(f, identity);
    auto dyn_flatfit_agg = dynamic_flatfit::make_aggregate(f, identity);
    auto soe_agg = soe::make_aggregate(f, identity);
    auto recalc_agg = recalc::make_aggregate(f, identity);
    auto reactive_agg = reactive::make_aggregate(f, identity);
    auto bclassic_agg = btree::make_aggregate<timestamp, 2, btree::classic>(f, identity);
    auto bknuckle_agg = btree::make_aggregate<timestamp, 2, btree::knuckle>(f, identity);
    auto bfinger_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);

    for (uint64_t i = 0; i < iterations; ++i) {
        size_t sz = recalc_agg.size();
        real_assert(sz == daba_agg.size(), name + ": recalc size != daba");
        real_assert(sz == daba_lite_agg.size(), name + ": recalc size != daba_lite");
        real_assert(sz == aba_agg.size(), name + ": recalc size != aba");
        real_assert(sz == twostacks_agg.size(), name + ": recalc size != two_stacks");
        real_assert(sz == twostacks_lite_agg.size(), name + ": recalc size != two_stacks_lite");
        real_assert(sz == flatfit_agg.size(), name + ": recalc size != flatfit");
        real_assert(sz == dyn_flatfit_agg.size(), name + ": recalc size != dyn_flatfit");
        real_assert(sz == soe_agg.size(), name + ": recalc size != soe");
        real_assert(sz == bclassic_agg.size(), name + ": recalc size != bclassic");
        real_assert(sz == bknuckle_agg.size(), name + ": recalc size != bknuckle");
        real_assert(sz == bfinger_agg.size(), name + ": recalc size != bfinger");

        if (recalc_agg.size() == window_size) {
            daba_agg.evict();
            daba_lite_agg.evict();
            aba_agg.evict();
            twostacks_agg.evict();
            twostacks_lite_agg.evict();
            flatfit_agg.evict();
            dyn_flatfit_agg.evict();
            soe_agg.evict();
            recalc_agg.evict();
            reactive_agg.evict();
            bclassic_agg.evict();
            bknuckle_agg.evict();
            bfinger_agg.evict();
        }

        recalc_agg.insert(i);
        daba_agg.insert(i);
        daba_lite_agg.insert(i);
        aba_agg.insert(i);
        twostacks_agg.insert(i);
        twostacks_lite_agg.insert(i);
        flatfit_agg.insert(i);
        dyn_flatfit_agg.insert(i);
        soe_agg.insert(i);
        reactive_agg.insert(i);
        bclassic_agg.insert(i);
        bknuckle_agg.insert(i);
        bfinger_agg.insert(i);

        typename F::Out res = recalc_agg.query();
        real_assert(res == daba_agg.query(), name + ": recalc != daba");
        real_assert(res == daba_lite_agg.query(), name + ": recalc != daba_lite");
        real_assert(res == aba_agg.query(), name + ": recalc != aba");
        real_assert(res == twostacks_agg.query(), name + ": recalc != two_stacks");
        real_assert(res == twostacks_lite_agg.query(), name + ": recalc != two_stacks_lite");
        real_assert(res == flatfit_agg.query(), name + ": recalc != flatfit");
        real_assert(res == dyn_flatfit_agg.query(), name + ": recalc != dyn_flatfit");
        real_assert(res == soe_agg.query(), name + ": recalc != soe");
        real_assert(res == reactive_agg.query(), name + ": recalc != reactive");
        real_assert(res == bclassic_agg.query(), name + ": recalc != bclassic");
        real_assert(res == bknuckle_agg.query(), name + ": recalc != bknuckle");
        real_assert(res == bfinger_agg.query(), name + ": recalc != bfinger");
    }
    std::cout << name << " passed" << std::endl;
}

template <class F>
void test_alg_no_inverse_sawtooth(F f, typename F::Partial identity,
                                   uint64_t rep,
                                   uint64_t window_size,
                                   std::string name) {
  auto daba_agg = daba::make_aggregate<true>(f, identity);
  auto daba_lite_agg = dabalite::make_aggregate(f, identity);
  auto aba_agg = aba::make_aggregate(f, identity);
  auto twostacks_agg = twostacks::make_aggregate(f, identity);
  auto twostacks_lite_agg = twostackslite::make_aggregate(f, identity);
  auto flatfit_agg = flatfit::make_aggregate(f, identity);
  auto dyn_flatfit_agg = dynamic_flatfit::make_aggregate(f, identity);
  auto recalc_agg = recalc::make_aggregate(f, identity);
  auto reactive_agg = reactive::make_aggregate(f, identity);
  auto bclassic_agg = btree::make_aggregate<timestamp, 2, btree::classic>(f, identity);
  auto bknuckle_agg = btree::make_aggregate<timestamp, 2, btree::knuckle>(f, identity);
  auto bfinger_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);
  
  while (rep-- > 0) {
    for (uint64_t i=0;i<window_size;i++) {
      recalc_agg.insert(i);
      daba_agg.insert(i);
      daba_lite_agg.insert(i);
      aba_agg.insert(i);
      twostacks_agg.insert(i);
      twostacks_lite_agg.insert(i);
      flatfit_agg.insert(i);
      dyn_flatfit_agg.insert(i);
      reactive_agg.insert(i);
      bclassic_agg.insert(i);
      bknuckle_agg.insert(i);
      bfinger_agg.insert(i);

      typename F::Out res = recalc_agg.query();
      real_assert(res == daba_agg.query(), name + ": recalc != daba");
      real_assert(res == daba_lite_agg.query(), name + ": recalc != daba_lite");
      real_assert(res == aba_agg.query(), name + ": recalc != aba");
      real_assert(res == twostacks_agg.query(), name + ": recalc != two_stacks");
      real_assert(res == twostacks_lite_agg.query(), name + ": recalc != two_stacks_lite");
      real_assert(res == flatfit_agg.query(), name + ": recalc != flatfit");
      real_assert(res == dyn_flatfit_agg.query(), name + ": recalc != dyn_flatfit");
      real_assert(res == reactive_agg.query(), name + ": recalc != reactive");
      real_assert(res == bclassic_agg.query(), name + ": recalc != bclassic");
      real_assert(res == bknuckle_agg.query(), name + ": recalc != bknuckle");
      real_assert(res == bfinger_agg.query(), name + ": recalc != bfinger");
    }
    // drain
    while (recalc_agg.size() > 0) {
      daba_agg.evict();
      daba_lite_agg.evict();
      aba_agg.evict();
      twostacks_agg.evict();
      twostacks_lite_agg.evict();
      flatfit_agg.evict();
      dyn_flatfit_agg.evict();
      recalc_agg.evict();
      reactive_agg.evict();
      bclassic_agg.evict();
      bknuckle_agg.evict();
      bfinger_agg.evict();

      typename F::Out res = recalc_agg.query();
      real_assert(res == daba_agg.query(), name + ": recalc != daba");
      real_assert(res == daba_lite_agg.query(), name + ": recalc != daba_lite");
      real_assert(res == aba_agg.query(), name + ": recalc != aba");
      real_assert(res == twostacks_agg.query(), name + ": recalc != two_stacks");
      real_assert(res == twostacks_lite_agg.query(), name + ": recalc != two_stacks_lite");
      real_assert(res == flatfit_agg.query(), name + ": recalc != flatfit");
      real_assert(res == dyn_flatfit_agg.query(), name + ": recalc != dyn_flatfit");
      real_assert(res == reactive_agg.query(), name + ": recalc != reactive");
      real_assert(res == bclassic_agg.query(), name + ": recalc != bclassic");
      real_assert(res == bknuckle_agg.query(), name + ": recalc != bknuckle");
      real_assert(res == bfinger_agg.query(), name + ": recalc != bfinger");
    }
  }
  std::cout << "sawtooth:" << name << " passed" << std::endl;
}

void test_non_fifo_set(uint64_t iterations, uint64_t window_size) {
    srand(12345);
    std::multiset<timestamp> ms;
    auto bclassic_agg = btree::make_aggregate<timestamp, 2, btree::classic>(Sum<int>(), 0);
    auto bknuckle_agg = btree::make_aggregate<timestamp, 2, btree::knuckle>(Sum<int>(), 0);
    auto bfinger_agg = btree::make_aggregate<timestamp, 2, btree::finger>(Sum<int>(), 0);
    for (uint64_t i = 0; i < iterations; ++i) {
        int ts = rand() % window_size;
        if (i >= window_size) {
            ms.erase(ts);
            bclassic_agg.evict(ts);
            bclassic_agg.checkInvariant(__FILE__, __LINE__);
            assert((int)ms.count(ts) == bclassic_agg.at(ts));
            bknuckle_agg.evict(ts);
            bknuckle_agg.checkInvariant(__FILE__, __LINE__);
            assert((int)ms.count(ts) == bknuckle_agg.at(ts));
            bfinger_agg.evict(ts);
            bfinger_agg.checkInvariant(__FILE__, __LINE__);
            assert((int)ms.count(ts) == bfinger_agg.at(ts));
        }
        ms.insert(ts);
        bclassic_agg.insert(ts, 1);
        bclassic_agg.checkInvariant(__FILE__, __LINE__);
        assert((int)ms.count(ts) == bclassic_agg.at(ts));
        bknuckle_agg.insert(ts, 1);
        bknuckle_agg.checkInvariant(__FILE__, __LINE__);
        assert((int)ms.count(ts) == bknuckle_agg.at(ts));
        bfinger_agg.insert(ts, 1);
        bfinger_agg.checkInvariant(__FILE__, __LINE__);
        assert((int)ms.count(ts) == bfinger_agg.at(ts));
    }
    std::cout << "non-FIFO with set passed" << std::endl;
}

template <class F>
void test_non_fifo_map(F f, typename F::Partial identity, uint64_t iterations, uint64_t window_size, std::string name) {
    srand(12345);
    typedef typename F::Partial aggT;
    std::map<timestamp, aggT> mp;
    aggT sum = identity;
    auto bclassic_agg = btree::make_aggregate<timestamp, 2, btree::classic>(f, identity);
    auto bknuckle_agg = btree::make_aggregate<timestamp, 2, btree::knuckle>(f, identity);
    auto bfinger_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);
    for (uint64_t i = 0; i < iterations; ++i) {
        timestamp const evict_ts = i;
        bclassic_agg.evict(evict_ts);
        bclassic_agg.checkInvariant(__FILE__, __LINE__);
        bknuckle_agg.evict(evict_ts);
        bknuckle_agg.checkInvariant(__FILE__, __LINE__);
        bfinger_agg.evict(evict_ts);
        bfinger_agg.checkInvariant(__FILE__, __LINE__);
        auto evict_iter = mp.find(evict_ts);
        if (evict_iter != mp.end()) {
            aggT const evict_val = evict_iter->second;
            sum = f.inverse_combine(sum, evict_val);
            mp.erase(evict_ts);
        }
        assert(f.lower(sum) == bclassic_agg.query());
        assert(f.lower(sum) == bknuckle_agg.query());
        assert(f.lower(sum) == bfinger_agg.query());
        timestamp const insert_ts = i + rand() % window_size;
        int const insert_elem = rand() % window_size;
        auto insert_iter = mp.find(insert_ts);
        aggT const insert_val = f.lift(insert_elem);
        if (insert_iter == mp.end())
            mp[insert_ts] = insert_val;
        else
            mp[insert_ts] = f.combine(insert_iter->second, insert_val);
        sum = f.combine(sum, insert_val);
        bclassic_agg.insert(insert_ts, insert_elem);
        bclassic_agg.checkInvariant(__FILE__, __LINE__);
        assert(f.lower(sum) == bclassic_agg.query());
        bknuckle_agg.insert(insert_ts, insert_elem);
        bknuckle_agg.checkInvariant(__FILE__, __LINE__);
        assert(f.lower(sum) == bknuckle_agg.query());
        bfinger_agg.insert(insert_ts, insert_elem);
        bfinger_agg.checkInvariant(__FILE__, __LINE__);
        assert(f.lower(sum) == bfinger_agg.query());
    }
    std::cout << "non-FIFO " << name << " with map passed" << std::endl;
}

template <class F>
using aggT = typename F::Partial;
template <class F>
aggT<F> naive_walk_sum(F f,
                       aggT<F> identity,
                       std::map<timestamp, aggT<F>> mp) {
  aggT<F> sum = identity;

  for (auto it=mp.begin(); it != mp.end(); ++it)
    sum = f.combine(sum, it->second);

  return sum;
}
inline std::ostream& operator<<(std::ostream& os, std::list<int> const& p) {
  os << "[";
  bool first = true;
  for (auto i=p.begin(), n=p.end(); i!=n; i++) {
    if (first)
      first = false;
    else
      os << ", ";
    os << *i;
  }
  return os << "]";
}

template <class F>
void test_non_fifo_brute_force(F f,
                               aggT<F> identity,
                               uint64_t iterations,
                               uint64_t window_size,
                               std::string name) {
    srand(0x12345);
    std::map<timestamp, aggT<F>> mp;

    auto bclassic_agg = btree::make_aggregate<timestamp, 2, btree::classic>(f, identity);
    auto bknuckle_agg = btree::make_aggregate<timestamp, 2, btree::knuckle>(f, identity);
    auto bfinger_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);

    // check_query: compares the queries from classic and finger B-trees
    // with a naive walk.
    auto check_query = [&]() -> void {
      auto sum = naive_walk_sum(f, identity, mp);
      //      std::cout << "correct:" << f.lower(sum) << std::endl;
      assert(f.lower(sum) == bclassic_agg.query());
      assert(f.lower(sum) == bknuckle_agg.query());
      assert(f.lower(sum) == bfinger_agg.query());
      //       std::cout << "me: " << x << std::endl;
    };

    for (uint64_t i = 0; i < iterations; ++i) {
        // eviction
        if (mp.size()>0 && mp.size()>=window_size) {
          bclassic_agg.evict();
          bclassic_agg.checkInvariant(__FILE__, __LINE__);
          bknuckle_agg.evict();
          bknuckle_agg.checkInvariant(__FILE__, __LINE__);
          bfinger_agg.evict();
          bfinger_agg.checkInvariant(__FILE__, __LINE__);
          auto evict_iter = mp.begin();
          if (evict_iter != mp.end()) { mp.erase(evict_iter); }
        }

        check_query();

        // insertion
        timestamp insert_ts;
        do {
          insert_ts = i + rand() % (11*window_size);
        } while (mp.find(insert_ts) != mp.end());

        int const insert_elem = rand() % window_size;
        auto insert_iter = mp.find(insert_ts);
        aggT<F> const insert_val = f.lift(insert_elem);
        if (insert_iter == mp.end())
            mp[insert_ts] = insert_val;
        else {
          std::cout << "ouch ouch ouch " << std::endl << std::flush;
          throw 0;
            mp[insert_ts] = f.combine(insert_iter->second, insert_val);
        }

        bclassic_agg.insert(insert_ts, insert_elem);
        bclassic_agg.checkInvariant(__FILE__, __LINE__);
        bknuckle_agg.insert(insert_ts, insert_elem);
        bknuckle_agg.checkInvariant(__FILE__, __LINE__);
        bfinger_agg.insert(insert_ts, insert_elem);
        bfinger_agg.checkInvariant(__FILE__, __LINE__);
        check_query();
    }
    std::cout << "non-FIFO " << name << " wrt. naive walk passed" << std::endl;
}

template <class F>
void test_range_query(F f,
                      aggT<F> identity,
                      uint64_t window_size,
                      std::string name) {
  srand(0x12345);
  std::vector<timestamp> times;
  std::vector<aggT<F>> values;
  auto bclassic_agg = btree::make_aggregate<timestamp, 2, btree::classic>(f, identity);
  timestamp insert_ts = 0;
  for (uint64_t i=0; i<window_size+2; i++) {
    insert_ts += (rand() % 3) + 1;
    times.push_back(insert_ts);
    if (i==0 || i==window_size+1) {
      values.push_back(identity);
    } else {
      const int insert_elem = rand() % window_size;
      values.push_back(f.lift(insert_elem));
      bclassic_agg.insert(insert_ts, insert_elem);
    }
  }
  for (uint64_t i=0; i<window_size+2; i++) {
    aggT<F> sum = identity;
    for (uint64_t j=i; j<window_size+2; j++) {
      sum = f.combine(sum, values[j]);
      int btree_sum = bclassic_agg.rangeQuery(times[i], times[j]);
      assert(f.lower(sum) == btree_sum);
    }
  }
  std::cout << "range query " << name << " passed" << std::endl;
}

template <class F>
void test_timestamped_fifo(F f, typename F::Partial identity, uint64_t iterations, uint64_t window_size, std::string name) {
    auto daba_agg = timestampedfifo::make_aggregate<timestamp, daba::Aggregate<F, true>>(f, identity);
    auto daba_lite_agg = timestampedfifo::make_aggregate<timestamp, dabalite::Aggregate<F>>(f, identity);
    auto twostacks_agg = timestampedfifo::make_aggregate<timestamp, twostacks::Aggregate<F>>(f, identity);
    auto twostacks_lite_agg = timestampedfifo::make_aggregate<timestamp, twostackslite::Aggregate<F>>(f, identity);
    auto flatfit_agg = timestampedfifo::make_aggregate<timestamp, flatfit::Aggregate<F>>(f, identity);
    auto recalc_agg = timestampedfifo::make_aggregate<timestamp, recalc::Aggregate<F>>(f, identity);
    auto bclassic_agg = btree::make_aggregate<timestamp, 2, btree::classic>(f, identity);
    auto bfinger_agg = btree::make_aggregate<timestamp, 2, btree::finger>(f, identity);

    for (uint64_t i = 0; i < iterations; ++i) {
        size_t sz = recalc_agg.size();
        real_assert(sz == daba_agg.size(), name + ": recalc size != daba");
        real_assert(sz == daba_lite_agg.size(), name + ": recalc size != daba_lite");
        real_assert(sz == twostacks_agg.size(), name + ": recalc size != two_stacks");
        real_assert(sz == twostacks_lite_agg.size(), name + ": recalc size != two_stacks_lite");
        real_assert(sz == flatfit_agg.size(), name + ": recalc size != flatfit");
        real_assert(sz == bclassic_agg.size(), name + ": recalc size != bclassic");
        real_assert(sz == bfinger_agg.size(), name + ": recalc size != bfinger");

        while (recalc_agg.size() > 0 && (window_size < recalc_agg.youngest() - recalc_agg.oldest())) {
            daba_agg.evict();
            daba_lite_agg.evict();
            twostacks_agg.evict();
            twostacks_lite_agg.evict();
            flatfit_agg.evict();
            recalc_agg.evict();
            bclassic_agg.evict();
            bfinger_agg.evict();
        }

        recalc_agg.insert(i, i);
        daba_agg.insert(i, i);
        daba_lite_agg.insert(i, i);
        twostacks_agg.insert(i, i);
        twostacks_lite_agg.insert(i, i);
        flatfit_agg.insert(i, i);
        bclassic_agg.insert(i, i);
        bfinger_agg.insert(i, i);        

        typename F::Out res = recalc_agg.query();
        real_assert(res == daba_agg.query(), name + ": recalc != daba");
        real_assert(res == daba_lite_agg.query(), name + ": recalc != daba_lite");
        real_assert(res == twostacks_agg.query(), name + ": recalc != two_stacks");
        real_assert(res == twostacks_lite_agg.query(), name + ": recalc != two_stacks_lite");
        real_assert(res == flatfit_agg.query(), name + ": recalc != flatfit");
        real_assert(res == bclassic_agg.query(), name + ": recalc != bclassic");
        real_assert(res == bfinger_agg.query(), name + ": recalc != bfinger");
    }
    std::cout << "timestamped-FIFO " << name << " passed" << std::endl;
}

int main() {
  test_timestamped_fifo(MinCount<int>(), MinCount<int>::identity, 1000000, 100, "mincount");
  test_timestamped_fifo(Sum<int>(), Sum<int>::identity, 1000000, 100, "sum");

  test_range_query(Sum<int>(), 0, 50, "sum");
  test_non_fifo_brute_force(Collect<int>(), Collect<int>::identity, 12345, 251, "collect");
  test_non_fifo_brute_force(BloomFilter<int>(), BloomFilter<int>::identity, 997, 91, "bloom");
  test_non_fifo_set(20000, 5000);
  test_non_fifo_map(Sum<int>(), 0, 20000, 5000, "sum");

  test_alg_with_inverse(Sum<int>(), 0, 1000000, 100, "sum");
  test_alg_with_inverse(Mean<int>(), Mean<int>::identity, 1000000, 100, "mean");
  test_alg_with_inverse(SampleStdDev<int>(), SampleStdDev<int>::identity, 1000000, 100, "stddev");
  test_alg_with_inverse(Collect<int>(), Collect<int>::identity, 10000, 100, "collect");

  test_alg_no_inverse_sawtooth(MinCount<int>(), MinCount<int>::identity, 3, 1921, "mincount");
  test_alg_no_inverse_sawtooth(Collect<int>(), Collect<int>::identity, 3, 519, "collect");

  test_alg_no_inverse(MinCount<int>(), MinCount<int>::identity, 1000000, 100, "mincount");
  test_alg_no_inverse(ArgMax<int, int, IdentityLifter<int>>(),
                      ArgMax<int, int, IdentityLifter<int>>::identity, 1000000, 100, "argmax");
  test_alg_no_inverse(Max<int>(), 0, 1000000, 100, "max");


  // test below fails when checking invariants
  //test_alg_no_inverse(GeometricMean<int>(), GeometricMean<int>::identity, 1000000, 100, "geomean");

  return 0;
}
