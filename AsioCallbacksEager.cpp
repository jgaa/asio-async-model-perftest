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
template <bool useNormalPost>
class Session {
public:
    Session(boost::asio::io_service& svc)
        : strand_{svc}
        , timer_{strand_.context()}
        , buffer_(config.bufferSize)
    {}

    template<typename Callback>
    void asyncRead(Callback cb) {
        buffer_.clear();
        timer_.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
        timer_.async_wait([this, cb](auto ec) mutable {
            if (!ec) {
                buffer_.resize(config.bufferSize);
                ranges::fill(buffer_, static_cast<char>(time(nullptr)));
                cb(std::span<char>(buffer_.begin(), buffer_.end()));
            }
        });
    }

    template<typename Callback>
    void asyncWrite(Callback cb) {
        buffer_.clear();
        timer_.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
        timer_.async_wait([this, cb](auto /*ec*/) {
            fill(buffer_.begin(), buffer_.end(), static_cast<char>(time(nullptr)));
            cb(std::span<char>(buffer_.begin(), buffer_.end()));
        });
    }

    void onWritten() {
        // See if we have to continue reading
        if (++numWrites_ <= static_cast<unsigned>(config.numBuffers)) {
            asyncWrite([this](auto data) {
               onWritten();
            });
            return;
        }
    }

    void onRead(const span<char>& data) {
        // See if we have to continue reading
        if (++numReads_ <= static_cast<unsigned>(config.numBuffers)) {
            asyncRead([this](auto data) {
               onRead(data);
            });
            return;
        }

        if constexpr (useNormalPost) {
            // Let's process the data
            strand_.post([this]{
                // Try to convince the compiler to not optimize away the nonsense loop
                volatile char ch;
                for(volatile auto c: buffer_) {
                    ch = c;
                }

                // Now, start replying
                asyncWrite([this] (auto data){
                   onWritten();
                });
            });
        } else {
            // Same as above, but using a completion function via the generic async_post()
            // Gives us an opportunity to measure it's overhead
            async_post(strand_, [this] {
                // Try to convince the compiler to not optimize away the nonsense loop
                volatile char ch;
                for(volatile auto c: buffer_) {
                    ch = c;
                }
            }, [this] () mutable {
                // Now, start replying
                asyncWrite([this] (auto data){
                   onWritten();
                });
            });
        }
    }

    void go() {
        strand_.post([this]{
            asyncRead([this](auto data) {
               onRead(data);
            });
        });
    }

private:
    boost::asio::io_service::strand strand_;
    unsigned numReads_ = 0;
    unsigned numWrites_ = 0;
    boost::asio::deadline_timer timer_;
    std::vector<char> buffer_;
};
}

template <bool useNormalPost>
class AsioCallbacksEager : public Approach {
public:
    void go() override {
        boost::asio::io_service svc;
        std::forward_list<Session<useNormalPost>> sessions;
        for(auto i = 0; i < config.numSessions; ++i) {
            sessions.emplace_front(svc);
            sessions.front().go();
        }

        runIoService(svc, config.numThreads);
     }

    string name() const override {
        return useNormalPost ? "AsioCallbacksEager" : "AsioCallbacksEagerAsyncPost";
    }
};

void addAsioCallbacksEager(approaches_t& list) {
    list.push_back(make_unique<AsioCallbacksEager<true>>());
}

void addAsioCallbacksEagerAsyncPost(approaches_t& list) {
    list.push_back(make_unique<AsioCallbacksEager<false>>());
}
