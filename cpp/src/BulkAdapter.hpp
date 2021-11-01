#pragma once
#include<vector>
#include<utility>

template <class Base, typename timeT, typename inT>
class BulkAdapter : public Base {
  using Base::Base;

  void bulkInsert(std::vector<std::pair<timeT, inT>> entries) {
    bulkInsert(entries.begin(), entries.end());
  }

  template <class Iterator>
  void bulkInsert(Iterator begin, Iterator end) {
      for (auto it=begin;it!=end;it++) {
          Base::insert(it->first, it->second);
      }
  }

  void bulkEvict(timeT const& time) {
      while (Base::oldest() <= time) {
          Base::evict();
      }
  }
};
