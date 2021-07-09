#ifndef __FIBA_H__
#define __FIBA_H__

#include <iostream>
#include <cassert>
#include <cmath>
#include <limits>
#include <ostream>
#include <sstream>
#include <vector>
#include <utility>
#include <tuple>
#include <deque>

#include "utils.h"

namespace btree {

using namespace std;

#ifdef COLLECT_STATS
#define IF_COLLECT_STATS(x) x
#else
#define IF_COLLECT_STATS(x) ;
#endif

IF_COLLECT_STATS(static long statsCombineCount = 0);
IF_COLLECT_STATS(static long statsTotalRepairAggCount = 0);
IF_COLLECT_STATS(static long statsTotalSpineRepairCount = 0);
IF_COLLECT_STATS(static long statsTotalNumNodesRepaired = 0);
IF_COLLECT_STATS(static long statsSumRootDegree = 0);

enum Kind { classic, knuckle, finger };

template<typename _timeT, int minArity, Kind kind, typename binOpFunc,
         bool early_stopping=false>
class Aggregate {
public:
  typedef typename binOpFunc::In inT;
  typedef typename binOpFunc::Partial aggT;
  typedef typename binOpFunc::Out outT;
  typedef _timeT timeT;

private:
  static int const maxArity = 2 * minArity;

  class Node {
    typedef Node* NodeP;
    NodeP _parent;
    aggT _agg;
    int _arity;
    bool _leftSpine, _rightSpine;
    timeT _times[maxArity];
    aggT _values[maxArity];
    NodeP* _children;

    aggT recalcAgg(binOpFunc const &op) const {
      if (kind==finger) {
        if (isRoot() || (_leftSpine && _rightSpine))
          return recalcAggInner(op);
        if (_leftSpine)
          return recalcAggLeft(op);
        if (_rightSpine)
          return recalcAggRight(op);
      }
      assert(hasAggUp());
      return recalcAggUp(op);
    }

    aggT recalcAggInner(binOpFunc const &op) const {
      aggT result = binOpFunc::identity;
      if (isLeaf()) {
        for (int i=0, n=_arity-1; i<n; i++) {
          result = op.combine(result, getValue(i));
          IF_COLLECT_STATS(++statsCombineCount);
        }
      } else {
        if (_arity > 1) {
          result = op.combine(result, getValue(0));
          IF_COLLECT_STATS(++statsCombineCount);
        }
        for (int i=1, n=_arity-1; i<n; i++) {
          result = op.combine(result, getChild(i)->getAgg());
          IF_COLLECT_STATS(++statsCombineCount);
          result = op.combine(result, getValue(i));
          IF_COLLECT_STATS(++statsCombineCount);
        }
      }
      return result;
    }

    aggT recalcAggLeft(binOpFunc const &op) const {
      aggT result = recalcAggInner(op);
      if (!isLeaf()){
        result = op.combine(result, getChild(_arity-1)->getAgg());
        IF_COLLECT_STATS(++statsCombineCount);
      }
      if (!isRoot() && !_parent->isRoot()) {
        result = op.combine(result, _parent->getAgg());
        IF_COLLECT_STATS(++statsCombineCount);
      }
      return result;
    }

    aggT recalcAggRight(binOpFunc const &op) const {
      aggT result = recalcAggInner(op);
      if (!isLeaf()) {
        result = op.combine(getChild(0)->getAgg(), result);
        IF_COLLECT_STATS(++statsCombineCount);
      }
      if (!isRoot() && !_parent->isRoot()) {
        result = op.combine(_parent->getAgg(), result);
        IF_COLLECT_STATS(++statsCombineCount);
      }
      return result;
    }

    aggT recalcAggUp(binOpFunc const &op) const {
      if (isLeaf())
        return recalcAggInner(op);
      if (_arity == 1)
        return getChild(0)->getAgg();
      aggT leftAgg = getChild(0)->getAgg();
      aggT innerAgg = recalcAggInner(op);
      aggT rightAgg = getChild(_arity - 1)->getAgg();
      IF_COLLECT_STATS(++statsCombineCount);
      IF_COLLECT_STATS(++statsCombineCount);
      return op.combine(leftAgg, op.combine(innerAgg, rightAgg));
    }

    bool recalcLeftSpine() const {
      if (isRoot())
        return false;
      if (!(_parent->isRoot() || _parent->leftSpine()))
        return false;
      return this == _parent->getChild(0);
    }

    bool recalcRightSpine() const {
      if (isRoot())
        return false;
      if (!(_parent->isRoot() || _parent->rightSpine()))
        return false;
      return this == _parent->getChild(_parent->arity()-1);
    }

    void setChild(int i, NodeP node) {
      assert(0 <= i && i < _arity);
      _children[i] = node;
    }

  public:
    Node(bool isLeaf)
      : _parent(NULL),
        _agg(binOpFunc::identity),
        _arity(1),
        _leftSpine(false), _rightSpine(false),
        _children(isLeaf ? NULL : new NodeP[maxArity + 1])
    { }

    ~Node() { delete[] _children; }

    int arity() const { return _arity; }

    void becomeRoot(binOpFunc const &op) {
      assert(!isRoot() && _parent->isRoot() && _parent->arity()==1);
      _parent = NULL;
      _leftSpine = false;
      _rightSpine = false;
      if (kind==finger)
        localRepairAgg(op);
    }

    int childIndex() const {
      assert(!isRoot());
      for (int i=0, n=_parent->arity(); i<n; i++)
        if (this == _parent->getChild(i))
          return i;
      assert(false);
      return -1;
    }

    aggT getAgg() const { return _agg; }

    NodeP getChild(int i) const {
      assert(!isLeaf() && 0 <= i && i < _arity);
      return _children[i];
    }

    timeT getTime(int i) const {
      assert(0 <= i && i < _arity - 1);
      return _times[i];
    }

    aggT getValue(int i) const {
      assert(0 <= i && i < _arity - 1);
      return _values[i];
    }

    bool hasAggUp() const {
      if (kind==finger)
        return !(isRoot() || _leftSpine || _rightSpine);
      else
        return true;
    }

    int height() const {
      int result = 0;
      Node const* descendent = this;
      while (!descendent->isLeaf()) {
        descendent = descendent->getChild(0);
        result++;
      }
      return result;
    }

    bool isDescendent(Node* node) const {
      Node const* ancestor = this;
      while (ancestor != NULL) {
        if (ancestor == node)
          return true;
        ancestor = ancestor->parent();
      }
      return false;
    }

