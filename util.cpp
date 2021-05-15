
#include <iostream>
#include <sstream>

#include "aamp.h"


using namespace std;

string format(uint64_t val, char sep) {
    stringstream o;

    auto s = to_string(val);

    if (s.size() < 2) {
        return s;
    }

    for (size_t i = 0; i < s.size(); ++i){
        if (i && !((s.size() - i) % 3)) {
            o << sep;
        }
        o << s.at(i);
    }

    return o.str();
}

void runIoService(boost::asio::io_service& svc, size_t numThreads) {
    vector<thread> threads;

    for(decltype (numThreads) i = 0; i < numThreads; ++i) {
        threads.emplace_back([&svc]{
            svc.run();
        });
    }

    for(auto& t: threads) {
        t.join();
    }
}
