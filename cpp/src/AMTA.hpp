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
    timeT times[2];
    Node* children[2];
    Node* parent;
    int arity;
    Node(aggT a, timeT t, Node* left) : parent(NULL), arity(0) { push_back(a, t, left); }
    void push_back(aggT a, timeT t, Node* child) {
      assert (arity == 0 || arity == 1);
      agg[arity] = a; times[arity] = t; children[arity] = child; arity++;
      if (child != NULL) child->parent = this;
    }
    void pop_front() {
      assert(arity != 0);
      if (abs(arity) == 1)
        arity = 0;
      else
        arity = -1;
    }
    bool leftPopped() { return arity == -1; }
    bool rightEmpty() { return arity == 1; }
    bool full() { return arity == 2; }

    ostream& printNode(ostream& os) const {
      os << "[";
      for (int i = 0; i < arity; i++)
        os << this->times[i] << "/" << this->agg[i] << ",";
      if (arity == -1)
        os << "-, " << this->times[1] << "/"  << this->agg[1];
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
  Node* _frontNode; // ptr to the node storing the oldest elt
  deque<aggT> _frontStack;
  aggT _identE, _frontSum, _backSum;

public:
  Aggregate(binOpFunc binOp, aggT identE_)
      : _binOp(binOp), _size(0), _tails(), _frontNode(NULL),
        _frontStack(), _identE(identE_), _frontSum(identE_), _backSum(identE_) {
  }

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
    std::cout << "_frontStack:";
    for (auto elt: _frontStack) { std::cout << elt << ", "; }
    std::cout << std::endl;
    std::cout << "_frontNode:" << *_frontNode << std::endl;
  }
  void rebuildFrontFrom(Node *c) {
    aggT agg = _frontStack.empty()?_identE:_frontStack.back();
    while (c != NULL) {
      Node *next = c->leftPopped() ? c->children[1] : c->children[0];
      if (c->full()) {
        agg = _binOp.combine(c->agg[1], agg);
        _frontStack.push_back(agg);
      }
      if (next == NULL) // reset the frontNode pointer
        _frontNode = c;
      c = next;
    }
  }
  void rebuildFront() {
    if (_tails.empty()) { _frontSum = _identE; return ;}
    _frontStack.clear();

    rebuildFrontFrom(_tails.back());
    aggT agg = _frontStack.empty() ? _identE : _frontStack.back();
    aggT other = (_frontNode->full() || _frontNode->rightEmpty())
                     ? _frontNode->agg[0]
                     : _frontNode->agg[1];
    _frontSum = _binOp.combine(other, agg);
  }

  void rebuildBack() {
    if (_tails.empty()) { _backSum = _identE; return ;}
    aggT agg = _identE;
    auto it = _tails.rbegin();

    while (++it != _tails.rend()) {
      Node *c = *it;
      aggT nodeAgg =
          c->full() ? _binOp.combine(c->agg[0], c->agg[1]) : c->agg[0];
      agg = _binOp.combine(agg, nodeAgg);
    }
    _backSum = agg;
  }

  void evict() {
    _frontSum = _frontStack.empty() ? _identE : _frontStack.back();
    if (_size > 0) _size--;
    Node *c = _frontNode;
    while (c != NULL) {
      if (c->full()) _frontStack.pop_back(); // was a full node, no longer
      c->pop_front();
      if (c->arity != 0)
        break;
      Node *next = c->parent;
      delete c;
      c = next;
    }
    if (c == NULL) { // hit the big root
//      std::cout << "big root emptied, moving on..." << std::endl;
      _tails.pop_back();
      _frontNode = NULL;
      rebuildFront(), rebuildBack();
    }
    else
      rebuildFrontFrom(c);
  }
  void bulkEvict() {
    throw 1;
  }
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

  timeT oldest() const {
    return _frontNode->leftPopped() ? _frontNode->times[1]
                                   : _frontNode->times[0];
  }

  outT query() const {
    return _binOp.lower(_binOp.combine(_frontSum, _backSum));
  }

  size_t size() { return _size; }

  timeT youngest() const {
    Node *backNode = _tails.front();
    return (backNode->full() || backNode->leftPopped())
               ? backNode->times[1]
               : backNode->times[0];
  }

  void insert_lifted(timeT const& time, aggT const& liftedValue) {
    bool hasCarry = true;
    Node *carriedFrom = NULL;
    aggT carry = liftedValue;
    timeT carryTime = time;
    bool bigRootHit = false;
    _backSum = _binOp.combine(_backSum, liftedValue);
    for (auto it = _tails.begin(); it != _tails.end(); it++) {
      Node *node = *it;
      if (node->full() || node->leftPopped()) { // has room for carry
        auto nextCarry = node->full()
                             ? _binOp.combine(node->agg[0], node->agg[1])
                             : node->agg[1];
        auto nextTime = node->times[1];
        *it = new Node(carry, carryTime, carriedFrom); // with just carry
        carriedFrom = node, carry = nextCarry, carryTime = nextTime;
      } else { // found a non-full node
        node->push_back(carry, carryTime, carriedFrom);
        hasCarry = false;
        if (it + 1 == _tails.end())
          bigRootHit = true;
        break;
      }
    }
    if (hasCarry) {
      Node *n = new Node(carry, carryTime, carriedFrom);
      if (_tails.empty())
        _frontNode = n;
      _tails.push_back(n);
    } else if (bigRootHit) {
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
    _size++;
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
