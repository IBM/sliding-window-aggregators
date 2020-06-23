#ifndef __TTIMESTAMPED_STLDequeBased_H__
#define __TTIMESTAMPED_STLDequeBased_H__

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

namespace timestamped_daba {
  template<typename valT, typename aggT, typename timeT>
  class __AggT {
  public:
      valT _val;
      aggT _agg;
      timeT _timestamp;
    __AggT() {}
    __AggT(valT val_, aggT agg_, timeT timestamp_)
      : _val(val_), _agg(agg_), _timestamp(timestamp_) {}
  };

  template<typename T, bool on> struct AggCache {};
  
  template<typename T> struct AggCache<T, false> {
    AggCache(T cv_) {}
    T fetch(std::function<T(void)> fallback) {
      return fallback();
    }
    void setCache(std::function<T(void)> th) {}
  };
  template<typename T> struct AggCache<T, true> {
    AggCache(T cv_)
      : cache(cv_) {}
    T fetch(std::function<T(void)> fallback) {
      _IFDEBUG(std::cerr << "serving from cache" << std::endl);
      return cache;
    }
    void setCache(std::function<T(void)> th) { cache=th(); }
    T cache;
  };
  
  template<typename binOpFunc,
           typename Timestamp,
           bool toCache=false,
           typename queueT=ChunkedArrayQueue<__AggT<typename binOpFunc::Partial, typename binOpFunc::Partial, Timestamp>>>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef Timestamp timeT;
    typedef __AggT<aggT, aggT, timeT> AggT;
  
    Aggregate(binOpFunc binOp_, aggT identE_)
      : _q(), _binOp(binOp_), _identE(identE_),
        _cachedRA(identE_) {
      l = _q.begin(), b = _q.begin();
      a = _q.begin(), r = _q.begin();    
    }
  
    size_t size() { return _q.size(); }
    
    void insert(timeT const& time, inT const& v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      _IFDEBUG(__debugPtrs(););
      auto prev_back = _get_back();
      aggT lifted = _binOp.lift(v);
      auto back = _binOp.combine(prev_back, lifted);
  
      _q.push_back(AggT(lifted, back, time));
  
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
      aggT accum = _identE;
      for (iterT it=_q.begin(); it!=_q.end(); it++){
        accum = _binOp.combine(accum, it->_val);
      }
      return _binOp.lower(accum);
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

    // cache R + A
    AggCache<aggT, toCache> _cachedRA;
  
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
          a->_agg = _binOp.combine(a->_val, prev_delta);
        }
        // advance l (to the right)
        if (l != r) { // l moves by itself until hitting r
          _IFDEBUG(std::cerr << "l!=r, advancing l forward" << std::endl;);
          //          auto gamma = _get_gamma();
          //          auto delta = _get_delta();
          auto ra = _cachedRA.fetch([this]() -> aggT {
              return this->_binOp.combine(this->_get_gamma(), this->_get_delta());
            });
          l->_agg = _binOp.combine(l->_agg, ra);
          assert(l!=_q.end());
          ++l;
        } else { // moves together with r (and perhaps a)
          _IFDEBUG(std::cerr << "l==r, advancing l, free riding" << std::endl;);
          assert(l!=_q.end());
          ++l; ++r; ++a;
          assert(l==r && a==l);
        }
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
  
      for (iterT it=_q.begin();it != l; it++) {
        std::cerr << "=a> (" << it->_val << ", " << it->_agg << ")" << std::endl;
        std::cerr << "+a> (" << partial_sum(it, b) << std::endl;
        assert(it->_agg == partial_sum(it, b));
      }
      for (iterT it=l;it != r; it++) {
        std::cerr << "=b> (" << it->_val << ", " << it->_agg << ")" << std::endl;
        std::cerr << "+b> (" << partial_sum(it, r) << std::endl;
        assert(it->_agg == partial_sum(it, r));
      }
      for (iterT it=r;it != a; it++) {
        std::cerr << "=c> (" << it->_val << ", " << it->_agg << ")" << std::endl;
        std::cerr << "+c> (" << partial_sum(r, it+1) << std::endl;
        assert(it->_agg == partial_sum(r, it+1));
      }
      for (iterT it=a;it != b; it++) {
        std::cerr << "=d> (" << it->_val << ", " << it->_agg << ")" << std::endl;
        std::cerr << "+d> (" << partial_sum(it, b) << std::endl;
        assert(it->_agg == partial_sum(it, b));
      }
      for (iterT it=b;it != _q.end(); it++) {
        std::cerr << "=B> (" << it->_val << ", " << it->_agg << ")" << std::endl;
        std::cerr << "+B> (" << partial_sum(b, it+1) << std::endl;
        assert(it->_agg == partial_sum(b, it+1));
      }
  
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
    inline aggT _get_back() { return is_back_empty() ? _identE : _q.back()._agg; }
    inline aggT _get_alpha() { return is_front_empty() ? _identE : _q.front()._agg; }
    inline aggT _get_delta() { return is_delta_empty() ? _identE : a->_agg; }
    inline aggT _get_gamma() { return is_gamma_empty() ? _identE : (a-1)->_agg; }
    inline void _flip() {
      _IFDEBUG(std::cerr << "flipping" << std::endl;);
      l = _q.begin(); r = b;
      a = _q.end(); b = _q.end();
      _cachedRA.setCache([this]()-> aggT { return this->_get_gamma(); });
    }
  };

  template <bool caching, typename timeT, class BinaryFunction, class T>
  Aggregate<BinaryFunction, timeT, caching> make_aggregate(BinaryFunction f, T elem) {
    return Aggregate<BinaryFunction, timeT, caching>(f, elem);
  }

  template <typename BinaryFunction, typename timeT, bool caching>
  struct MakeAggregate {
    template <typename T>
    Aggregate<BinaryFunction, timeT, caching> operator()(T elem) {
      BinaryFunction f;
      return make_aggregate<caching, timeT, BinaryFunction>(f, elem);
    }
  };
}

#endif
