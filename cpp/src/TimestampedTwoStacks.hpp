#ifndef __TIMESTAMPED_TWOSTACKS_H_
#define __TIMESTAMPED_TWOSTACKS_H_

#include<deque>
#include<iostream>
#include<iterator>
#include<cassert>

namespace timestamped_twostacks {
  using namespace std;

  template<typename valT, typename aggT, typename timeT>
  class __AggT {
  public:
      valT _val;
      aggT _agg;
      timeT _timestamp;
    __AggT() {}
    __AggT(valT val_, aggT agg_, timeT time_)
      : _val(val_), _agg(agg_), _timestamp(time_) {}
  };

  template<typename binOpFunc,
           typename Timestamp,
           typename stackT=deque<__AggT<typename binOpFunc::Partial, typename binOpFunc::Partial, Timestamp>>>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef Timestamp timeT;
    typedef __AggT<aggT, aggT, timeT> AggT;

    Aggregate(binOpFunc binOp_, aggT identE_)
      : _front(), _back(), _binOp(binOp_), _identE(identE_) {}

    size_t size() { return _front.size() + _back.size(); }

    void insert(timeT const& time, inT const& v) {
//     void insert(inT v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      auto prev = _back.size()>0?_back.back()._agg:_identE;
      aggT lifted = _binOp.lift(v);
      _back.push_back(AggT(lifted, _binOp.combine(prev, lifted), time));
    }

    void evict() {
      _IFDEBUG(std::cerr << "evicting" << std::endl;);

      if (_front.empty()) {
        while (!_back.empty()) {
          AggT & backsBack = _back.back();
          aggT v = backsBack._val;
          timeT t = backsBack._timestamp;
          auto a = _front.size()>0?_front.back()._agg:_identE;
          _front.push_back(AggT(v, _binOp.combine(v, a), t));
          _back.pop_back(); // eject from back
        }
      }
      _front.pop_back();
    }

    outT query() {
      auto bp = _back.size()>0?_back.back()._agg:_identE;
      auto fp = _front.size()>0?_front.back()._agg:_identE;

      return _binOp.lower(_binOp.combine(fp, bp));
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
      aggT accum = _identE;
      for (auto it=_front.rbegin(); it!=_front.rend(); it++){
        accum = _binOp.combine(accum, it->_val);
      }
      for (iterT it=_back.begin(); it!=_back.end(); it++){
        accum = _binOp.combine(accum, it->_val);
      }
      return _binOp.lower(accum);
    }

  private:
    stackT _front, _back;
    typedef typename stackT::iterator iterT;
    // the binary operator deck
    binOpFunc _binOp;
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
}

#endif
