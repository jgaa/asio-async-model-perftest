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

#include "aamp.h"

using namespace std;

class Session {
public:
    Session(boost::asio::io_service& svc)
        : buffer_(config.bufferSize)
        , svc_{svc}
        , strand_{svc}
    {}

    boost::asio::awaitable<void> start() {

        // "Read" the input
        for(decltype(config.numBuffers) i = {}; i < config.numBuffers; ++i){
            buffer_.clear();
            boost::asio::deadline_timer timer{svc_};
            timer.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
            co_await timer.async_wait(boost::asio::use_awaitable);
            buffer_.resize(config.bufferSize);
            ranges::fill(buffer_, static_cast<char>(time(nullptr)));
        }

        // Process the data
        auto& b = buffer_;
        co_await async_post(strand_, [&b]() mutable {
            volatile char ch;
            for(const auto c: b) {
                ch = c;
            }
        }, boost::asio::use_awaitable);


        // "Write" the output
        for(decltype(config.numBuffers) i = {}; i < config.numBuffers; ++i){
            boost::asio::deadline_timer timer{svc_};
            timer.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
            co_await timer.async_wait(boost::asio::use_awaitable);
            fill(buffer_.begin(), buffer_.end(), static_cast<char>(time(nullptr)));
        }

        co_return;
    }

    void go() {
        boost::asio::co_spawn(strand_.context().get_executor(), [this] () mutable  {
            return start();
        }, boost::asio::detached);
    }

private:
    std::vector<char> buffer_;
    boost::asio::io_service& svc_;
    boost::asio::io_service::strand strand_;
};


class AsioStacklessEager : public Approach {
public:
    void go() override {
        boost::asio::io_service svc;
        std::forward_list<Session> sessions;
        for(decltype(config.numSessions) i = 0; i < config.numSessions; ++i) {
            sessions.emplace_front(svc);
            sessions.front().go();
        }

        runIoService(svc, config.numThreads);
     }

    string name() const override {
        return "AsioStacklessEager";
    }
};

void addAsioStacklessEager(approaches_t& list) {
    list.push_back(make_unique<AsioStacklessEager>());
}
