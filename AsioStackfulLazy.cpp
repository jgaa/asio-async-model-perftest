
#include <vector>
#include <chrono>
#include <iostream>
#include <ranges>
#include <span>
#include <syncstream>
#include <thread>
#include <coroutine>
#include <utility>
#include <algorithm>
#include <filesystem>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/program_options.hpp>

#include "aamp.h"

using namespace std;

namespace {
// From C++ High Performance C++, 2. edition
template <typename T>
class Generator {
    struct Promise {
        T value_;
        Generator get_return_object() {
            return Generator{coroutine_handle<Promise>::from_promise(*this)};
        }

        auto initial_suspend() {
            return suspend_always{};
        }

        auto final_suspend() noexcept {
            return suspend_always{};
        }

        void return_void() {};

        void unhandled_exception() { throw; }

        auto yield_value(T&& value) {
            value_ = move(value);
            return suspend_always{};
        }

        auto yield_value(const T& value) {
            value_ = value;
            return suspend_always{};
        }
    };

    using ch_t = coroutine_handle<Promise>;

    struct Sentiel {};
    struct Iterator {
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator& operator++() {
            h_.resume();
            return *this;
        }

        void operator++(int) {
            (void)operator++();
        }

        T operator*() const {
            return h_.promise().value_;
        }

        T* operator->() const {
            return addressof(operator*());
        }

        bool operator == (Sentiel) const {
            return h_.done();
        }

        ch_t h_;
    };

    ch_t h_;

    explicit Generator(ch_t h) : h_{h} {}
public:
    using promise_type = Promise;
    Generator(Generator&& g)
        : h_{exchange(g.h_, {})}
    {}

    ~Generator() {
        if (h_) {
            h_.destroy();
        }
    }

    auto begin() {
        h_.resume();
        return Iterator{h_};
    }

    auto end() {
        return Sentiel{};
    }
};

template <typename T = char, bool coro = false>
Generator<T> generateCharacters(auto& y, auto& ios) {
    for(uint64_t num = 0; num < (config.bufferSize * config.numBuffers); ++num) {
        if ((num % config.numCharsBetweenWait) == 0) {
            boost::asio::deadline_timer timer(ios);
            timer.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
            timer.async_wait(y);
        }

        co_yield static_cast<char>(num % sizeof(char));
    }
}

template <typename T = span<char>, typename InRange>
Generator<T> transformCharsToBuffer(InRange data) {
    vector<char> buffer(config.bufferSize);
    buffer.clear();
    uint64_t used = {};
    for(auto c : data) {
        if (used == config.bufferSize) {
            //basic_osyncstream(clog) << "Have buffer" << endl;
            co_yield {buffer.data(), buffer.size()};
            buffer.clear();
            used = {};
        }

        buffer.emplace_back(c);
        ++used;
    }

    if (!buffer.empty()) {
        co_yield span{buffer.data(), buffer.size()};
    }
}

template <typename T = char, typename InRange>
Generator<T> transformBuffersToChar(InRange data) {
    for(auto b: data) {
        for(auto c : b) {
            co_yield c;
        }
    }
}
}
//
class StackfulSession {
public:
    StackfulSession(boost::asio::yield_context& y, boost::asio::io_service& ios)
        : y_{y}, ios_{ios} {}

    void run() {
        const auto count = std::ranges::distance(
                    transformBuffersToChar(
                        transformCharsToBuffer(
                            generateCharacters(y_, ios_))));
        assert(count == (config.numBuffers * config.bufferSize));
    }

private:
   boost::asio::yield_context& y_;
   boost::asio::io_service& ios_;
};


class AsioStackfulLazy : public Approach {
public:
    void go() override {
        boost::asio::io_service svc;
        for(auto i = 0; i < config.numSessions; ++i) {
            boost::asio::spawn([&svc](boost::asio::yield_context y) {
                StackfulSession s{y, svc};
                s.run();
            });
            runIoService(svc, config.numThreads);
         }
     }

    string name() const override {
        return "AsioStackfullLazy";
    }
};

void addAsioStackfulLazy(approaches_t& list) {
    list.push_back(make_unique<AsioStackfulLazy>());
}
