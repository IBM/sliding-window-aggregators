#include "AMTA.hpp"
#include "RingBufferQueue.hpp"
#include "AggregationFunctions.hpp"
#include "TimestampedDABALite.hpp"

using namespace std;
using timestampT = long;


void simpleCycles(int n){
    auto f = Sum<int>();
    amta::Aggregate agg = amta::make_aggregate<timestampT>(f, 0);
    auto daba = timestamped_dabalite::make_aggregate<timestampT>(f, 0);
    for (int i=0;i<n;i++) {
        agg.insert(i, i+1);
        daba.insert(i, i+1);
        agg.print();
        auto aggQuery = agg.query();
        cout << aggQuery << endl;
        assert(aggQuery == daba.query());
    }

    for (int i=0;i<n-1;i++) {
        agg.evict();
        daba.evict();
        agg.print();
        auto aggQuery = agg.query();
        cout << aggQuery << endl;
        assert(aggQuery == daba.query());
    }
    agg.insert(n+1, 7);
    daba.insert(n+1, 7);
    agg.print();
    auto aggQuery = agg.query();
    cout << aggQuery << endl;
    assert(aggQuery == daba.query());
}

void simpleBulkEvict()  {
    int N = 1499;
    auto f = Sum<int>();
    amta::Aggregate agg = amta::make_aggregate<timestampT>(f, 0);
    auto daba = timestamped_dabalite::make_aggregate<timestampT>(f, 0);

    for (int i=0;i<N;i++) {
        agg.insert(i, i+1);
        daba.insert(i, i+1);
    }
#define ONE_DANCE(t) {  \
        agg.bulkEvict(t); \
        while (daba.size()>0 && daba.oldest() <= t) daba.evict(); \
        assert(agg.query() == daba.query()); \
    }  ;

    ONE_DANCE(439);
    ONE_DANCE(981);
    ONE_DANCE(1439);
    ONE_DANCE(1456);
    ONE_DANCE(1792);
}
int main(int argc, char *argv[]) {
    simpleCycles(17);
    simpleBulkEvict();
    return 0;
}
