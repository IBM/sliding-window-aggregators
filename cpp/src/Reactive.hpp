#ifndef __REACTIVE_H_
#define __REACTIVE_H_


namespace reactive {
  static size_t initial_size = 2; // todo: fix + double resize

  template<typename binOpFunc>
  class Aggregate {
  public:
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;

    typedef std::pair<size_t, size_t> rangeT;
    typedef std::deque<rangeT> workQT;
    
    // invariant: true size is 2*cap
    Aggregate(binOpFunc binOp_, aggT identE_)
      : cap(initial_size), sz(0), q(new aggT[2*cap]),
        frontPtr(0), backPtr(cap-1),
        workQ(), binOp(binOp_), identE(identE_)
    {}

    inline
    size_t size() { return sz; }
    
    void insert(inT v) {
      if (sz == cap) { doubleIt(); }
      backPtr = (1+backPtr)%cap;
      q[cap + backPtr] = binOp.lift(v);
      sz++;
      sched_insert(cap + backPtr, workQ);
    }
  
    void evict() {
      sched_insert(cap + frontPtr, workQ);
      assert(sz>0);
      frontPtr = (frontPtr+1)%cap;
      sz--;
      if (sz < cap/4) { halfIt(); }
    }
  
    outT query() {
      aggT res = propagate();
      return binOp.lower(res);
    }


    outT naive_query() {
      aggT sum = identE;
      for (size_t i=0;i<sz;i++)
        sum = binOp.combine(sum, q[cap + (frontPtr+i)%cap]);
      return binOp.lower(sum);
    }
  private:
    size_t cap, sz;
    size_t frontPtr, backPtr;
    aggT *q;
    std::deque<rangeT> workQ;
    binOpFunc binOp;
    aggT identE;

    inline size_t total_size(size_t cap) { return 2*cap; }

    void reallocTo(size_t ncap) {
      aggT *nq = new aggT[total_size(ncap)];
      for (size_t pos=0;pos<sz;pos++) { nq[ncap+pos] = q[cap + (frontPtr+pos)%cap]; }
      q = nq, cap = ncap;
      frontPtr=0, backPtr=sz-1;
      workQ.clear(), sched_append_range(cap+0, cap+sz-1, workQ), propagate();
    }
    
    void doubleIt() { reallocTo(2*cap); }
    void halfIt() { reallocTo(std::max(initial_size, cap/2)); }
    

    inline
    bool ok(size_t i, size_t bi, size_t fi, bool split) {
      if (split)
        return i<=bi || i>=fi;
      else
        return i>=fi && i<=bi;
    }

    inline
    aggT propagate() {
      if (sz==0) return identE;

      std::deque<rangeT> now(workQ), next;

      workQ.clear(); // incorporated, so no longer needed to flag them
      
      size_t fi = cap+frontPtr, bi = cap+backPtr;
      bool split = bi < fi;
      aggT pre = q[bi], suf = q[fi];

      while (!now.empty()) {
        for (auto & r : now) {
          size_t pb=r.first/2;
          size_t pe=r.second/2;
          if (pb < 1) break; // root reached
          for (size_t p=pb;p<=pe;++p) {
            size_t lp = 2*p, rp = 2*p+1;
            bool ok_l = ok(lp,bi,fi,split);
            bool ok_r = ok(rp,bi,fi,split);
            if (ok_l&&ok_r) q[p] = binOp.combine(q[lp], q[rp]);
            else if (ok_l) q[p] = q[lp];
            else q[p] = q[rp];
            //            std::cerr << "combining: " << left << " + " << right << std::endl;
          }
          if (pe>=pb) sched_append_range(pb, pe, next);
        }

        if (split) {
          if (0==(fi&1)) { suf = binOp.combine(suf, q[fi^1]); }
          if (1==(bi&1) && (bi>1)) { pre = binOp.combine(q[bi^1], pre); }
          //          std::cerr <<"XXX" << ".. suffix:" << suf << ", prefix: " << pre << std::endl;
        }
        fi /=2; bi /=2;
        now.swap(next); next.clear();
      }
    
      if (split)
        return binOp.combine(suf, pre);
      else
        return q[1];
    }
    
    inline void sched_append_range(size_t s, size_t t, std::deque<rangeT> & q) {
      if (!q.empty() && (s - q.back().second <= 1))
        q.back().second = std::max(t, q.back().second);
      else
        q.push_back(rangeT(s, t));
    }

    inline void sched_insert(size_t p, std::deque<rangeT> & q) {
      assert(q.size() <= 3);
      for (auto it=q.begin();it!=q.end();++it) {
        if (p+1 < it->first){
          q.insert(it, rangeT(p, p));
          return ;
        } else if (p+1 == it->first) {
          it->first = p;
          return ;
        } else if (p <= it->second) {
          return ;
        } else if (p == it->second+1) {
          it->second = p;
          auto next = it + 1;
          if (next != q.end() && p+1 == next->first) {
            it->second = next->second;
            q.erase(next);
          }
          return ;
        }
      }
      q.push_back(rangeT(p, p));
    }
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
