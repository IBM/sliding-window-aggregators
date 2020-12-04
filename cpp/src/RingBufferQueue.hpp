#ifndef __RING_BUFFER_QUEUE_H_
#define __RING_BUFFER_QUEUE_H_

#include<cstddef>
#include<iterator>
#include<algorithm> 
#include<cassert>

template <typename E>
class RingBufferQueue {
public:
    // TODO: iterator
    struct iterator {
    };

    RingBufferQueue() 
        : _rb(new ring_buffer(MAGIC_MINIMUM_RING_SIZE)) {
    }

    ~RingBufferQueue() {
        delete _rb;
    }

    size_t size() { return _rb->size; }

    void push_back(E elem) {
        int n = size();
        if (n >= _rb->capacity) 
            rescale_to(THRES*_rb->capacity, n+1);

        _rb->size++; 
        _rb->buffer[_rb->back++] = elem;
        _rb->back %= _rb->capacity; 
    }

    void pop_front() {
        _rb->front = (1 + _rb->front)%_rb->capacity;
        _rb->size--;

        int n = size();
        if (n <= _rb->capacity/(2*THRES)) 
            rescale_to(_rb->capacity/THRES, n);
    }

    E front() {
        assert(size() > 0);
        return _rb->buffer[_rb->front];
    }

    E back() {
        assert(size() > 0);
        int c = _rb->capacity;
        return _rb->buffer[(_rb->back + c - 1)%c];
    }

    const iterator & begin() { throw 1; } // TODO:
    const iterator & end() { throw 1; } // TODO:
  

private:
    const int THRES = 2;
    const int MAGIC_MINIMUM_RING_SIZE = 2;
    struct ring_buffer {
        ring_buffer()
            : buffer(NULL), size(0), capacity(0), front(0), back(0) {};

        ring_buffer(size_t c) 
            : buffer(new E[c]), size(0), capacity(c), front(0), back(0) {};

        ~ring_buffer() { delete buffer; }
        
        E* buffer;
        int size, capacity;
        int front, back;    
    };

    void rescale_to(size_t new_size, size_t ensure_size) {
        new_size = std::max(new_size, (size_t) MAGIC_MINIMUM_RING_SIZE);

        if (ensure_size > new_size) { throw 1; } // this should never happen

        E* rescaled_buffer = new E[new_size];

        // pack and reindex the next pointers
        size_t old_cap = _rb->capacity;
        int n = size(), f = _rb->front;
        for (int index=0;index<n;++index) {
            rescaled_buffer[index] = _rb->buffer[(f + index)%old_cap];        
        }

        std::swap(rescaled_buffer, _rb->buffer);

        delete[] rescaled_buffer;
        
        _rb->front = 0, _rb->back = n;
    }

    ring_buffer* _rb;
};


#endif