#ifndef __SUBTRACTONEVICT_HPP_
#define __SUBTRACTONEVICT_HPP_

#include<deque>
#include<iostream>
#include<iterator>
#include<cassert>

#ifdef DEBUG
#define _IFDEBUG(x) x
#else
#define _IFDEBUG(x)
#endif

namespace soe {
  using namespace std;

  template<typename binOpFunc,
           typename queueT=deque<typename binOpFunc::Partial>>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;

    Aggregate(binOpFunc binOp_, aggT identE_)
      : _q(), _binOp(binOp_), _identE(identE_),
      _sum(identE_) {}

    size_t size() { return _q.size(); }
  
    void insert(inT v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      aggT lifted = _binOp.lift(v);
      _sum = _binOp.combine(_sum, lifted);
      _q.push_back(lifted);
    }

    void evict() {
      _IFDEBUG(std::cerr << "evicting" << std::endl;);
      auto front = _q.front();
      _sum = _binOp.inverse_combine(_sum, front);
      _q.pop_front();
    }

    outT query() { return _binOp.lower(_sum); }

    outT naive_query() {
      aggT accum = _identE;
      for (auto it=_q.begin(); it!=_q.end(); it++){
        accum = _binOp.combine(accum, *it);
      }
      return _binOp.lower(accum);
  }

  
private:
    queueT _q;
    typedef typename queueT::iterator iterT;
    // the binary operator deck
    binOpFunc _binOp;
    aggT _identE;
    aggT _sum;
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