    bool isLeaf() const { return NULL == _children; }

    bool isRoot() const { return NULL == _parent; }

    bool leftSpine() const { return _leftSpine; }

    bool localCheckInvariant(binOpFunc const &op, bool strong,
                             const char* f, int l) const {
      bool ok = true;
      ok &= (0 <= _arity);
      if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
      if (strong) {
        ok &= (isRoot() || minArity <= _arity) && (_arity <= maxArity);
        if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
      } else {
        ok &= (isRoot() || minArity - 1 <= _arity) && (_arity <= maxArity + 1);
        if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
      }
      if (isLeaf()) {
        for (int i=0, n=_arity-2; i<n; i++) {
          ok &= (getTime(i) < getTime(i+1));
          if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
        }
      } else {
        for (int i=0, n=_arity-1; i<n; i++) {
          Node* leftNode = getChild(i);
          if (leftNode->arity() >= 2) {
            timeT leftTime = leftNode->getTime(leftNode->arity() - 2);
            ok &= (leftTime < getTime(i));
            if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
          }
          Node* rightNode = getChild(i + 1);
          if (rightNode->arity() >= 2) {
            timeT rightTime = rightNode->getTime(0);
            ok &= getTime(i) < rightTime;
            if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
          }
        }
        for (int i=0, n=_arity; i<n; i++) {
          ok &= (getChild(i)->parent() == this);
          if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
        }
      }
      ok &= (_leftSpine == recalcLeftSpine());
      if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
      ok &= (_rightSpine == recalcRightSpine());
      if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
      if (strong) {
        ok &= (_agg == recalcAgg(op));
        if(!ok){cerr<<f<<":"<<l<<": FAILED"<<endl<<*this; throw 0;}
      }
      return ok;
    }

    void localEvict(binOpFunc const &op, int index) {
      assert(!isLeaf() && 1 < _arity && index < _arity - 1);
      for (int i=index, n=_arity-2; i<n; i++) {
        setEntry(i, getTime(i + 1), getValue(i + 1));
        setChild(i + 1, getChild(i + 2));
      }
      if (index == _arity-2 && (isRoot() || _rightSpine))
        getChild(_arity-2)->_rightSpine = true;
      _arity--;
      localRepairAggIfUp(op);
    }

    void localEvictEntry(binOpFunc const &op, timeT time) {
      int index;
      bool found = localSearch(time, index);
      assert(found);
      for (int i=index, n=_arity-2; i<n; i++)
        setEntry(i, getTime(i + 1), getValue(i + 1));
      _arity--;
      localRepairAggIfUp(op);
    }

    void localInsert(binOpFunc const &op, timeT time, aggT value, NodeP node, bool repairs=true) {
      assert(!isLeaf() && _arity <= maxArity);
      int i = _arity - 1;
      if (i==0 || time>getTime(i-1)) {
        pushBack(op, time, value, node);
      } else {
        _arity++;
        while (i>0 && time<getTime(i-1)) {
          setEntry(i, getTime(i-1), getValue(i-1));
          setChild(i+1, getChild(i));
          i--;
        }
        setEntry(i, time, value);
        setChild(i + 1, node);
        node->_parent = this;
        if (repairs) localRepairAggIfUp(op);
      }
    }

    bool localInsertEntry(binOpFunc const &op, timeT time, aggT value,bool repairs=true) {
      int index;
      bool found = localSearch(time, index);
      assert((found || isLeaf()) && _arity <= maxArity);
      if (found) {
        setEntry(index, time, op.combine(getValue(index), value));
        if (repairs) localRepairAggIfUp(op);
      } else {
        if (_arity == 0 || index == _arity - 1) {
          pushBackEntry(op, time, value);
        } else {
          _arity++;
          for (int i=_arity-2; i>index; i--)
            setEntry(i, getTime(i - 1), getValue(i - 1));
          setEntry(index, time, value);
          if (repairs) localRepairAggIfUp(op);
        }
      }
      return !found;
    }

    void localRepairAggIfUp(binOpFunc const &op) {
      if (hasAggUp())
        localRepairAgg(op);
    }

    bool localRepairAgg(binOpFunc const &op) {
      IF_COLLECT_STATS(statsTotalNumNodesRepaired++);
      aggT const newAgg = recalcAgg(op);
      if (_agg != newAgg) { //careful: '!=' and '==' asymmetric for geomean
        _agg = newAgg;
        return true;
      }
      return false;
    }

    bool localSearch(timeT const& time, int& index) {
      int n;
      for (index=0, n=_arity-1; index<n; index++) {
        if (time < getTime(index))
          return false;
        else if (time == getTime(index))
          return true;
      }
      assert(index == _arity - 1);
      return false;
    }

    NodeP parent() const { return _parent; }
    void setParent(Node* p) { _parent = p; }

    void popBack(binOpFunc const &op, int howMany) {
      assert(howMany < _arity);
      _arity -= howMany;
      if (!isLeaf() && (isRoot() || _rightSpine))
        getChild(_arity - 1)->_rightSpine = true;
      localRepairAggIfUp(op);
    }

    void popFront(binOpFunc const &op, int howMany) {
      assert(howMany < _arity);
      for (int i=0, n=_arity-1-howMany; i<n; i++)
        setEntry(i, getTime(i + howMany), getValue(i + howMany));
      if (!isLeaf())
        for (int i=0, n=_arity-howMany; i<n; i++)
          setChild(i, getChild(i + howMany));
      _arity -= howMany;
      if (!isLeaf() && (isRoot() || _leftSpine))
        getChild(0)->_leftSpine = true;
      localRepairAggIfUp(op);
    }

    ostream& print(ostream& os, int const indent) const {
      /*
       * Note: commented out so Scott can compile and test
       * locally.
       */
      for (int c=0; c<indent; c++) os << "  ";
      if (isLeaf()) {
        os << "(";
        for (int i=0, n=_arity-1; i<n; i++) {
          if (i > 0)
            os << ' ';
          os << getTime(i) << "/" << getValue(i);
        }
      } else {
        os << "(" << endl;
        for (int i=0, n=_arity-1; i<n; i++) {
          getChild(i)->print(os, indent+1);
          for (int c=0; c<indent; c++) os << "  ";
          os << " " << getTime(i) << "/" << getValue(i) << endl;
        }
        getChild(_arity-1)->print(os, indent+1);
        for (int c=0; c<indent; c++) os << "  ";
      }
      os << ")/ " << _agg;
      if (isRoot()) os << " root";
      if (isLeaf()) os << " leaf";
      if (kind==finger && _leftSpine) os << " left-spine";
      if (kind==finger && _rightSpine) os << " right-spine";
      os << endl;
      /* */
      return os;
    }

