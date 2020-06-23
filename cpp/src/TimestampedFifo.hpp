#ifndef __TIMESTAMPED_FIFO_H__
#define __TIMESTAMPED_FIFO_H__

#include <cstddef>
#include <utility>
#include "ChunkedArrayQueue.hpp"

namespace timestampedfifo {
    template <typename Timestamp,
              typename FifoAggregate>
    class Aggregate {
    public:
        typedef typename FifoAggregate::inT inT;
        typedef typename FifoAggregate::aggT aggT;
        typedef typename FifoAggregate::outT outT;
        typedef Timestamp timeT;

        template <typename... Args>
        Aggregate(Args&&... args):
            _times(),
            _q(std::forward<Args>(args)...)
        {}

        size_t size() {
            return _q.size();
        }

        void evict() {
            _q.evict();
            _times.pop_front();
        }

        void insert(const Timestamp& timestamp, const inT& value) {
            _q.insert(value);
            _times.push_back(timestamp);
        }

        Timestamp oldest() {
            return _times.front();
        }

        Timestamp youngest() {
            return _times.back();
        }

        outT query() {
            return _q.query();
        }

    private:
        ChunkedArrayQueue<Timestamp> _times;
        FifoAggregate _q;
    };

    template <typename Timestamp, typename FifoAggregate, typename... Args>
    Aggregate<Timestamp, FifoAggregate> make_aggregate(Args&&... args) {
        return Aggregate<Timestamp, FifoAggregate>(std::forward<Args>(args)...);
    }

    template <typename Timestamp, typename FifoAggregate>
    struct MakeAggregate {
        template <typename... Args>
        Aggregate<Timestamp, FifoAggregate> operator()(Args&&... args) {
            return make_aggregate<Timestamp, FifoAggregate>(std::forward<Args>(args)...);
        }
    };
}

#endif // __TIMESTAMPED_FIFO_H__
