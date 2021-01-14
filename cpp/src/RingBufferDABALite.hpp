#pragma once

#include<vector>
#include<iostream>
#include<iterator>
#include<cassert>
#include "RingBufferQueue.hpp"

#ifdef DEBUG
#define _IFDEBUG(x) x
#else
#define _IFDEBUG(x)
#endif

namespace rb_dabalite {
  template<typename valT>
  class __AggT {
  public:
      valT _val;
    __AggT() {}
    __AggT(valT val_)
      : _val(val_) {}
  };

  template<typename binOpFunc, size_t MAX_CAPACITY>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef __AggT<aggT> AggT;

    Aggregate(binOpFunc binOp_, aggT identE_)
      : _q(), _binOp(binOp_), _identE(identE_),
        _midSum(identE_), _backSum(identE_) {
      l = _q.begin(), b = _q.begin();
      a = _q.begin(), r = _q.begin();
    }

    size_t size() { return _q.size(); }

    void insert(inT v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);      
      aggT lifted = _binOp.lift(v);
      _backSum = _binOp.combine(_backSum, lifted);

      _q.push_back(AggT(lifted));

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

    outT naive_query() {
      // no longer supported because actual elements have been thrown out
      throw 0;
    }

  private:
    typedef RingBufferQueue<AggT, MAX_CAPACITY> dequeT;
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

  template <size_t MAX_CAPACITY, class BinaryFunction, class T>
  Aggregate<BinaryFunction, MAX_CAPACITY> make_aggregate(BinaryFunction f, T elem) {
    return Aggregate<BinaryFunction, MAX_CAPACITY>(f, elem);
  }

  template <typename BinaryFunction, size_t MAX_CAPACITY>
  struct MakeAggregate {
    template <typename T>
    Aggregate<BinaryFunction, MAX_CAPACITY> operator()(T elem) {
      BinaryFunction f;
      return make_aggregate(f, elem);
    }
  };
}