    string repr() {
      ostringstream stream;
      stream << "<";
      for (int i=0;i<this->arity()-1;++i) {
        if (i>0) stream << ", ";
        stream << this->getTime(i);
      }
      stream << ">";
      stream << "(";
      if (isRoot()) stream << " root";
      if (isLeaf()) stream << " leaf";
      if (kind==finger && _leftSpine) stream << " left-spine";
      if (kind==finger && _rightSpine) stream << " right-spine";
      stream << ")";
      return stream.str();
    }

    void pushBack(binOpFunc const &op, timeT time, aggT value, NodeP node, bool eagerAgg=true) {
      assert(!isLeaf() && 0 < _arity && _arity <= maxArity);
      _arity++;
      setEntry(_arity-2, time, value);
      setChild(_arity-1, node);
      if (isRoot() || _rightSpine) {
        getChild(_arity-2)->_rightSpine = false;
        node->_rightSpine = true;
      }
      if (eagerAgg && hasAggUp()) {
        _agg = op.combine(_agg, value);
        IF_COLLECT_STATS(++statsCombineCount);
        _agg = op.combine(_agg, node->getAgg());
        IF_COLLECT_STATS(++statsCombineCount);
      }
      node->_parent = this;
    }

    void pushBackEntry(binOpFunc const &op, timeT time, aggT value, bool eagerAgg=true) {
      assert(isLeaf() && _arity <= maxArity);
      _arity++;
      setEntry(_arity - 2, time, value);
      if (eagerAgg && hasAggUp()) {
        _agg = op.combine(_agg, value);
        IF_COLLECT_STATS(++statsCombineCount);
      }
    }

    void pushFront(binOpFunc const& op, NodeP node, timeT time, aggT value) {
      assert(!isLeaf() && 0 < _arity && _arity <= maxArity);
      _arity++;
      for (int i=_arity-2; i>=1; i--)
        setEntry(i, getTime(i - 1), getValue(i - 1));
      for (int i=_arity-1; i>=1; i--)
        setChild(i, getChild(i - 1));
      setEntry(0, time, value);
      setChild(0, node);
      if (isRoot() || _leftSpine) {
        getChild(1)->_leftSpine = false;
        node->_leftSpine = true;
      }
      if (hasAggUp()) {
        _agg = op.combine(value, _agg);
        IF_COLLECT_STATS(++statsCombineCount);
        _agg = op.combine(node->getAgg(), _agg);
        IF_COLLECT_STATS(++statsCombineCount);
      }
      node->_parent = this;
    }

    void pushFrontEntry(binOpFunc const &op, timeT time, aggT value) {
      assert(isLeaf() && _arity <= maxArity);
      _arity++;
      for (int i=_arity-2; i>=1; i--)
        setEntry(i, getTime(i-1), getValue(i-1));
      setEntry(0, time, value);
      if (hasAggUp()) {
        _agg = op.combine(value, _agg);
        IF_COLLECT_STATS(++statsCombineCount);
      }
    }

    bool rightSpine() const { return _rightSpine; }
    void setRightSpine(bool flag) { _rightSpine = flag; }

    void clear() { _arity = 1; _agg = binOpFunc::identity; }

    void setEntry(int i, timeT time, aggT value) {
      assert(0 <= i && i < _arity - 1);
      _times[i] = time;
      _values[i] = value;
    }

    void setOnlyChild(NodeP child) {
      assert(!isLeaf() && _arity==1);
      _arity = 1;
      setChild(0, child);
      child->_parent = this;
      child->_leftSpine = isRoot() || _leftSpine;
      child->_rightSpine = isRoot() || _rightSpine;
      if (hasAggUp())
        _agg = child->getAgg();
    }

    friend inline std::ostream& operator<<(std::ostream& os, Node const& x) {
      return x.print(os, 0);
    }
  };

  #define FAKE_NODE ((Node *)0x1L)
   // (dynamic_cast<Node*>(0x1))
  binOpFunc _binOp;
  Node *_root;
  Node *_leftFinger, *_rightFinger;
  size_t _size;

  void heightDecrease() {
    if (false) cout << "-- height-decrease" << endl;
    assert(_root->arity() == 1);
    Node* oldRoot = _root;
    _root = oldRoot->getChild(0);
    _root->becomeRoot(_binOp);
    delete oldRoot;
    assert(checkInvariant(__FILE__, __LINE__, _root));
  }

  void heightIncrease(bool check=true) {
    if (true) cout << "-- height-increase" << endl;
    Node* oldRoot = _root;
    _root = new Node(false);
    _root->setOnlyChild(oldRoot);
    if (check) assert(checkInvariant(__FILE__, __LINE__, oldRoot));
  }

  Node* leastCommonAncestor(Node* node1, Node* node2) const {
    int height1 = node1->height(), height2 = node2->height();
    while (height1 < height2) {
      node1 = node1->parent();
      height1++;
    }
    while (height1 > height2) {
      node2 = node2->parent();
      height2++;
    }
    assert(height1 == height2);
    while (node1 != node2) {
      node1 = node1->parent();
      node2 = node2->parent();
    }
    assert(node1 == node2 && node1 != NULL);
    return node1;
  }

  Node* merge(Node* parent, int nodeIdx, int siblingIdx) {
    if (false) cout << "-- merge" << endl;
    int betweenIndex = (nodeIdx < siblingIdx) ? nodeIdx : siblingIdx;
    timeT betweenTime = parent->getTime(betweenIndex);
    aggT betweenValue = parent->getValue(betweenIndex);
    Node* left = parent->getChild(betweenIndex);
    Node* right = parent->getChild(betweenIndex + 1);
    if (left->isLeaf()) {
      left->pushBackEntry(_binOp, betweenTime, betweenValue);
      for (int i=0, n=right->arity()-1; i<n; i++)
        left->pushBackEntry(_binOp, right->getTime(i), right->getValue(i));
    } else {
      left->pushBack(_binOp, betweenTime, betweenValue, right->getChild(0));
      for (int i=0, n=right->arity()-1; i<n; i++)
        left->pushBack(_binOp, right->getTime(i), right->getValue(i),
                       right->getChild(i + 1));
    }
    parent->localEvict(_binOp, betweenIndex);
    if (kind!=classic && _rightFinger == right)
      _rightFinger = left;
    delete right;
    assert(checkInvariant(__FILE__, __LINE__, left));
    return left;
  }

