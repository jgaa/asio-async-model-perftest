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
#include <syncstream>

#include <boost/asio.hpp>
#include <boost/asio/yield.hpp>

#include "aamp.h"

using namespace std;

namespace {
class Session {
public:
    Session(boost::asio::io_service& svc)
        : buffer_(config.bufferSize)
        , strand_{svc}
        , timer_{strand_.context().get_executor()}
        , work_{svc}
    {
    }

    ~Session() {
    }

    void operator()(boost::system::error_code ec = {}) {
        reenter (coro_) {
            for(;i_ < config.numBuffers; ++i_) {
                buffer_.clear();
                timer_.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
                yield timer_.async_wait(std::ref(*this));
                buffer_.resize(config.bufferSize);
                ranges::fill(buffer_, static_cast<char>(time(nullptr)));
            }

            yield async_post(strand_, [this]() mutable {
                        volatile char ch;
                        for(const auto c: buffer_) {
                            ch = c;
                        }
            }, std::ref(*this));

            i_ = 0;

            for(;i_ < config.numBuffers; ++i_) {
                timer_.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
                yield timer_.async_wait(std::ref(*this));
                fill(buffer_.begin(), buffer_.end(), static_cast<char>(time(nullptr)));
            }

            work_.reset();
        }
    }

    void go() {
        operator()();
    }

private:
    boost::asio::coroutine coro_;
    std::vector<char> buffer_;
    boost::asio::io_service::strand strand_;
    boost::asio::deadline_timer timer_;
    decltype (config.numBuffers) i_ = {};
    std::optional<boost::asio::io_service::work> work_;
};
}

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
