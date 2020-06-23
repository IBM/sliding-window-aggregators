#ifndef __OKASAKISQUEUE_H_
#define __OKASAKISQUEUE_H_

#include<memory>
#include<functional>
#include<iostream>
#include<iterator>
#include<cassert>
#include<cstdint>
#include<algorithm>

#ifdef DEBUG
#define _IFDEBUG(x) x
#else
#define _IFDEBUG(x)
#endif

namespace okasaki {
  using std::shared_ptr;
  using std::make_shared;


  template <typename T>
  inline size_t __sz(T t) { return (NULL==t)?0:t->sz; } 
  
  template <typename E, typename S>
  class agg_cons {
  public:
    typedef agg_cons<E, S> node;
    typedef shared_ptr<node> cons_ptr;
    typedef std::function<cons_ptr(cons_ptr,cons_ptr,cons_ptr)> RotF;

    agg_cons(E hd_, S agg_, cons_ptr tl_) {
      forced=true;
      cached.hd=hd_; cached.tl=tl_;
      agg=agg_;
      sz = 1 + __sz(tl_);
    }

    agg_cons(S agg_, cons_ptr rotL, cons_ptr rotR, cons_ptr rotA, RotF rf) {
      forced=false;
      agg=agg_;
      susp.rotL=rotL; susp.rotR=rotR; susp.rotA=rotA;
      susp.rotFunc = rf;
      sz = __sz(rotL) + __sz(rotR) + __sz(rotA);
    }


    inline E hd() {
      if (!forced) this->force();
      return cached.hd;
    }
    inline cons_ptr tl() {
      if (!forced) this->force();
      return cached.tl;
    }

    inline void force() {
      cons_ptr nl = susp.rotFunc(susp.rotL, susp.rotR, susp.rotA);
      cached.hd = nl->hd();
      cached.tl = nl->tl();
      forced=true;
    }

    struct {
      E hd;
      cons_ptr tl;
    } cached;
    struct {
      cons_ptr rotL, rotR, rotA;
      RotF rotFunc;
    } susp;
    bool forced;
    S agg;
    size_t sz;
  };

  template<typename binOpFunc>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef agg_cons<aggT, aggT> AggNodeT;
    typedef shared_ptr<AggNodeT> cons_ptr;
    
    Aggregate(binOpFunc binOp_, aggT identE_) :
      F(), B(), N(), _binOp(binOp_), _identE(identE_) {}

    size_t size() { return __sz(F) + __sz(B); }

    void insert(inT v) {
      _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
      auto prev = aggOf(B);
      aggT lifted = _binOp.lift(v);
      B = make_shared<AggNodeT>(lifted, _binOp.combine(prev, lifted), B);
      step();
    }

    void evict() {
      _IFDEBUG(std::cerr << "evicting" << std::endl;);
      F=F->tl();
      step();
    }

    outT query() {
      auto bp = aggOf(B), fp = aggOf(F);

      return _binOp.lower(_binOp.combine(fp, bp));
    }

    outT naive_query() {
      // todo: how can we support naive_query for real?
      return _binOp.lower(_identE);
    }

    ~Aggregate() {
      while (N!=NULL) { N = N->tl(); }
      while (B!=NULL) { B = B->tl(); }
      while (F!=NULL) { F = F->tl(); }
    }
  private:
    inline aggT aggOf(cons_ptr nn) { return (NULL!=nn)?nn->agg:_identE; }
    
    cons_ptr rot(cons_ptr L, cons_ptr R, cons_ptr A) {
      auto rv = R->hd();
      auto as = aggOf(A);
      cons_ptr nextA = make_shared<AggNodeT>(rv, _binOp.combine(rv, as), A);
      
      if (NULL==L) return nextA; // no more L

      
      aggT la=aggOf(L->tl()),ra=aggOf(R->tl()), naa = aggOf(nextA);
      cons_ptr susp_tl =
        make_shared<AggNodeT>(
                              _binOp.combine(la, _binOp.combine(ra, naa)),
                              L->tl(),
                              R->tl(),
                              nextA,
                              [this](cons_ptr x, cons_ptr y, cons_ptr z)-> cons_ptr {
                                return this->rot(x,y,z); });

      return make_shared<AggNodeT>(L->hd(), _binOp.combine(L->hd(), aggOf(susp_tl)), susp_tl);
    }
    inline void step() {
      if (NULL==N) {
        _IFDEBUG(std::cerr << "flipping" << std::endl;);
        cons_ptr newF = rot(F, B, cons_ptr()); // last is NULL
        F=N=newF; B.reset(); // set B to NULL, releasing the old one
      }
      else N = N->tl();
    }
    cons_ptr F, B, N;
    
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
