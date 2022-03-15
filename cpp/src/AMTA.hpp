#pragma once

#include <cassert>
#include <deque>
#include <iostream>
#include <ostream>
#include <vector>
#include <iterator>

/* This file contains an implementation of our interpretation of the Amortized
 * MTA data structure from the following paper:
 *
 *   Álvaro Villalba, Josep Lluís Berral, and David Carrera
 *
 *   Constant-Time Sliding Window Framework with Reduced Memory Footprint and
 *   Efficient Bulk Evictions
 *
 *   IEEE Transactions on Parallel and Distributed Systems
 *   (Volume: 30, Issue: 3, March 1 2019)
 *
 *   https://doi.org/10.1109/TPDS.2018.2868960
 * */
namespace amta {

using namespace std;

constexpr bool PRINT_DEBUG = false;
template <typename _timeT, typename binOpFunc>
class Aggregate {
public:
  typedef typename binOpFunc::In inT;
  typedef typename binOpFunc::Partial aggT;
  typedef typename binOpFunc::Out outT;
  typedef _timeT timeT;

private:
  /* Node is an internal binary-tree node structure and as such, a node
   * can have at most two children.
   *
   *     o The "arity" field can be 0, 1, 2, -1, indicating how many children it
   *     has, where -1 means one child on the right (left has been popped).
   *
   *     o For i = 0, 1, agg[i] stores the aggregated value of the subtree
   *     pointed to by children[i]--and times[i] is the largest timestamp in
   *     that subtree.
   * */
  struct Node {
    aggT agg[2];
    timeT times[2];
    Node* children[2];
    Node* parent;
    int arity;
    Node(aggT a, timeT t, Node* left) { init(a, t, left); }
    void init(aggT a, timeT t, Node *left) {
      parent = NULL, arity = 0;
      push_back(a, t, left);
    }
    void push_back(aggT a, timeT t, Node *child) {
      assert (arity == 0 || arity == 1);
      agg[arity] = a; times[arity] = t; children[arity] = child; arity++;
      if (child != NULL) child->parent = this;
    }
    Node* pop_front() {
      assert(arity != 0);
      if (abs(arity) == 1) {
        arity = 0;
        return this->children[1];
      } else {
        arity = -1;
        return this->children[0];
      }
    }
    bool leftPopped() { return arity == -1; }
    bool rightEmpty() { return arity == 1; }
    bool full() { return arity == 2; }
    bool isLeaf() { return arity == 0; }

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
  vector<Node *> _freeList;

public:
  Aggregate(binOpFunc binOp, aggT identE_)
      : _binOp(binOp), _size(0), _tails(), _frontNode(NULL),
        _frontStack(), _identE(identE_), _frontSum(identE_), _backSum(identE_), _freeList() {
  }

  ~Aggregate() {
    while (!_freeList.empty())
      _compactFreeList();
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
      _deleteNode(c);
      c = next;
    }
    if (c == NULL) { // hit the big root
      if (PRINT_DEBUG) std::cout << "big root emptied, moving on..." << std::endl;
      _tails.pop_back();
      _frontNode = NULL;
      rebuildFront(), rebuildBack();
    }
    else
      rebuildFrontFrom(c);
  }

  void _slice(Node *node, timeT const& time) {
    while (node != NULL) {
      if (!node->leftPopped()) {
        if (time < node->times[0]) {
          node = node->children[0];
          continue;
        }
        else if (time == node->times[0]) {
          _deleteNode(node->pop_front());
          break;
        }
      }
      if (!node->rightEmpty() && time < node->times[1]) {
          if (!node->leftPopped())
            _deleteNode(node->pop_front());
          node = node->children[1];
          continue;
      }
      // should never reach this case
      throw 1;
    }
  }

  void _emptyOutNode(Node *node) {
    if (node != NULL && !node->isLeaf()) {
      Node *c = NULL;
      if (!node->leftPopped() && (c = node->children[0]) != NULL)
        _freeList.push_back(c);
      if (!node->rightEmpty() && (c = node->children[1]) != NULL)
        _freeList.push_back(c);
    }
  }

  Node* _newNode(aggT a, timeT t, Node* left) {
    if (_freeList.empty())
      return new Node(a, t, left);
    Node* node = _freeList.back();
    _freeList.pop_back();
    _emptyOutNode(node);
    node->init(a, t, left);
    return node;
  }

  void _deleteNode(Node* node, bool recursive=true) {
    if (node == NULL)
      return ; // nothing to delete
    if (recursive) _emptyOutNode(node);
    delete node;
  }

  void _compactFreeList() {
    Node* node = _freeList.back();
    _freeList.pop_back();
    _emptyOutNode(node);
    delete node;
  }

  void bulkEvict(timeT const& time) {
    if (_tails.empty() || time < oldest()) return ; // nothing to evict

    _size = -1; // size tracking will stop working from this point on
    while (!_tails.empty()) {
      Node *head = _tails.back();
      timeT mostRecent = head->rightEmpty() ? head->times[0] : head->times[1];
      if (PRINT_DEBUG) std::cout << "considering --" << *head << std::endl;
      if (PRINT_DEBUG) std::cout << "mostRecent=" << mostRecent << std::endl;
      if (time < mostRecent) {
        if (PRINT_DEBUG) std::cout << "Slicing the head node" << std::endl;
        // stop at this root, but slice it before we quit
        if (head->full()) {
          if (PRINT_DEBUG) std::cout << "head is full" << std::endl;
          if (time >= head->times[0]) {
            // delete the left subtree completely & slice the right tree
            if (PRINT_DEBUG) std::cout << "== delete left; slice right" << std::endl;
            _deleteNode(head->pop_front());
            _slice(head->children[1], time);
          } else {
            if (PRINT_DEBUG) std::cout << "== slice left" << std::endl;
             // slice the left subtree but leave the right tree alone
            _slice(head->children[0], time);
          }
        } else {
          int indicator = head->rightEmpty()?0:1;
          if (PRINT_DEBUG) {
            std::cout << "## not full, slicing indictor=" << indicator
                      << std::endl;
          }
          _slice(head->children[indicator], time); // slice the only subtree
        }

        // done, no more eviction this time
        break;
      }
      if (PRINT_DEBUG) std::cout << "Evicting whole head" << std::endl;
      // evict this whole root, plus perhaps some more
      _deleteNode(head); _tails.pop_back();

      // shortcut the case where this head ends with time
      if (mostRecent == time) break;
    }
    rebuildBack(), rebuildFront();

    if (PRINT_DEBUG) this->print();
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
        *it = _newNode(carry, carryTime, carriedFrom); // with just carry
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
      Node *n = _newNode(carry, carryTime, carriedFrom);
      if (_tails.empty())
        _frontNode = n;
      _tails.push_back(n);
    } else if (bigRootHit) {
      rebuildFront(), rebuildBack();
    }
  }

  void insert(timeT const& time, inT const& value) {
    insert_lifted(time, _binOp.lift(value));
    if (_size >= 0) _size++;
  }

  void insert(inT const& val) {
    if (_tails.empty()) {
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
