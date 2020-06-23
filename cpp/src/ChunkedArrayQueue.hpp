#ifndef __CHUNKED_ARRAY_QUEUE_H_
#define __CHUNKED_ARRAY_QUEUE_H_

#include<array>
#include<iterator>
#include<cassert>
#include<cstdint>
#include<algorithm>

#define MAGIC_CHUNK_SIZE 512

template <typename E>
class ChunkedArrayQueue {
  // how many elements per chunk? we answer this in an ad hoc way:
  // simply make sure that each chunk sums to a certain size
  static const int CAPACITY = (MAGIC_CHUNK_SIZE < sizeof(E))? 1 : MAGIC_CHUNK_SIZE/sizeof(E);
  struct Chunk {
    Chunk(Chunk *prev_=NULL, Chunk *next_=NULL)
      : prev(prev_), next(next_) {
      elems = new E[CAPACITY];
    }
    ~Chunk() { delete [] elems; }
    Chunk *prev, *next;
    E *elems;
  };

public:
  struct iterator {
    typedef std::input_iterator_tag iterator_category;
    typedef iterator  _Self;
    typedef E* pointer;
    typedef E& reference;
    typedef E value_type;
    typedef size_t difference_type;

    iterator()
      : chunk(NULL), index(0) {}
    
    iterator(Chunk *chunk_, size_t index_)
      : chunk(chunk_), index(index_) {}

    iterator(const iterator &that)
      : chunk(that.chunk), index(that.index) {}

    void resetTo(Chunk *chunk_, size_t index_) {
      chunk = chunk_; index = index_;
    }
    
    _Self& operator++(){
      index++;
      if (index == CAPACITY) {
        assert (NULL != chunk->next);
        chunk = chunk->next, index = 0;
      }
      return *this;
    }
    _Self operator++(int) {
        _Self tmp(*this); // copy
        operator++(); // pre-increment
        return tmp;   // return old value
    }
    
    _Self& operator--(){
      if (index==0) {
        assert (NULL != chunk->prev);
        chunk = chunk->prev, index = CAPACITY - 1;
      }
      else index--;
      return *this;
    }
    _Self operator--(int) {
      _Self tmp(*this); // copy
      operator--(); // pre-increment
      return tmp;   // return old value
    }
    _Self operator+(difference_type __n) const {
      assert(__n == 1);
      _Self it = iterator(chunk, index);
      ++it;
      return it;
    }

    _Self operator-(difference_type __n) const {
      assert(__n == 1);
      _Self it = iterator(chunk, index);
      --it;
      return it;
    }
    _Self& operator=(const _Self& other) // copy assignment
    {
      if (this != &other) { // self-assignment check expected
        chunk = other.chunk, index = other.index;
        return *this;
      }
      return *this;
    }
    
    E& operator*() const {return chunk->elems[index];}
    E* operator->() const { return chunk->elems + index;}
    
    // where in the world are we?
    Chunk *chunk;
    size_t index;
  
    // comparison
    friend inline bool
    operator== (const iterator &x, const iterator &y){
      return (x.chunk == y.chunk) && (x.index == y.index);
    }

    friend inline bool
    operator != (const iterator &x, const iterator &y){
      return (x.chunk != y.chunk) || (x.index != y.index);
    }

    // for assertion purposes only. extremely slow.
    friend inline bool
    operator <= (const iterator &x, const iterator &y){
      iterator cur=x;
      for (;;) {
        if (cur == y) return true;
        if (cur.index == CAPACITY-1 && NULL == cur.chunk->next) return false;
        cur++;
      }
    }
    
  };

  ChunkedArrayQueue()
    : _size(0) {
    Chunk *c = new Chunk();
    _begin = iterator(c, 0);
    _end = iterator(c, 0);
  }

  ~ChunkedArrayQueue() {
    Chunk *cur = _begin.chunk;
    while (cur!=NULL) {
      Chunk *next = cur->next;
      delete cur;
      cur = next;
    }
  }

  size_t size() { return _size; }

  void push_back(E elem) {
    *_end = elem;
    _end.index++;
    if (_end.index == CAPACITY) {
      assert (NULL == _end.chunk->next);
      Chunk *c = new Chunk(_end.chunk, NULL);
      _end.chunk->next = c;
      _end.resetTo(c, 0);
    }
    _size++;
  }

  void pop_front() {
    assert (size() > 0);
    _begin.index++;
    if (_begin.index == CAPACITY) {
      Chunk *next = _begin.chunk->next;
      assert (NULL != next);
      delete _begin.chunk;
      next->prev = NULL;
      _begin.resetTo(next, 0);
    }
    _size--;
  }

  E front() {
    assert (size() > 0);
    return *_begin;
  }

  E back() {
    assert (size() > 0);
    return *(_end-1);
  }

  const iterator & begin() { return _begin; }
  const iterator & end() { return _end; }
  
private:
  iterator _begin, _end;
  size_t _size;
  
};

#endif