  void move(Node* parent, int recipientIdx, int giverIdx) {
    if (false) cout << "-- move" << endl;
    Node* recipient = parent->getChild(recipientIdx);
    Node* giver = parent->getChild(giverIdx);
    int betweenIdx = (recipientIdx < giverIdx) ? recipientIdx : giverIdx;
    timeT betweenTime = parent->getTime(betweenIdx);
    aggT betweenValue = parent->getValue(betweenIdx);
    if (recipientIdx < giverIdx) {
      timeT stolenTime = giver->getTime(0);
      aggT stolenValue = giver->getValue(0);
      parent->setEntry(betweenIdx, stolenTime, stolenValue);
      if (recipient->isLeaf()) {
        recipient->pushBackEntry(_binOp, betweenTime, betweenValue);
      } else {
        Node* stolenNode = giver->getChild(0);
        recipient->pushBack(_binOp, betweenTime, betweenValue, stolenNode);
      }
      giver->popFront(_binOp, 1);
    } else {
      timeT stolenTime = giver->getTime(giver->arity() - 2);
      aggT stolenValue = giver->getValue(giver->arity() - 2);
      parent->setEntry(betweenIdx, stolenTime, stolenValue);
      if (recipient->isLeaf()) {
        recipient->pushFrontEntry(_binOp, betweenTime, betweenValue);
      } else {
        Node* stolenNode = giver->getChild(giver->arity() - 1);
        recipient->pushFront(_binOp, stolenNode, betweenTime, betweenValue);
      }
      giver->popBack(_binOp, 1);
    }
    parent->localRepairAggIfUp(_binOp);
    assert(checkInvariant(__FILE__, __LINE__, recipient, giver));
  }

  Node* pickEvictionSibling(Node* node, int& nodeIdx, int& siblingIdx) const {
    Node* parent = node->parent();
    assert(NULL != parent && 1 < parent->arity());
    nodeIdx = node->childIndex();
    if (nodeIdx == parent->arity() - 1)
      siblingIdx = nodeIdx - 1;
    else
      siblingIdx = nodeIdx + 1;
    return parent->getChild(siblingIdx);
  }

  aggT rangeQueryRec(Node const& node, timeT tFrom, timeT tTo) const {
    timeT const TMIN = std::numeric_limits<timeT>::min();
    timeT const TMAX = std::numeric_limits<timeT>::max();
    if (tFrom == TMIN && tTo == TMAX && node.hasAggUp())
      return node.getAgg();
    aggT res = binOpFunc::identity;
    if (!node.isLeaf()) {
      timeT const tNext = node.getTime(0);
      if (tFrom < tNext) {
        res = _binOp.combine(res, rangeQueryRec(*node.getChild(0),
                                                tFrom,
                                                tNext<=tTo ? TMAX : tTo));
        IF_COLLECT_STATS(++statsCombineCount);
      }
    }
    for (int i=0, n=node.arity()-1; i<n; i++) {
      timeT tCurr = node.getTime(i);
      if (tFrom <= tCurr && tCurr <= tTo) {
        res = _binOp.combine(res, node.getValue(i));
        IF_COLLECT_STATS(++statsCombineCount);
      }
      if (!node.isLeaf() && i + 1 < n) {
        timeT tNext = node.getTime(i + 1);
        if (tCurr < tTo && tFrom < tNext) {
          res = _binOp.combine(res, rangeQueryRec(*node.getChild(i + 1),
                                                  tFrom<=tCurr ? TMIN : tFrom,
                                                  tNext<=tTo ? TMAX : tTo));
          IF_COLLECT_STATS(++statsCombineCount);
        }
      }
    }
    assert(node.isLeaf() || node.arity() > 1);
    if (!node.isLeaf()) {
      timeT tCurr = node.getTime(node.arity() - 2);
      if (tCurr < tTo) {
        res = _binOp.combine(res, rangeQueryRec(*node.getChild(node.arity()-1),
                                                tFrom<=tCurr ? TMIN : tFrom,
                                                tTo));
        IF_COLLECT_STATS(++statsCombineCount);
      }
    }
    return res;
  }

  Node* rebalanceAfterEvict(Node* node, bool* hitLeft, bool* hitRight, Node* toRepair=NULL) {
    *hitLeft = node->leftSpine();
    *hitRight = node->rightSpine();
    if (node == toRepair)
      node->localRepairAggIfUp(_binOp);
    while (!node->isRoot() && node->arity() < minArity) {
      Node* parent = node->parent();
      int nodeIndex, siblingIndex;
      Node* sibling = pickEvictionSibling(node, nodeIndex, siblingIndex);
      *hitRight |= sibling->rightSpine();
      if (sibling->arity() <= minArity) {
        node = merge(parent, nodeIndex, siblingIndex);
        if (parent->isRoot() && parent->arity() == 1)
          heightDecrease();
        else
          node = parent;
      } else {
        move(parent, nodeIndex, siblingIndex);
        node = parent;
      }
      if (node == toRepair)
	node->localRepairAggIfUp(_binOp);
      *hitLeft |= node->leftSpine();
      *hitRight |= node->rightSpine();
    }
    return node;
  }

  Node* rebalanceAfterInsert(Node* node, bool* hitLeft, bool* hitRight) {
    *hitLeft = node->leftSpine();
    *hitRight = node->rightSpine();
    while (node->arity() > maxArity) {
      if (node->isRoot()) {
        heightIncrease();
        *hitLeft = true;
        *hitRight = true;
      }
      split(node);
      node = node->parent();
      *hitLeft |= node->leftSpine();
      *hitRight |= node->rightSpine();
    }
    return node;
  }

