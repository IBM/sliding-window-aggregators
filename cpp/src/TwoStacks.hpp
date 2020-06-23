#ifndef __TWOSTACKS_H_
#define __TWOSTACKS_H_

#include<deque>
#include<iostream>
#include<iterator>
#include<cassert>

namespace twostacks {
  using namespace std;

  template<typename valT, typename aggT>
  class __AggT {
  public:
      valT _val;
      aggT _agg;
    __AggT() {}
    __AggT(valT val_, aggT agg_)
      : _val(val_), _agg(agg_) {}
  };

  template<typename binOpFunc,
           typename stackT=deque<__AggT<typename binOpFunc::Partial, typename binOpFunc::Partial>>>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef __AggT<aggT, aggT> AggT;

    Aggregate(binOpFunc binOp_, aggT identE_)
      : _front(), _back(), _binOp(binOp_), _identE(identE_) {}

    size_t size() { return _front.size() + _back.size(); }

    void insert(inT v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      auto prev = _back.size()>0?_back.back()._agg:_identE;
      aggT lifted = _binOp.lift(v);
      _back.push_back(AggT(lifted, _binOp.combine(prev, lifted)));
    }

    void evict() {
      _IFDEBUG(std::cerr << "evicting" << std::endl;);

      if (_front.empty()) {
        while (!_back.empty()) {
          aggT v = _back.back()._val; _back.pop_back(); // eject from back
          auto a = _front.size()>0?_front.back()._agg:_identE;
          _front.push_back(AggT(v, _binOp.combine(v, a)));
        }
      }
      _front.pop_back();
    }

    outT query() {
      auto bp = _back.size()>0?_back.back()._agg:_identE;
      auto fp = _front.size()>0?_front.back()._agg:_identE;

      return _binOp.lower(_binOp.combine(fp, bp));
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
