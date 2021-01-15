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
private:
    struct ring_buffer {
        ring_buffer()
            : buffer(NULL), size(0), capacity(0), front(0), back(0) {};

        ring_buffer(size_t c)
            : buffer(new E[c]), size(0), capacity(c), front(0), back(0) {
                front_ptr = back_ptr = buffer;
            };

        ~ring_buffer() {
            if (buffer != NULL)
                delete[] buffer;
        }

        E* buffer;
        int size, capacity;
        int front, back;
        E* front_ptr;
        E* back_ptr;
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
            if (pos >= rb->capacity)
                pos -= rb->capacity;
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
            rescale_to(THRES*_rb->capacity, n+1);

        _rb->size++;
        *(_rb->back_ptr) = elem;
        _rb->back_ptr++, _rb->back++;        

        if (_rb->back >= _rb->capacity) {
            _rb->back -= _rb->capacity;
            _rb->back_ptr -= _rb->capacity;
        }
    }

    void pop_front() {
        _rb->front++, _rb->front_ptr++;
        if (_rb->front >= _rb->capacity) {
            _rb->front -= _rb->capacity;
            _rb->front_ptr -= _rb->capacity;
        }
        _rb->size--;

        // only check to downsize if the ring buffer is in dynamic mode
        // i.e., no fixed preset capacity.
        if constexpr (PRESET_CAP == 0) {
            int n = size();
            if (n <= _rb->capacity/(2*THRES))
                rescale_to(_rb->capacity/THRES, n);
        }
    }

    E front() {
        assert(size() > 0);
        // return _rb->buffer[_rb->front];
        return *(_rb->front_ptr);
    }

    E back() {
        assert(size() > 0);
        E* trueBackPtr = _rb->back_ptr - 1;
        if (trueBackPtr < _rb->buffer) 
            _rb->buffer += _rb->capacity;
        // return _rb->buffer[bi];
        return *trueBackPtr;
    }

    // const iterator begin() { return iterator(_rb->front, _rb); }
    // const iterator end() { return iterator(_rb->front + _rb->size, _rb); }
    const iterator begin() { return iterator(_rb->front_ptr, _rb); }
    const iterator end() { return iterator(_rb->back_ptr, _rb); }


private:
    const int THRES = 2;
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
        int n = size(), f_src = _rb->front, f_dst = _rb->front;
        for (int index=0;index<n;++index) {
            if (f_dst >= new_size) f_dst -= new_size;
            if (f_src >= old_cap) f_src -= old_cap;
            rescaled_buffer[f_dst++] = _rb->buffer[f_src++];
        }

        std::swap(rescaled_buffer, _rb->buffer);

        delete[] rescaled_buffer;

        _rb->front %= new_size, _rb->back = (_rb->front + n)%new_size;
        _rb->front_ptr = _rb->buffer + _rb->front;
        _rb->back_ptr = _rb->buffer + _rb->back;
        _rb->capacity = new_size;
    }
public:
    ring_buffer* _rb;
};

#endif
