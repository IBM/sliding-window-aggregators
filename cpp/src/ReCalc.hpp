#ifndef __RECALC_HPP_
#define __RECALC_HPP_

#include <deque>
#include <iostream>
#include <iterator>
#include <cassert>

#ifdef DEBUG
#define _IFDEBUG(x) x
#else
#define _IFDEBUG(x)
#endif

namespace recalc {
  using namespace std;

  template<typename binOpFunc,
           typename queueT=deque<typename binOpFunc::In>>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;

    Aggregate(binOpFunc binOp_, aggT identE_): 
        _q(), _binOp(binOp_), _identE(identE_)
    {}

    size_t size() { return _q.size(); }
  
    void insert(inT v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      _q.push_back(v);
    }

    void evict() {
      _IFDEBUG(std::cerr << "evicting" << std::endl;);
      _q.pop_front();
    }

    outT query() {
      aggT accum = _identE;
      for (auto it=_q.begin(); it!=_q.end(); it++) {
        _binOp.recalc_combine(accum, *it);
      }
      return _binOp.lower(accum);
    }

private:
    queueT _q;
    typedef typename queueT::iterator iterT;
    binOpFunc _binOp;
    aggT _identE;
  };

  template <class BinaryFunction, class T>
  Aggregate<BinaryFunction> make_aggregate(BinaryFunction f, T elem) {
    return Aggregate<BinaryFunction>(f, elem);
  }

  template <typename BinaryFunction>
  struct MakeAggregate {
    template <typename T>
    Aggregate<BinaryFunction> operator()(T elem) {
      BinaryFunction f;
      return make_aggregate(f, elem);
    }
  };
}

#endif
