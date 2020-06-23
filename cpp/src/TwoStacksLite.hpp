#ifndef __TWOSTACKSLITE_H_
#define __TWOSTACKSLITE_H_

#include<deque>
#include<iostream>
#include<iterator>
#include<cassert>

namespace twostackslite {
  using namespace std;

  template<typename aggT>
  class __AggT {
  public:
      aggT _val;
    __AggT() {}
    __AggT(aggT val_)
      : _val(val_) {}
  };

  template<typename binOpFunc,
           typename stackT=deque<__AggT<typename binOpFunc::Partial>>>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef __AggT<aggT> AggT;

    Aggregate(binOpFunc binOp_, aggT identE_)
      : _front(), _back(), _binOp(binOp_), _backSum(identE_), _identE(identE_) {}

    size_t size() { return _front.size() + _back.size(); }

    void insert(inT v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      aggT lifted = _binOp.lift(v);
      _backSum = _binOp.combine(_backSum, lifted);
      _back.push_back(AggT(lifted));
    }

    void evict() {
      _IFDEBUG(std::cerr << "evicting" << std::endl;);

      if (_front.empty()) {
        while (!_back.empty()) {
          aggT v = _back.back()._val; _back.pop_back(); // eject from back
          auto a = _front.size()>0?_front.back()._val:_identE;
          _front.push_back(AggT(_binOp.combine(v, a)));
        }
        _backSum = _identE; // _back must now be empty, resetting _backSum to I
      }
      _front.pop_back();
    }

    outT query() {
      auto bp = _backSum;
      auto fp = _front.size()>0?_front.back()._val:_identE;

      // std::cerr << "prequery: " << _binOp.combine(fp, bp) << std::endl;
      auto answer = _binOp.lower(_binOp.combine(fp, bp));
      // std::cerr << "query: " << bp << "--" << fp << "--" << answer << std::endl;
      return  answer;
    }

    outT naive_query() {
      throw 0;
    }

  private:
    stackT _front, _back;
    typedef typename stackT::iterator iterT;
    // the binary operator deck
    binOpFunc _binOp;
    aggT _backSum;
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
