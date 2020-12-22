#pragma once

#include<iostream>
#include<iterator>
#include<cassert>
#include "RingBufferQueue.hpp"

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
            : _q(), _num_flipped(0), _binOp(binOp_), _backSum(identE_), _identE(identE_) {}

        size_t size() { return _q.size(); }

        void insert(inT v) {
            aggT lifted = _binOp.lift(v);
            _backSum = _binOp.combine(_backSum, lifted);
            _q.push_back(AggT(lifted));
        }

        void evict() {
            if (front_empty())
                flip();
            _q.pop_front();
            _num_flipped--;
        }

        outT query() {
            auto bp = _backSum;
            auto fp = (front_empty())?_identE:_q.front()._val;

            // std::cerr << "prequery: " << _binOp.combine(fp, bp) << std::endl;
            auto answer = _binOp.lower(_binOp.combine(fp, bp));
            // std::cerr << "query: " << bp << "--" << fp << "--" << answer << std::endl;
            return  answer;
        }
    private:
        inline bool front_empty() { return _num_flipped == 0; }
        inline int decr(int it) {
            it--;
            if (it < 0)
                it += _q._rb->capacity;
            return it;
        }
        inline void flip() {
            // std::cerr << "evict: flippping" << std::endl;
            // front is empty, let's turn the "stack" implicity.
            iterT it = _q.end() - 1;
            size_t n = size();
            aggT running_sum = _identE;
            // std::cerr << "evict: ++++ (initial) running_sum " << running_sum << std::endl;
            for (size_t rep=0;rep<n;rep++) {
                // std::cerr << "evict: ++++ val " << local_agg._val << " ";
                running_sum = _binOp.combine(it->_val, running_sum);
                // std::cerr << "running_sum " << running_sum << std::endl;
                it->_val = running_sum;
                --it;
            }
            // reset the "back" stack
            _backSum = _identE;
            _num_flipped = n;
        }

        queueT _q;
        typedef typename queueT::iterator iterT;
        size_t _num_flipped;
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