#ifndef __TIMESTAMPED_DABALITE_H__
#define __TIMESTAMPED_DABALITE_H__

#include<deque>
#include"ChunkedArrayQueue.hpp"
#include<iostream>
#include<iterator>
#include<cassert>

#ifdef DEBUG
#define _IFDEBUG(x) x
#else
#define _IFDEBUG(x)
#endif

#include "BulkAdapter.hpp"

namespace timestamped_dabalite {
  template<typename valT, typename timeT>
  class __AggT {
  public:
      valT _val;
      timeT _timestamp;
    __AggT() {}
    __AggT(valT val_, timeT timestamp_)
      : _val(val_), _timestamp(timestamp_) {}
  };

  template<typename binOpFunc,
           typename Timestamp,
           typename queueT=ChunkedArrayQueue<__AggT<typename binOpFunc::Partial, Timestamp> > >
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef Timestamp timeT;
    typedef __AggT<aggT, timeT> AggT;

    Aggregate(binOpFunc binOp_, aggT identE_)
      : _q(), _binOp(binOp_), _identE(identE_),
        _midSum(identE_), _backSum(identE_) {
      l = _q.begin(), b = _q.begin();
      a = _q.begin(), r = _q.begin();
    }

    size_t size() { return _q.size(); }

    void insert(timeT const& time, inT const& v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      _IFDEBUG(__debugPtrs(););
      aggT lifted = _binOp.lift(v);
      _backSum = _binOp.combine(_backSum, lifted);

      _q.push_back(AggT(lifted, time));

      _step();
    }

    void evict() {
      _IFDEBUG(std::cerr << "evicting" << std::endl;);
      _q.pop_front();

      _step();
    }

    outT query() {
      if (_q.size() > 0) {
        aggT alpha = _get_alpha(), back = _get_back();

        return _binOp.lower(_binOp.combine(alpha, back));
      }
      else return _binOp.lower(_identE);
    }

    timeT oldest() {
        return _q.front()._timestamp;
    }

    timeT youngest() {
        return _q.back()._timestamp;
    }

    outT naive_query() {
      // no longer supported because actual elements have been thrown out
      throw 0;
    }

  private:
    typedef queueT dequeT;
    typedef typename dequeT::iterator iterT;

    dequeT _q;

    // pointers into the queue
    iterT l,r,a,b;

    // the binary operator deck
    binOpFunc _binOp;
    aggT _identE;

    // extra sums
    aggT _midSum, _backSum;

    inline void _step() {
      _IFDEBUG(std::cerr << "begins _step::" << std::endl;);
      _IFDEBUG(__debugPtrs(););
      if (l == b) {
        _flip();
      _IFDEBUG(std::cerr << "after flip:"
                  << "l <-> r: " << std::distance(l, r)
                  << ", r <-> a: " << std::distance(r, a)
                  << std::endl);
      }
      _IFDEBUG(__debugPtrs(););

      // work if front stuff isn't empty
      if (_q.begin() != b) {
        _IFDEBUG(__debugPtrs(););
        if (a != r) {
          // a moves left
          assert(r!=a);
          _IFDEBUG(std::cerr << "r=!a, a moves left" << std::endl;);
          auto prev_delta = _get_delta();
          --a;
          a->_val = _binOp.combine(a->_val, prev_delta);
        }
        // advance l (to the right)
        if (l != r) { // l moves by itself until hitting r
          _IFDEBUG(std::cerr << "l!=r, advancing l forward" << std::endl;);
          //          auto gamma = _get_gamma();
          //          auto delta = _get_delta();
          l->_val = _binOp.combine(l->_val, _midSum);
          assert(l!=_q.end());
          ++l;
        } else { // moves together with r (and perhaps a)
          _IFDEBUG(std::cerr << "l==r, advancing l, free riding" << std::endl;);
          assert(l!=_q.end());
          ++l; ++r; ++a;
          _midSum = _get_delta();
          assert(l==r && a==l);
        }
      }
      else {
        // if empty, reset the preaggregated sums
        _backSum = _midSum = _identE;
      }
  #ifdef CHECK_INVARIANTS
      assert_ps_invariants();
  #endif
      _IFDEBUG(__debugPtrs();)
    }

    int iterLoc(iterT t) {
      int loc = 0;
      for (iterT it=_q.begin();it!=t;it++, loc++) {
        if (it==_q.end()) return -1;
      }
      return loc;
    }

