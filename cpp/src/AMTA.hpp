#pragma once

#include <cassert>
#include <deque>
#include <iostream>
#include <vector>

namespace amta {

using namespace std;

template <typename _timeT, typename binOpFunc> class Aggregate {
public:
  typedef typename binOpFunc::In inT;
  typedef typename binOpFunc::Partial aggT;
  typedef typename binOpFunc::Out outT;
  typedef _timeT timeT;

private:
  class Node {
    aggT agg[2];
    timeT latestTime[2];
    Node* children[2];
    int arity() { return (children[0] != NULL) + (children[1] != NULL); }
  };

  binOpFunc _binOp;
  size_t _size;
  vector<Node *> tails;
  deque<aggT> frontStack;

public:
  Aggregate(binOpFunc binOp)
      : _binOp(binOp), _size(0), tails(), frontStack() {}

  ~Aggregate() {
      // TODO: write me
  }

  void evict() { throw 1; }
  void bulkEvict() { throw 1; }
  void bulkInsert(vector<pair<timeT, inT>> entries) {
    bulkInsert(entries.begin(), entries.end());
  }

  template <class Iterator>
  void bulkInsert(Iterator begin, Iterator end) {
      for(auto it=begin;it!=end;it++) {
          auto [time, value] = *it;
          insert(time, value);
      }
  }

  timeT oldest() const { throw 1; }

  outT query() const { throw 1; }

  size_t size() { return _size; }

  timeT youngest() const { throw 1; }

  void insert_lifted(timeT const& time, aggT const& liftedValue) {
      throw 1;
  }

  void insert(timeT const& time, inT const& value) {
    insert_lifted(time, _binOp.lift(value));
  }

  void insert(inT const& val) {
    if (0 == _size) {
      insert(0, val);
    } else {
      timeT const time = 1 + youngest();
      insert(time, val);
    }
};

template <typename timeT, class BinaryFunction, class T>
Aggregate<timeT, BinaryFunction>
make_aggregate(BinaryFunction f, T elem) {
    return Aggregate<timeT, BinaryFunction>(f);
}

template <typename BinaryFunction, typename timeT>
struct MakeAggregate {
  template <typename T>
  Aggregate<timeT, BinaryFunction> operator()(T elem) {
    BinaryFunction f;
    return make_aggregate<
      timeT, BinaryFunction
      >(f, elem);
  }
};
} // namespace amta