  void repairAggs(Node* top, bool hitLeft, bool hitRight) {
    // STATS: total number of calls to repairAggs
    IF_COLLECT_STATS(statsTotalRepairAggCount++);
    IF_COLLECT_STATS(statsSumRootDegree += _root->arity());
    if (kind==finger) {
      if (!top->hasAggUp()) {
        top->localRepairAgg(_binOp);
      } else {
        while (top->hasAggUp()) {
          top = top->parent();
          if (!top->localRepairAgg(_binOp) && early_stopping) return;
        }
      }
      IF_COLLECT_STATS(bool spineRepairs = false;)
      if (top->leftSpine() || (hitLeft && top->isRoot())) {
        IF_COLLECT_STATS(spineRepairs = true;)
        Node* left = top;
        while (!left->isLeaf()) {
          left = left->getChild(0);
          left->localRepairAgg(_binOp);
        }
      }
      if (top->rightSpine() || (hitRight && top->isRoot())) {
        IF_COLLECT_STATS(spineRepairs = true;)
        Node* right = top;
        while (!right->isLeaf()) {
          right = right->getChild(right->arity() - 1);
          right->localRepairAgg(_binOp);
        }
      }
      IF_COLLECT_STATS(statsTotalSpineRepairCount += (int) spineRepairs;)
    } else {
      top = top->parent();
      while (NULL != top) {
        if (!top->localRepairAgg(_binOp) && early_stopping) return;
        top = top->parent();
      }
    }
  }

  Node* searchNode(timeT time) const {
    Node* node = _root;
    if (kind!=classic && !_root->isLeaf()) {
      if (time < _root->getTime(0)) {
        node = _leftFinger;
        while (!node->isRoot() && node->getTime(node->arity() - 2) < time)
          node = node->parent();
      } else if (_root->getTime(_root->arity() - 2) < time) {
        node = _rightFinger;
        while (!node->isRoot() && time < node->getTime(0))
          node = node->parent();
      }
    }
    while (!node->isLeaf()) {
      Node* child = NULL;
      for (int i=0, n=node->arity()-1; i<n; i++) {
        if (time < node->getTime(i)) {
          child = node->getChild(i);
          break;
        }
        if (time == node->getTime(i))
          return node;
      }
      if (child == NULL)
        child = node->getChild(node->arity() - 1);
      node = child;
    }
    return node;
  }

  Node* searchOldestNode() const {
    if (kind!=classic)
      return _leftFinger;
    Node* node = _root;
    while (!node->isLeaf())
      node = node->getChild(0);
    return node;
  }

  Node* searchYoungestNode() const {
    if (kind!=classic)
      return _rightFinger;
    Node* node = _root;
    while (!node->isLeaf())
      node = node->getChild(node->arity() - 1);
    return node;
  }

  void split(Node* left) {
    if (false) cout << "-- split" << endl;
    timeT betweenTime = left->getTime(minArity);
    aggT betweenValue = left->getValue(minArity);
    Node* right = new Node(left->isLeaf());
    left->parent()->localInsert(_binOp, betweenTime, betweenValue, right);
    if (left->isLeaf()) {
      for (int i=minArity+1,n=maxArity; i<n; i++)
        right->pushBackEntry(_binOp, left->getTime(i), left->getValue(i));
    } else {
      right->setOnlyChild(left->getChild(minArity + 1));
      for (int i=minArity+1,n=maxArity; i<n; i++)
        right->pushBack(_binOp, left->getTime(i), left->getValue(i),
                        left->getChild(i+1));
    }
    left->popBack(_binOp, minArity);
    if (kind!=classic && _rightFinger == left)
      _rightFinger = right;
    left->parent()->localRepairAggIfUp(_binOp);
    assert(checkInvariant(__FILE__, __LINE__, left, right));
  }


  /* Internal node "breadcrumb" - (Node *, lowerBound, upperBound)
   * the interval represented is an open-open interval (lowerBound, upperBound)
   */
  typedef tuple<Node *, timeT, timeT> ipathBreadcrumb;


  struct Treelet {
    timeT time;
    aggT value;
    Node *node;
    Node *rightChild;

    Treelet(Node* node_)
      : node(node_), rightChild(FAKE_NODE) {};

    Treelet(Node* node_, Node* rightChild_)
      : node(node_), rightChild(rightChild_) {};

    Treelet(timeT const& time_, aggT const& value_, Node *node_, Node* rightChild_)
      : time(time_), value(value_), node(node_), rightChild(rightChild_) {};
    friend ostream& operator<<(ostream& os, const Treelet& tl) {
      return os << "{"
                << "time=" << tl.time << ", "
                << "value=" << tl.value << ", "
                << "node=" << tl.node << ", "
                << "node=" << tl.rightChild << ", "
                << "}";
    }
    inline bool isReal() { return rightChild != FAKE_NODE; }
  };


  tuple<Node*, bool> scopedDescend(
      std::deque<ipathBreadcrumb>& latestPath,
      Node* node, timeT const& t, timeT lb, timeT ub
  ) {
    int index;
    cout << "down-- " << node->repr() << ", lb=" << lb
         << ", ub=" << ub << endl;
    for (;;) {
      bool found = node->localSearch(t, index);
      if (index > 0) lb = node->getTime(index - 1);
      if (index < node->arity()-1) ub = node->getTime(index);

      if (found)
        return std::make_tuple(node, true);

      if (node->isLeaf())
        break;

      node = node->getChild(index);
      latestPath.push_back(std::make_tuple(node, lb, ub));
      cout << "down-- " << node->repr() << ", lb=" << lb << ", ub=" << ub
           << endl;
    }
    return std::make_tuple(node, false);
  }

  tuple<Node*, bool> multiSearchFirst(
      std::deque<ipathBreadcrumb>& latestPath,
      timeT const& t) {

    Node* node = this->_root;
    timeT lb = std::numeric_limits<timeT>::min(),
          ub = std::numeric_limits<timeT>::max();

    cout << "multiSearchFirst(t=" << t << ")" << endl;

    if (!node->isLeaf()) { // proper finger search if root is not also a leaf
      node = this->_rightFinger;
      lb = node->getTime(0);

      cout << "up-- " << node->repr() << ", lb = " << lb << endl;
      while (!node->isRoot() && lb > t) {
        Node* p = node->parent();
        if (p == NULL) throw 1; // Parent of non-root should not be NULL

        node = p; lb = p->getTime(0);
        cout << "up-- " << node->repr() << ", lb = " << lb << endl;
      }
      if (node->isRoot()) lb = std::numeric_limits<timeT>::min();
    }
    cout << "apex-- " << node->repr() << endl;
    latestPath.push_back(std::make_tuple(node, lb, ub));

    return scopedDescend(latestPath, node, t, lb, ub);
  }

