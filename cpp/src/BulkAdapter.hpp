#pragma once

template <class Base, typename timeT, typename inT>
class BulkAdapter : public Base {

  void bulkInsert(vector<pair<timeT, inT>> entries) {
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
