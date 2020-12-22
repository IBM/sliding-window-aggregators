#include "RingBufferQueue.hpp" 
#include<iostream>
#include<deque>

void printUsingIter(RingBufferQueue<int> &rb) {
    RingBufferQueue<int>::iterator it = rb.begin();

    // std::cout << "|> " << it.aap << std::endl;
    // std::cout << ">| " << rb.end().aap << std::endl;
    std::cout << "|> " << (it.it - it.rb->buffer) << ", "; // std::endl;
    std::cout << ">| " << (rb.end().it - it.rb->buffer) << std::endl;
    
    while (it != rb.end()) {
        std::cout << (*it)  << " ";
        it++;
    }
    std::cout << std::endl;
}

int main(int argc, char const *argv[])
{
    RingBufferQueue<int> rb;

    int pb[] = {3, 1, 4, 9, 2, 6};
    for (auto val: pb) {
        rb.push_back(val);
        std::cout << "#++" << rb.front() << " " << rb.back() << std::endl;
        printUsingIter(rb);
        std::cout << "++++++" << std::endl;
    }

    while (rb.size() > 0) {
        std::cout<< "#--"  << rb.front() << " " << rb.back() << std::endl;
        rb.pop_front();
        printUsingIter(rb);
        // std::cout << "fix4: " << (*fix4) << std::endl;
        // std::cout << "++++++" << std::endl;
    }

    for (auto val: pb) {
        rb.push_back(val);
        std::cout << "#++" << rb.front() << " " << rb.back() << std::endl;
        printUsingIter(rb);
        std::cout << "++++++" << std::endl;
    }
    while (rb.size() > 0) {
        std::cout << "#--" << rb.front() << " " << rb.back() << std::endl;
        rb.pop_front();
        printUsingIter(rb);
    }

    RingBufferQueue<int>::iterator eob = rb.end();

    std::cout << "rb.capacity=" << rb._rb->capacity << ", end.it=" << rb.end().it 
    << ", begin.it=" << rb.begin().it
    << ", eob.it=" << eob.it << std::endl;

    std::cout << "---------------" << std::endl;

    const int N = 10000;
    srand(0x101);
    std::deque<int> dd;
    for (int c=0;c<N;c++) {
        int cmd = rand() % 10001 - 5000;

        if (cmd > 0) {
            rb.push_back(cmd);
            dd.push_back(cmd);
        }
        else {
            if (rb.size() > 0) {
                rb.pop_front();
                dd.pop_front();
            }
        }
        if (rb.size()> 0 && (dd.front() != rb.front() ||
            dd.back() != dd.back()) ) {
                throw 1;
        }
        if (rb.size() != dd.size()) {
            throw 1;
        }
    }

    return 0;
}
