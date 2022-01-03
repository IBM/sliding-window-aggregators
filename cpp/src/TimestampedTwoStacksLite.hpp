#ifndef __TIMESTAMPED_TWOSTACKSLITE_H_
#define __TIMESTAMPED_TWOSTACKSLITE_H_

#include<deque>
#include<iostream>
#include<iterator>
#include<cassert>

#include "BulkAdapter.hpp"

namespace timestamped_twostacks_lite {
  using namespace std;

  template<typename aggT, typename timeT>
  class __AggT {
  public:
      aggT _val;
      timeT _timestamp;
    __AggT() {}
    __AggT(aggT val_, timeT time_)
      : _val(val_), _timestamp(time_) {}
  };

  template<typename binOpFunc,
           typename Timestamp,
           typename stackT=deque<__AggT<typename binOpFunc::Partial, Timestamp>>>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef Timestamp timeT;
    typedef __AggT<aggT, timeT> AggT;

    Aggregate(binOpFunc binOp_, aggT identE_)
      : _front(), _back(), _binOp(binOp_), _backSum(identE_), _identE(identE_) {}

    size_t size() { return _front.size() + _back.size(); }

    void insert(timeT const& time, inT const& v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      aggT lifted = _binOp.lift(v);
      _backSum = _binOp.combine(_backSum, lifted);
      _back.push_back(AggT(lifted, time));
    }

    void evict() {
      _IFDEBUG(std::cerr << "evicting" << std::endl;);

      if (_front.empty()) {
        while (!_back.empty()) {
          AggT & backsBack = _back.back();
          aggT v = backsBack._val;
          timeT t = backsBack._timestamp;
          auto a = _front.size()>0?_front.back()._val:_identE;
          _front.push_back(AggT(_binOp.combine(v, a), t));
          _back.pop_back(); // eject from back
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

    timeT oldest() {
        return _front.empty()?
          (_back.front()._timestamp):(_front.back()._timestamp);
    }

    timeT youngest() {
        return _back.empty()?
          (_front.front()._timestamp):(_back.back()._timestamp);
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

  template <typename timeT, class BinaryFunction, class T>
  Aggregate<BinaryFunction, timeT> make_aggregate(BinaryFunction f, T elem) {
    return Aggregate<BinaryFunction, timeT>(f, elem);
  }

  template <typename BinaryFunction, typename timeT>
  struct MakeAggregate {
    template <typename T>
    Aggregate<BinaryFunction, timeT> operator()(T elem) {
      BinaryFunction f;
      return make_aggregate<timeT, BinaryFunction>(f, elem);
    }
  };

  template <typename timeT, class BinaryFunction, class T>
  auto make_bulk_aggregate(BinaryFunction f, T elem) {
    return BulkAdapter<
        Aggregate<BinaryFunction, timeT>,
        timeT,
        typename BinaryFunction::In
    >(f, elem);
  }

  template <typename BinaryFunction, typename timeT>
  struct MakeBulkAggregate {
    template <typename T>
    auto operator()(T elem) {
      BinaryFunction f;
      return make_bulk_aggregate<timeT, BinaryFunction>(f, elem);
    }
  };

}

#endif
