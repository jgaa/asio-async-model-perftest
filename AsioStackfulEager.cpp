#include <vector>
#include <iostream>
#include <span>
#include <thread>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <optional>
#include <forward_list>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/program_options.hpp>

#include "aamp.h"

using namespace std;

namespace {
class Session {
public:
    Session(boost::asio::io_service& svc)
        : buffer_(config.bufferSize)
        , svc_{svc}
    {}

    void go() {
        boost::asio::spawn(svc_, [this](boost::asio::yield_context y) {
            // "Read" the input
            for(decltype(config.numBuffers) i = {}; i < config.numBuffers; ++i){
                buffer_.clear();
                boost::asio::deadline_timer timer{svc_};
                timer.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
                timer.async_wait(y);
                buffer_.resize(config.bufferSize);
                ranges::fill(buffer_, static_cast<char>(time(nullptr)));
            }

            // Process the data
            async_post(svc_, [this](){
                volatile char ch;
                for(volatile auto c: buffer_) {
                    ch = c;
                }
            }, y);

            // "Write" the output
            for(decltype(config.numBuffers) i = {}; i < config.numBuffers; ++i){
                boost::asio::deadline_timer timer{svc_};
                timer.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
                timer.async_wait(y);
                fill(buffer_.begin(), buffer_.end(), static_cast<char>(time(nullptr)));
            }

        });
    }

private:
    std::vector<char> buffer_;
    boost::asio::io_service& svc_;
};
}

class AsioStackfulEager : public Approach {
public:
    void go() override {
        boost::asio::io_service svc;
        std::forward_list<Session> sessions;
        for(auto i = 0; i < config.numSessions; ++i) {
            sessions.emplace_front(svc);
            sessions.front().go();
        }

        runIoService(svc, config.numThreads);
     }

    string name() const override {
        return "AsioStackfulEager";
    }
};

void addAsioStackfulEager(approaches_t& list) {
    list.push_back(make_unique<AsioStackfulEager>());
}
