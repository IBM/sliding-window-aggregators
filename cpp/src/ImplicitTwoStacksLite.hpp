#pragma once

#include<iostream>
#include<iterator>
#include<cassert>
#include "RingBufferQueue.hpp"

// #ifdef DEBUG
// #define _IFDEBUG(x) x
// #else
// #define _IFDEBUG(x)
// #endif

namespace implicit_twostackslite {
    using namespace std;

    template<typename aggT>
    class __AggT {
    public:
        aggT _val;
        __AggT() {}
        __AggT(aggT val_)
            : _val(val_) {}
    };

    template<typename binOpFunc,
           typename queueT=RingBufferQueue<__AggT<typename binOpFunc::Partial>>>
    class Aggregate {
    public:
        typedef typename binOpFunc::In inT;
        typedef typename binOpFunc::Partial aggT;
        typedef typename binOpFunc::Out outT;
        typedef __AggT<aggT> AggT;

        Aggregate(binOpFunc binOp_, aggT identE_) 
            : _q(), _binOp(binOp_), _backSum(identE_), _identE(identE_) {
                _split = _q.begin();
        }

        size_t size() { return _q.size(); }

        void insert(inT v) {
            // _IFDEBUG(std::cerr << "inserting " << v << std::endl;);
            // std::cerr << "--inserting " << v << std::endl;
            aggT lifted = _binOp.lift(v);
            _backSum = _binOp.combine(_backSum, lifted);
            // _IFDEBUG(std::cerr << "inserted backSum=" << _backSum << std::endl;);
            // std::cerr << "inserted backSum=" << _backSum << std::endl;
            _q.push_back(AggT(lifted));
        }

        bool front_empty() { return _q.begin() == _split; }

        void evict() { 
            if (front_empty()) {
                std::cerr << "evict: flippping" << std::endl;
                // front is empty, let's turn the "stack" implicity.
                iterT front = _q.begin();
                iterT it = _q.end()-1;
                aggT running_sum = _identE;
                // std::cerr << "evict: ++++ (initial) running_sum " << running_sum << std::endl;
                while (it != front) {
                    // std::cerr << "evict: ++++ val " << it->_val << " ";
                    running_sum = _binOp.combine(it->_val, running_sum);
                    it->_val = running_sum;
                    // std::cerr << "running_sum " << running_sum << std::endl;
                    --it;
                }
                _split = _q.end();
                _backSum = _identE;
            }
            _q.pop_front();
            if (_q.size() == 0) _split = _q.begin();
        }

        outT query() {
            auto bp = _backSum;
            auto fp = (size()==0 || front_empty())?_identE:_q.front()._val;

            // std::cerr << "prequery: " << _binOp.combine(fp, bp) << std::endl;
            auto answer = _binOp.lower(_binOp.combine(fp, bp));
            // std::cerr << "query: " << bp << "--" << fp << "--" << answer << std::endl;
            return  answer;
        }
    private:
        queueT _q;
        typedef typename queueT::iterator iterT;
        iterT _split;
        // the binary operator deck
        binOpFunc _binOp;
        aggT _backSum;
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