  tuple<Node*, bool> multiSearchNext(
      std::deque<ipathBreadcrumb>& latestPath,
      timeT const& t) {
    Node *node;
    timeT lb, ub;
    cout << "multiSearchNext(t=" << t << ")" << endl;
    std::tie(node, lb, ub) = latestPath.back();

    cout << "next: up-- " << node->repr() << ", lb = " << lb
         << ", ub = " << ub
         << endl;
    while (!(lb < t && t < ub) && !node->isRoot()) {
      latestPath.pop_back();
      std::tie(node, lb, ub) = latestPath.back();
      cout << "next: up-- " << node->repr() << ", lb = " << lb
           << ", ub = " << ub
           << endl;
    }

    cout << "apex-- " << node->repr() << ", lb = " << lb
         << ", ub = " << ub
         << endl;

    return scopedDescend(latestPath, node, t, lb, ub);
  }

  inline
  tuple<Node*, bool> multiSearchFind(
      std::deque<ipathBreadcrumb>& latestPath,
      timeT const& t) {
    if (latestPath.empty())
      return multiSearchFirst(latestPath, t);
    else
      return multiSearchNext(latestPath, t);
  }

  void doInitialMultisearch(
        vector<pair<timeT, inT>> entries,
        vector<Treelet> &treelets) {
    std::deque<ipathBreadcrumb> latestPath;

    for (const auto [time, value]: entries) {
      auto [node, found] = multiSearchFind(latestPath, time);
      if (found) {
        node->localInsertEntry(_binOp, time, _binOp.lift(value));
        treelets.push_back(Treelet(node)); // Trigger propagation
      } else
        treelets.push_back(Treelet(time, value, node, NULL));
    }
    cout << "Initial treelets = [";
    for (auto tl: treelets) { cout << tl << " "; }
    cout << "]" << endl;
  }

  void destroy(Node* node) {
    if (!node->isLeaf()) {
        for (int i = 0; i < node->arity(); i++) {
            destroy(node->getChild(i));
        }
    }
    delete node;
  }

public:
  Aggregate(binOpFunc binOp)
    : _binOp(binOp),
      _root(new Node(true)),
      _leftFinger(NULL), _rightFinger(NULL),
      _size(0)
  {
    IF_COLLECT_STATS(statsCombineCount = 0);
    if (kind!=classic) {
      _leftFinger = _root;
      _rightFinger = _root;
    }
  }

  ~Aggregate() {
    IF_COLLECT_STATS(cout << "# of combine calls: " << statsCombineCount << endl);
    IF_COLLECT_STATS(cout << "# of nodes repaired: " << statsTotalNumNodesRepaired << endl);
    IF_COLLECT_STATS(cout << "# of repair calls involving a spine: " << statsTotalSpineRepairCount << endl);
    IF_COLLECT_STATS(cout << "# of repair calls: " << statsTotalRepairAggCount << endl);
    IF_COLLECT_STATS(cout << "avg root degree when repairs were made: " <<  ((double) statsSumRootDegree) / statsTotalRepairAggCount << endl);

    destroy(_root);
  }

  aggT at(timeT const& time) {
    Node* node = searchNode(time);
    for (int i=0, n=node->arity()-1; i<n; i++)
      if (time == node->getTime(i))
        return node->getValue(i);
    return binOpFunc::identity;
  }

  bool checkInvariantRec(const char* f, int l,
                         Node* node, Node* top1, Node* top2) const {
    bool strong;
    if (top1 == NULL)
      strong = true;
    else if (node->leftSpine() || node->rightSpine())
      strong = false;
    else
      strong = node->isDescendent(top1) || node->isDescendent(top2);
    bool ok = true;
    ok &= node->localCheckInvariant(_binOp, strong, f, l);
    if (!node->isLeaf())
      for (int i=0, n=node->arity(); i<n; i++)
        ok |= checkInvariantRec(f, l, node->getChild(i), top1, top2);
    return ok;
  }

  bool checkInvariant(const char* f, int l,
                      Node* top1=NULL, Node* top2=NULL) const {
    return checkInvariantRec(f, l, _root, top1, top2);
  }

  void evict() {
    Node* leaf = searchOldestNode();
    assert(leaf->isLeaf());
    if (false) cout << "- " << (kind==finger?"finger-":(kind==knuckle?"knuckle-":"finger-")) << minArity << " evict [scott: time_point can't print] " /*<< leaf->getTime(0) << '/' << leaf->getValue(0)*/ << endl;
    leaf->popFront(_binOp, 1);
    bool hitLeft=false, hitRight=false;
    Node* topChanged = rebalanceAfterEvict(leaf, &hitLeft, &hitRight);
    repairAggs(topChanged, hitLeft, hitRight);
    _size--;
    if (false) cout << *_root;
    assert(checkInvariant(__FILE__, __LINE__));
  }

  bool evict(timeT const& time) {
    Node* node = searchNode(time);
    int index;
    bool found = node->localSearch(time, index);
    if (!found)
      return false;
    if (false) cout << "- " << (kind==finger?"finger-":(kind==knuckle?"knuckle-":"finger-")) << minArity << " evict [scott: time_point can't print] " /*<< node->getTime(index) << '/' << node->getValue(index)*/ << endl;
    Node *topChanged;
    bool hitLeft, hitRight;
    if (node->isLeaf()) {
      node->localEvictEntry(_binOp, time);
      topChanged = rebalanceAfterEvict(node, &hitLeft, &hitRight);
    } else {
      Node *left = node->getChild(index), *right = node->getChild(index + 1);
      Node* leaf;
      timeT leafTime;
      aggT leafValue;
      if (right->arity() > minArity)
        leaf = oldest(right, leafTime, leafValue);
      else
        leaf = youngest(left, leafTime, leafValue);
      leaf->localEvictEntry(_binOp, leafTime);
      node->setEntry(index, leafTime, leafValue);
      topChanged = rebalanceAfterEvict(leaf, &hitLeft, &hitRight, node);
      if (topChanged->isDescendent(node)) {
        while (topChanged != node) {
          topChanged = topChanged->parent();
          hitLeft |= topChanged->leftSpine();
          hitRight |= topChanged->rightSpine();
          topChanged->localRepairAggIfUp(_binOp);
        }
        assert(topChanged == node);
      }
    }
    repairAggs(topChanged, hitLeft, hitRight);
    _size--;
    if (false) cout << *_root;
    assert(checkInvariant(__FILE__, __LINE__));
    return true;
  }

