#ifndef __RING_BUFFER_QUEUE_H_
#define __RING_BUFFER_QUEUE_H_

#include<cstddef>
#include<iterator>
#include<algorithm>
#include<cassert>
//
// ** if PRESET_CAP == 0, the RingBufferQueue operates in dynamic mode. That is,
// the buffer will resize automatically. 
//
// ** if PRESET_CAP > 0, the RingBufferQueue operates in static mode. That is,
// it is initialized to a fixed size as specified by PRESET_CAP. In this mode, 
// resizing is *not* allowed.

template <typename E, size_t PRESET_CAP=0>
class RingBufferQueue {
public:
    struct iterator;
private:
    struct ring_buffer {
        ring_buffer()
            : buffer(NULL), size(0), capacity(0) {};

        ring_buffer(size_t c)
            : buffer(new E[c]), size(0), capacity(c) {
                front_it = iterator((size_t) 0, this);
                back_it = iterator((size_t) 0, this);
            };

        ~ring_buffer() {
            if (buffer != NULL)
                delete[] buffer;
        }

        E* buffer;
        int size, capacity;
        iterator front_it, back_it;
    };
public:
    // iterator:
    //    WARNING: iterators aren't guaranteed to work after modifications to the collection.
    //    The current implementation should survive push_back and pop_front as long as there is
    //    no resizing of the underlying buffer. This is to allow further optimization
    //    to data access (A previous version did support this and was rather slow).
    struct iterator {
        typedef std::input_iterator_tag iterator_category;
        typedef iterator  _Self;
        typedef E* pointer;
        typedef E& reference;
        typedef E value_type;
        typedef size_t difference_type;

        iterator()
            : it(NULL), rb(NULL) {}

        iterator(size_t pos, ring_buffer* rb_)
            : rb(rb_) {
            while (pos >= rb->capacity) { pos -= rb->capacity; }
            it = rb->buffer + pos;
        }
        iterator(pointer loc_, ring_buffer* rb_)
            : it(loc_), rb(rb_) {}

        iterator(const iterator &that)
            : it(that.it), rb(that.rb) {}

        _Self& operator++() {
            ++it;
            if (it >= (rb->buffer + rb->capacity))
                it -= rb->capacity;
            return *this;
        }

        _Self operator++(int) {
            _Self tmp(*this); // copy
            operator++(); // pre-increment
            return tmp;   // return old value
        }

        _Self& operator--(){
            --it;
            if (it < rb->buffer)
                it += rb->capacity;
            return *this;
        }

        _Self operator--(int) {
            _Self tmp(*this); // copy
            operator--(); // pre-increment
            return tmp;   // return old value
        }

        // special case of n == 1
        _Self operator+(difference_type __n) const {
            assert(__n == 1);
            _Self it = iterator(*this);
            ++it;
            return it;
        }

        // special case of n == 1
        _Self operator-(difference_type __n) const {
            assert(__n == 1);
            _Self it = iterator(*this);
            --it;
            return it;
        }

        _Self& operator=(const _Self& other) { // copy assignment
            if (this != &other) { // self-assignment check expected
                it = other.it, rb = other.rb;
            }
            return *this;
        }

        E& operator*() const {
            return *it;
        }

        E* operator->() const {
            return it;
        }

        // comparison
        friend inline bool
        operator== (const iterator &x, const iterator &y){
            return x.it == y.it;
        }

        friend inline bool
        operator != (const iterator &x, const iterator &y) {
            return x.it != y.it;
        }
        // where in the world are we?
        pointer it; // where the iterator is pointing to
        ring_buffer* rb; // the actual ring buffer
    };

    RingBufferQueue() {
        // if in dynamic mode (i.e., no fixed preset cap)
        if constexpr (PRESET_CAP == 0)
            _rb = new ring_buffer(MAGIC_MINIMUM_RING_SIZE);
        else
            _rb = new ring_buffer(PRESET_CAP);
    }

    ~RingBufferQueue() {
        delete _rb;
    }

    size_t size() { return _rb->size; }

    void push_back(E elem) {
        int n = size();
        if (n+1 >= _rb->capacity)
            rescale_to(2*_rb->capacity, n+1);

        _rb->size++;
        *(_rb->back_it++) = elem;
    }

    void pop_front() {
        _rb->front_it++;
        _rb->size--;

        // only check to downsize if the ring buffer is in dynamic mode
        // i.e., no fixed preset capacity.
        if constexpr (PRESET_CAP == 0) {
            int n = size();
            if (n <= _rb->capacity/4)
                rescale_to(_rb->capacity/2, n);
        }
    }

    E front() {
        assert(size() > 0);
        // return _rb->buffer[_rb->front];
        return *(_rb->front_it);
    }

    E back() {
        assert(size() > 0);
<<<<<<< HEAD
        E* trueBackPtr = _rb->back_ptr - 1;
        if (trueBackPtr < _rb->buffer) 
            trueBackPtr += _rb->capacity;
        // return _rb->buffer[bi];
        return *trueBackPtr;
=======
        iterator true_back = _rb->back_it - 1;
        return *true_back;
>>>>>>> Perf optimization: all pointers
    }

    // const iterator begin() { return iterator(_rb->front, _rb); }
    // const iterator end() { return iterator(_rb->front + _rb->size, _rb); }
    const iterator begin() { return iterator(_rb->front_it); }
    const iterator end() { return iterator(_rb->back_it); }


private:
    const int MAGIC_MINIMUM_RING_SIZE = 4;

    void rescale_to(size_t new_size, size_t ensure_size) {
        // if the capacity has been prespecified, resizing is *not* allowed
        if constexpr (PRESET_CAP > 0)
            throw 1;

        new_size = std::max(new_size, (size_t) MAGIC_MINIMUM_RING_SIZE);

        if (ensure_size > new_size) { throw 1; } // this should never happen

        E* rescaled_buffer = new E[new_size];

        // copy while preserving the positions (modulo new_size)
        size_t old_cap = _rb->capacity;
        int n = size();
        E* src = _rb->front_it.it;
        E* dst = rescaled_buffer + (_rb->front_it.it - _rb->buffer);
        if (dst >= rescaled_buffer + new_size) dst -= new_size;
        for (int index=0;index<n;++index) {
            if (dst >= rescaled_buffer + new_size) dst -= new_size;
            if (src >= _rb->buffer + old_cap) src -= old_cap;
            *(dst++) = *(src++);
        }
        int front_index = _rb->front_it.it - _rb->buffer;
        std::swap(rescaled_buffer, _rb->buffer);

        delete[] rescaled_buffer;

        _rb->capacity = new_size;
        _rb->front_it = iterator((size_t) front_index, _rb);
        _rb->back_it = iterator((size_t) (front_index+n), _rb);
    }
public:
    ring_buffer* _rb;
};

#endif
