#pragma once

#include <cassert>
#include <deque>
#include <iostream>
#include <ostream>
#include <vector>
#include <iterator>

namespace amta {

using namespace std;
template <typename _timeT, typename binOpFunc>
class Aggregate {
public:
  typedef typename binOpFunc::In inT;
  typedef typename binOpFunc::Partial aggT;
  typedef typename binOpFunc::Out outT;
  typedef _timeT timeT;

private:
  struct Node {
    aggT agg[2];
    timeT latestTime[2];
    Node* children[2];
    Node* parent;
    int arity;
    Node(aggT a, Node* left) : parent(NULL), arity(0) { push_back(a, left); }
    void push_back(aggT a, Node* child) {
      agg[arity] = a; children[arity] = child; arity++;
      if (child != NULL) child->parent = this;
    }
    void pop_front() { arity = -1; }
    bool leftEmpty() { return arity == -1; }
    bool rightEmpty() { return arity == 1; }
    bool full() { return arity == 2; }

    ostream& printNode(ostream& os) const {
      os << "[";
      for (int i=0;i<arity;i++) os << this->agg[i] << ",";
      os << "]";
      return os;
    }
  };

  friend inline std::ostream& operator<<(std::ostream& os, Node const& x) {
    return x.printNode(os);
  }

  binOpFunc _binOp;
  size_t _size;
  vector<Node *> _tails;
  deque<aggT> _frontStack;
  aggT _identE, _frontSum, _backSum;

public:
  Aggregate(binOpFunc binOp, aggT identE_)
    : _binOp(binOp), _size(0), _tails(), _frontStack(),
      _identE(identE_), _frontSum(identE_), _backSum(identE_) {}

  ~Aggregate() {
      // TODO: write me
  }

  void print() {
    std::cout << "_tails:";
    for (int i = _tails.size() - 1; i >= 0; i--) {
      std::cout << " " << *(_tails[i]);
    }
    std::cout << std::endl;
    std::cout << "_frontSum=" << _frontSum << ", _backSum=" << _backSum
              << std::endl;
  }
  void rebuildFront() {
    if (_tails.empty()) { _frontSum = _identE; return ;}

    Node* c =  _tails.back();
    aggT agg = _identE;
    while (c != NULL) {
      Node* next = c->leftEmpty()?c->children[1]:c->children[0];
      if (c->full()) {
        agg = _binOp.combine(c->agg[1], agg);;
        _frontStack.push_back(agg);
        if (next == NULL) {
          aggT other = c->full()?c->agg[0]:c->agg[1];
          agg = _binOp.combine(other, agg);
        }
      }
      c = next;
    }
    _frontSum = agg;
  }

  void rebuildBack() {
    aggT agg = _identE;
    auto it = _tails.rbegin();

    while (++it != _tails.rend()) {
      Node *c = *it;
      aggT nodeAgg = c->full()?_binOp.combine(c->agg[0], c->agg[1]):c->agg[0];
      agg = _binOp.combine(agg, nodeAgg);
    }
    _backSum = agg;
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

  outT query() const { return _binOp.combine(_frontSum, _backSum); }

  size_t size() { return _size; }

  timeT youngest() const { throw 1; }

  void insert_lifted(timeT const& time, aggT const& liftedValue) {
    bool hasCarry = true;
    Node* carriedFrom = NULL;
    aggT carry = liftedValue;
    bool bigRootHit = false;
    _backSum = _binOp.combine(_backSum, liftedValue);
    for (auto it=_tails.begin(); it != _tails.end(); it++) {
      Node *node = *it;
      if (node->arity == 2) {
        auto nextCarry = _binOp.combine(node->agg[0], node->agg[1]);
        *it = new Node(carry, carriedFrom); // with just carry
        carriedFrom = node, carry = nextCarry;
      } else { // found a non-full node
        node->push_back(carry, carriedFrom);
        hasCarry = false;
        if (it + 1 == _tails.end())
          bigRootHit = true;
        break;
      }
    }
    if (hasCarry)
      _tails.push_back(new Node(carry, carriedFrom));
    else if (bigRootHit) {
      rebuildFront(), rebuildBack();
    }
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
  }
};

template <typename timeT, class BinaryFunction, class T>
Aggregate<timeT, BinaryFunction>
make_aggregate(BinaryFunction f, T elem) {
    return Aggregate<timeT, BinaryFunction>(f, elem);
}

template <typename BinaryFunction, typename timeT>
struct MakeAggregate {
  template <typename T>
  Aggregate<timeT, BinaryFunction> operator()(T elem) {
    BinaryFunction f;
    return make_aggregate<timeT, BinaryFunction>(f, elem);
  }
};
} // namespace amta