  void insert(timeT const& time, inT const& value) {
    if (false) cout << "- " << (kind==finger?"finger-":(kind==knuckle?"knuckle-":"finger-")) << minArity << " insert [scott: time_point can't print] " /*<< time << '/' << value*/ << endl;
    Node* node = searchNode(time);
    bool found = !node->localInsertEntry(_binOp, time, _binOp.lift(value));
    assert(found || node->isLeaf());
    bool hitLeft=false, hitRight=false;
    Node* topChanged = rebalanceAfterInsert(node, &hitLeft, &hitRight);
    repairAggs(topChanged, hitLeft, hitRight);
    // only increase the size if this creates a new entry in the tree
    if (!found) { _size++; }
    if (false) cout << *_root;
    assert(checkInvariant(__FILE__, __LINE__));
  }

  void insert(inT const& val) {
    if (0 == _size) {
      insert(0, val);
    } else {
      Node* leaf = searchYoungestNode();
      assert(leaf->isLeaf());
      timeT const time = 1 + leaf->getTime(leaf->arity() - 2);
      insert(time, val);
    }
  }

  class TreeletMerger {
    public:
    TreeletMerger(bool activateRightSpine)
      : times(), values(), children(), rightSpine(activateRightSpine) {}

    void mergeIn(Node *node, typename vector<Treelet>::iterator &start,
                 typename vector<Treelet>::iterator &end, size_t treeletCount) {
      size_t n = node->arity() + treeletCount - 1;
      size_t nodeIndex = 0;
      typename vector<Treelet>::iterator tlIt = start;
      if (!node->isLeaf()) { children.push_back(node->getChild(0)); }
      cout << "mergeIn: treeletCount=" << treeletCount << ", trail=";

      while (nodeIndex < node->arity()-1 && tlIt != end) {
        if (!tlIt->isReal()) {tlIt++; continue;} // skip "fake" treelets
        const timeT& tlTime = tlIt->time;
        const timeT& nTime = node->getTime(nodeIndex);

        cout << "C";
        if (tlTime < nTime) {
          if (!node->isLeaf()) { children.push_back(tlIt->rightChild); }
          values.push_back(tlIt->value);
          times.push_back((tlIt++)->time);
        }
        else {
          if (!node->isLeaf()) { children.push_back(node->getChild(nodeIndex+1)); }
          values.push_back(node->getValue(nodeIndex)); // TODO: check me
          times.push_back(node->getTime(nodeIndex++));
        }
        flipRightSpineFlag(node->isLeaf());
      }
      while (nodeIndex < node->arity()-1) { // done with treelets
        cout << "L";
        if (!node->isLeaf()) { children.push_back(node->getChild(nodeIndex+1)); }
        values.push_back(node->getValue(nodeIndex)); // TODO: check me
        times.push_back(node->getTime(nodeIndex));
        nodeIndex++;
        flipRightSpineFlag(node->isLeaf());
      }
      while (tlIt != end) {
        if (!tlIt->isReal()) {tlIt++; continue;} // skip "fake" treelets
        if (!node->isLeaf()) {
          children.push_back(tlIt->rightChild);
        }
        values.push_back(tlIt->value);
        times.push_back(tlIt->time);
        tlIt++;
        flipRightSpineFlag(node->isLeaf());
      }
      cout << "." << endl;
      // DEBUG prints
      cout << "tm: times = [";
      for (auto t: times) cout << t << ", ";
      cout << "]" << endl;
    }

    vector<timeT> times;
    vector<aggT> values;
    vector<Node *> children;
    bool rightSpine;
    private:
    void flipRightSpineFlag(bool leaf) {
      if (!leaf && rightSpine) {
        children[children.size()-2]->setRightSpine(false);
        children[children.size()-1]->setRightSpine(true);
      }
    }
  };
  struct TopsRecord {
    Node* topLeftSpine = NULL;
    Node* topRightSpine = NULL;
    bool rootTouched = false;

    void topOut(Node* target) {
      if (target->rightSpine())
        topRightSpine = target;
      if (target->leftSpine())
        topLeftSpine = target;
      if (target->isRoot()) {
        rootTouched = true;
        topLeftSpine = target, topRightSpine = target;
      }
    }
  };

  void doBulkLocalInsertNoOverflow(
    typename vector<Treelet>::iterator& groupStart,
    typename vector<Treelet>::iterator& groupEnd,
    Node *thisTarget)
  {
    for (auto it=groupStart;it!=groupEnd;it++) {
      if (!it->isReal()) continue;
      if (thisTarget->isLeaf())
        thisTarget->localInsertEntry(_binOp, it->time, it->value);
      else
        thisTarget->localInsert(_binOp, it->time, it->value, it->rightChild);
    }
    thisTarget->localRepairAggIfUp(_binOp); // Repair once at the end
  }

  Node* migrateInto(Node* old, Node *target, int pi, int ub, TreeletMerger& tm,
                      vector<Treelet> &nextTreelets) {
    cout << "migrateInto: old=" << old << ", pi=" << pi << ", ub=" << ub << endl;
    cout << "migrateInto: moved_times=";
    if (pi>=0) cout << "(" << tm.times[pi] << ")";
    cout << "[";
    for (int i=pi+1;i<ub;i++) cout << tm.times[i] << ", ";
    cout << "]" << endl;
    if (target == NULL)
      target = new Node(old->isLeaf());

    Node* parent = old->parent();
    if (parent == NULL) { cerr << "should never happen" << endl; throw 1; }
    target->setParent(parent);
    target->clear();
    if (!target->isLeaf())
      target->setOnlyChild(tm.children[pi+1]);

    for (int ci=pi+1;ci<ub;ci++) {
      if (target->isLeaf())
        target->pushBackEntry(_binOp, tm.times[ci], tm.values[ci]);
      else
        target->pushBack(_binOp, tm.times[ci], tm.values[ci], tm.children[ci+1]);
    }

    cout << "node formation completed " << target->repr() << endl;;
    if (pi >= 0) { // new treelet
      cout << "made new treelet " << target << ": t=" << tm.times[pi]
           << ", p=" << parent << endl;
      nextTreelets.push_back(
          Treelet(tm.times[pi], tm.values[pi], parent, target));
    }
    target->localRepairAggIfUp(_binOp);

    return target;
  }

