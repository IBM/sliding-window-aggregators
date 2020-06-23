#ifndef __FLATFIT_H__
#define __FLATFIT_H__

#include<vector>
#include<stack>
#include<iterator>
#include<cassert>

#ifdef DEBUG
#define _IFDEBUG(x) x
#else
#define _IFDEBUG(x)
#endif

namespace flatfit {
  template<typename valT, typename ptrT>
  class __AggT {
  public:
    valT _val;
    ptrT _next;
    __AggT() {}
    __AggT(valT val_)
      : _val(val_) {}
    __AggT(valT val_, ptrT next_)
      : _val(val_), _next(next_) {}
  };

  template<typename binOpFunc>
  class Aggregate {
  public:
    typedef uint32_t ptrT;
    typedef typename binOpFunc::In inT;
    typedef typename binOpFunc::Partial aggT;
    typedef typename binOpFunc::Out outT;
    typedef __AggT<aggT, ptrT> AggT;

    Aggregate(binOpFunc binOp_, aggT identE_)
      : _size(0), _buffer(),
        _ever_evicted(false),
        _front(0), _back(-1),
        _binOp(binOp_), _identE(identE_) {}

    size_t size() { return _size; }

    void insert(inT v) {
      if (_ever_evicted &&
          _size + 1 > (int) _buffer.size())
        throw 1; // this shouldn't happen: can't grow beyond capacity after the first evict

      int prev = (_size > 0)?_back:-1;
      AggT vAgg = AggT(_binOp.lift(v), 0);
      ++_back; ++_size;
      if (_ever_evicted) {
        _back %= _buffer.size();
        _buffer[_back] = vAgg;
      }
      else
        _buffer.push_back(vAgg);

      if (prev>=0)
        _buffer[prev]._next = _back;
    }

    void evict() {
      _ever_evicted = true;
      _front = (_front + 1)%_buffer.size();
      --_size;
    }


    outT query() {
      if (_size == 0)
        return _binOp.lower(_identE);

      // for non-empty cases
      std::stack<int> tracing_indices;
      for (int cur=_front;cur!=_back;cur=_buffer[cur]._next)
        tracing_indices.push(cur);

      aggT theSum = _identE;
      while (!tracing_indices.empty()) {
        int index = tracing_indices.top();
        theSum = _binOp.combine(_buffer[index]._val, theSum);
        _buffer[index] = AggT(theSum, _back);
        tracing_indices.pop();
      }

      return _binOp.lower(_binOp.combine(theSum, _buffer[_back]._val));
    }

    friend inline std::ostream& operator<<(std::ostream& os, Aggregate const& t) {
      os << "(f=" << t._front << ",b=" << t._back << ",s=" << t._size << ") -- ";
      for (int i=0;i<t._buffer.size();i++) {
        os << "[" << t._buffer[i]._val << "; " << t._buffer[i]._next << "]";
      }
      return os;
    }
  private:
    int _size;
    std::vector<AggT> _buffer;
    bool _ever_evicted;
    int32_t _front, _back;

    // the binary operator deck
    binOpFunc _binOp;
    aggT _identE;
  };

  template <class BinaryFunction, class T>
  Aggregate<BinaryFunction>
  make_aggregate(BinaryFunction f, T elem) {
    return Aggregate<BinaryFunction>(f, elem);
  }

  template <typename BinaryFunction>
  struct MakeAggregate {
    template <typename T>
    Aggregate<BinaryFunction> operator()(T elem) {
      BinaryFunction f;
      return make_aggregate<>(f, elem);
    }
  };
}
#endif
