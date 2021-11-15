#include "AMTA.hpp"
#include "AggregationFunctions.hpp"

using namespace std;
int main(int argc, char *argv[]) {
    auto f = Sum<int>();
    amta::Aggregate agg = amta::make_aggregate<long>(f, 0);

    for (int i=0;i<20;i++) {
        agg.insert(i, i+1);
        agg.print();
        cout << agg.query() << endl;
    }
    return 0;
}
