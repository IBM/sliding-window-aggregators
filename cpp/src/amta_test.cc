#include "AMTA.hpp"
#include "AggregationFunctions.hpp"

using namespace std;
int main(int argc, char *argv[]) {
    auto f = Sum<int>();
    amta::Aggregate agg = amta::make_aggregate<long>(f, 0);
    int n=5;
    for (int i=0;i<n;i++) {
        agg.insert(i, i+1);
        agg.print();
        cout << agg.query() << endl;
    }

    for (int i=0;i<n-1;i++) {
      agg.evict();
      agg.print();
      cout << agg.query() << endl;
    }
    agg.insert(n+1, 7);
    agg.print();
    cout << agg.query() << endl;
    return 0;
}
