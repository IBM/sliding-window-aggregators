#ifndef __IMPLICITQUEUEABA_H_
#define __IMPLICITQUEUEABA_H_

#include"ChunkedArrayQueue.hpp"
#include<iostream>
#include<iterator>
#include<cassert>


#ifdef DEBUG
#define _IFDEBUG(x) x
#else
#define _IFDEBUG(x)
#endif

namespace aba {
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
           typename queueT=ChunkedArrayQueue<__AggT<typename binOpFunc::Partial, typename binOpFunc::Partial>>>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef __AggT<aggT, aggT> AggT;
 
    Aggregate(binOpFunc binOp_, aggT identE_)
      : _q(), _binOp(binOp_), _identE(identE_) {
      b = _q.begin();
    }
    
    size_t size() { return _q.size(); }

    void insert(inT v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      auto prev = (b!=_q.end())?_q.back()._agg:_identE;
      aggT lifted = _binOp.lift(v);
      _q.push_back(AggT(lifted, _binOp.combine(prev, lifted)));
    }

    void evict() {
      _IFDEBUG(std::cerr << "evicting" << std::endl;);
      // front empty?
      if (_q.begin()==b) {
        iterT p=_q.end();
        aggT sum=_identE;
        while (p!=_q.begin()) {
          --p;
          //          cout << "going over" << p->_val << " with sum= " << sum<< endl;
          sum = _binOp.combine(p->_val, sum);
          p->_agg = sum;
        }
        b=_q.end();
      }
      _q.pop_front();
    }

    outT query() {
      auto bp = (b!=_q.end())?_q.back()._agg:_identE;
      auto fp = (b!=_q.begin())?_q.front()._agg:_identE;
      
      return _binOp.lower(_binOp.combine(fp, bp));
    }

    outT naive_query() {
      aggT accum = _identE;
      for (iterT it=_q.begin(); it!=_q.end(); it++){
        accum = _binOp.combine(accum, it->_val);
      }
      return _binOp.lower(accum);
    }

  private:
    typedef typename queueT::iterator iterT;
    
    queueT _q;
    iterT b;
    
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
};
#endif