    void  __debugPtrs(){
      std::cerr << "[l="<< iterLoc(l) <<
        ", r="<< iterLoc(r) <<
        ", a="<< iterLoc(a) <<
        ", b="<< iterLoc(b) <<
        ", sz="<< _q.size() << "]" << std::endl;

      for (iterT it=_q.begin(); it!=_q.end();it++) {
        std::cerr << "(" << it->_val << ", " << it->_agg << ")";
      }
      std::cerr << std::endl;
    }

    aggT partial_sum(iterT p, iterT q) {
      aggT accum = _identE;
      for (iterT it=p; it!=q; it++){
        accum = _binOp.combine(accum, it->_val);
      }
      return accum;
    }

    void assert_ps_invariants() {
      for (iterT it=_q.begin(); it!=_q.end();it++) {
        if (a==it) { std::cerr << "|a|"; }
        if (b==it) { std::cerr << "|b|"; }
        if (l==it) { std::cerr << "|l|"; }
        if (r==it) { std::cerr << "|r|"; }
        std::cerr << it->_val << " ";
      }
      if (a==_q.end()) { std::cerr << "|a|"; }
      if (b==_q.end()) { std::cerr << "|b|"; }
      if (l==_q.end()) { std::cerr << "|l|"; }
      if (r==_q.end()) { std::cerr << "|r|"; }

      std::cerr << std::endl;

      assert(_q.begin()<=l);
      assert(l<=r);
      assert(r<=a);
      assert(a<=b);
      assert(b<=_q.end());


      // size asserts
      auto sizeOfUnprocessed = std::distance(l, b);
      auto sizeOfFront = std::distance(_q.begin(), b);
      auto sizeOfBack = std::distance(b, _q.end());
      std::cerr << "size: unproc=" << sizeOfUnprocessed <<
        ", front="<< sizeOfFront <<
        ", back="<< sizeOfBack << std::endl;
      assert(_q.size()==0 || sizeOfUnprocessed + 1 == (sizeOfFront - sizeOfBack));
      assert(sizeOfBack <= sizeOfFront);
      assert(std::distance(l, r) == std::distance(r, a));
      std::cerr << "l <-> r: " << std::distance(l, r)
                << ", r <-> a: " << std::distance(r, a)
                << ", a <-> b: " << std::distance(a, b)
                << std::endl;
    }

    inline bool is_back_empty() { return b == _q.end(); }
    inline bool is_front_empty() { return b == _q.begin(); }
    inline bool is_delta_empty() { return a == b; }
    inline bool is_gamma_empty() { return a == r; }
    inline aggT _get_back() { return _backSum; }
    inline aggT _get_alpha() { return is_front_empty() ? _identE : _q.front()._val; }
    inline aggT _get_delta() { return is_delta_empty() ? _identE : a->_val; }
    inline aggT _get_gamma() { return is_gamma_empty() ? _identE : (a-1)->_val; }
    inline void _flip() {
      _IFDEBUG(std::cerr << "flipping" << std::endl;);
      l = _q.begin(); r = b;
      a = _q.end(); b = _q.end();
      _midSum = _backSum;
      _backSum = _identE;

    }
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

namespace timestamped_rb_dabalite {
    template<typename binOpFunc,
             typename Timestamp,
             size_t MAX_CAPACITY>
    using Aggregate = timestamped_dabalite::Aggregate <
            binOpFunc,
            Timestamp,
            RingBufferQueue<
                timestamped_dabalite::__AggT<
                    typename binOpFunc::Partial, 
                    Timestamp
                >,
                MAX_CAPACITY
            > 
        >;

  template <size_t MAX_CAPACITY, typename timeT, class BinaryFunction, class T>
  Aggregate<BinaryFunction, timeT, MAX_CAPACITY> make_aggregate(BinaryFunction f, T elem) {
      return Aggregate<BinaryFunction, timeT, MAX_CAPACITY>(f, elem);
  }

  template <typename BinaryFunction, typename timeT, size_t MAX_CAPACITY>
  struct MakeAggregate {
    template <typename T>
    Aggregate<BinaryFunction, timeT, MAX_CAPACITY> operator()(T elem) {
        BinaryFunction f;
        return make_aggregate<MAX_CAPACITY, timeT, BinaryFunction>(f, elem);
    }
  };      

}

#endif