  void massSplit(Node *thisTarget, TreeletMerger& tm,
                 vector<Treelet> &nextTreelets) {
    int rangeStart=-1, totalArity=tm.times.size()+1, a=minArity;
    Node* nn=thisTarget;
    bool fromRightSpine=thisTarget->rightSpine();
    // assumption: current node is overflowing
    if (thisTarget->isRoot()) heightIncrease(false);

    while (totalArity - rangeStart - 1 > maxArity) {
      if (nn == thisTarget && thisTarget->rightSpine())
        thisTarget->setRightSpine(false);
      nn = migrateInto(thisTarget, nn, rangeStart, rangeStart + a + 1, tm,
                       nextTreelets);
      rangeStart += a + 1, nn = NULL;
    }

    nn = migrateInto(thisTarget, nn, rangeStart, totalArity-1, tm,
                     nextTreelets);
    if (nn != thisTarget && fromRightSpine) {
      cout << "setting " << nn << " to right spine!" << endl;
      cout << "removing " << thisTarget << " from right spine." << endl;
      nn->setRightSpine(true);
      if (this->_rightFinger == thisTarget)
        this->_rightFinger = nn;
    }
  }

  void doBulkLocalInsertOverflow(typename vector<Treelet>::iterator& groupStart,
                                 typename vector<Treelet>::iterator& groupEnd,
                                 size_t groupCount, Node* thisTarget,
                                 vector<Treelet>& nextTreelets) {
    cout << "doBulkLocalInsert: groupCount=" << groupCount << endl;
    TreeletMerger tm(thisTarget->isRoot() && thisTarget->rightSpine());

    tm.mergeIn(thisTarget, groupStart, groupEnd, groupCount);
    massSplit(thisTarget, tm, nextTreelets);
  }

  void doBulkLocalInsert(vector<Treelet> &treelets,
                         vector<Treelet> &nextTreelets, TopsRecord &tops) {
    auto groupStart = treelets.begin();
    while (groupStart != treelets.end()) {
      auto groupEnd = groupStart;
      Node *thisTarget = groupStart->node;
      size_t groupCount = 0;
      while (groupEnd != treelets.end() && groupEnd->node == thisTarget) {
        if (groupEnd->isReal())
          groupCount++; // count real treelets
        groupEnd++;
      }

      cout << "thisTarget: rightSpine=" << thisTarget->rightSpine() << endl;
      // Case 1: This node won't overflow
      if (thisTarget->arity() + groupCount <= maxArity) {
        doBulkLocalInsertNoOverflow(groupStart, groupEnd, thisTarget);
      } else
        // Case 2: the node will overflow
        doBulkLocalInsertOverflow(groupStart, groupEnd, groupCount, thisTarget,
                                  nextTreelets);

      if (thisTarget->hasAggUp())
        nextTreelets.push_back(Treelet(thisTarget->parent()));
      else {
        cout << "topping out at " << thisTarget << endl;
        tops.topOut(thisTarget);
      }
      groupStart = groupEnd;
    }
  }

  void bulkInsert(vector<pair<timeT, inT>> entries) {
    if (kind != finger) throw -1; // only support finger trees
    vector<Treelet> thisTreelets, nextTreelets;
    doInitialMultisearch(entries, thisTreelets);

    TopsRecord tops;
    // Level by level insertions
    while (!thisTreelets.empty()) {
      nextTreelets.clear();
      doBulkLocalInsert(thisTreelets, nextTreelets, tops);
      thisTreelets.swap(nextTreelets);
      cout << "(next) treelets = [";
      for (auto tl: thisTreelets) { cout << tl << " "; }
      cout << "]" << endl;
    }

    cout << "top_rs=" << tops.topRightSpine << ", top_ls=" << tops.topLeftSpine
         << ", rootTouched=" << tops.rootTouched << endl;
    if (tops.rootTouched)
      _root->localRepairAgg(_binOp);

    if (tops.topLeftSpine != NULL) { // Repair the left spine
      Node *left = tops.topLeftSpine;
      left->localRepairAgg(_binOp);
      while (!left->isLeaf()) {
        left = left->getChild(0);
        left->localRepairAgg(_binOp);
      }
    }

    if (tops.topRightSpine != NULL) { // Repair the right spine
      Node *right = tops.topLeftSpine;
      right->localRepairAgg(_binOp);
      while (!right->isLeaf()) {
        right = right->getChild(right->arity()-1);
        right->localRepairAgg(_binOp);
      }
    }
    cout << "_dump: " << *_root << endl;
    checkInvariant(__FILE__, __LINE__);
  }

  timeT oldest() const {
    timeT time;
    aggT value;
    oldest(_root, time, value);
    return time;
  }

  Node* oldest(Node* node, timeT& time, aggT& value) const {
    while (!node->isLeaf())
      node = node->getChild(0);
    time = node->getTime(0);
    value = node->getValue(0);
    return node;
  }

  outT query() const {
    if (kind==finger) {
      if (_root->isLeaf()) {
        return _binOp.lower(_root->getAgg());
      } else {
        aggT leftAgg = _leftFinger->getAgg();
        aggT rootAgg = _root->getAgg();
        aggT rightAgg = _rightFinger->getAgg();
        IF_COLLECT_STATS(++statsCombineCount);
        IF_COLLECT_STATS(++statsCombineCount);
        return _binOp.lower(_binOp.combine(leftAgg,
                                           _binOp.combine(rootAgg, rightAgg)));
      }
    } else {
      return _binOp.lower(_root->getAgg());
    }
  }

  outT rangeQuery(timeT timeFrom, timeT timeTo) const {
    if (kind==finger) {
      Node *nodeFrom = searchNode(timeFrom), *nodeTo = searchNode(timeTo);
      Node* top = leastCommonAncestor(nodeFrom, nodeTo);
      return _binOp.lower(rangeQueryRec(*top, timeFrom, timeTo));
    } else {
      return _binOp.lower(rangeQueryRec(*_root, timeFrom, timeTo));
    }
  }

  size_t size() { return _size; }

  timeT youngest() const {
    timeT time;
    aggT value;
    youngest(_root, time, value);
    return time;
  }

  Node* youngest(Node* node, timeT& time, aggT& value) const {
    while (!node->isLeaf())
      node = node->getChild(node->arity() - 1);
    time = node->getTime(node->arity() - 2);
    value = node->getValue(node->arity() - 2);
    return node;
  }
};

template <typename timeT, int minArity, Kind kind, class BinaryFunction, class T>
Aggregate<timeT, minArity, kind, BinaryFunction>
make_aggregate(BinaryFunction f, T elem) {
  return Aggregate<timeT, minArity, kind, BinaryFunction>(f);
}

template <typename BinaryFunction, typename timeT, int minArity, Kind kind>
struct MakeAggregate {
  template <typename T>
  Aggregate<timeT, minArity, kind, BinaryFunction> operator()(T elem) {
    BinaryFunction f;
    return make_aggregate<timeT, minArity, kind>(f, elem);
  }
};
}

#endif
