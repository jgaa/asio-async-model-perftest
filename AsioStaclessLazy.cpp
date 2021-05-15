
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

struct StacklessSession {
    using ch_awaitable_t = boost::asio::awaitable<char>;
    using span_awaitable_t = boost::asio::awaitable<std::span<char>>;
public:
    struct CharGenerator {
        CharGenerator(boost::asio::io_service::strand& s)
            : end_{config.bufferSize * config.numBuffers}, strand_{s}
        {}

        ch_awaitable_t getc() {
            if ((++i_ % config.numCharsBetweenWait) == 0) {
                boost::asio::deadline_timer timer(strand_.context());
                timer.expires_from_now(boost::posix_time::millisec(config.waitTimeMillisec));
                co_await timer.async_wait(boost::asio::use_awaitable);
            }

            assert(!eof());
            co_return static_cast<char>(i_ % sizeof(char));
        }

        bool eof() const noexcept {
            return i_ >= end_;
        }

        uint64_t i_ = {};
        const uint64_t end_;
        boost::asio::io_service::strand& strand_;
    };

    struct ToBuffer {
        ToBuffer(CharGenerator& cg) : cg_{cg}, buffer_(config.bufferSize) {
            buffer_.clear();
        }

        span_awaitable_t getb() {
//            basic_osyncstream(clog) << "getb() says hello" << endl;
            buffer_.clear();
            while(!cg_.eof() && buffer_.size() < config.bufferSize) {
                buffer_.push_back(co_await cg_.getc());
            }

//            basic_osyncstream(clog) << "Have buffer of size " << buffer_.size()
//                                    << ", i=" << cg_.i_ << endl;
            co_return span<char>{buffer_.data(), buffer_.size()};
        }

        bool eof() const noexcept {
            return cg_.eof();
        }

        CharGenerator& cg_;
        vector<char> buffer_;
    };

    struct ToStream {
        ToStream(ToBuffer& tb) : tb_{tb} {}

        ch_awaitable_t getc() {
            if (it_ == current_.end()) {
                current_ = co_await tb_.getb();
                it_ = current_.begin();
                if (current_.empty()) {
                    co_return '\0'; // One after eof, but we don't have a magic value, and we don't care here.
                }
            }

            assert(it_ != current_.end());
            co_return *it_++;
        }

        bool eof() const noexcept {
            return tb_.eof();
        }

        ToBuffer& tb_;
        std::span<char> current_;
        decltype (current_.end()) it_ = current_.end();
    };


    StacklessSession(boost::asio::io_service& ios)
        : strand{ios} {}

    boost::asio::awaitable<void> run() {
        //basic_osyncstream(clog) << "run() says hello" << endl;


        CharGenerator cg{strand};
        ToBuffer tb{cg};
        ToStream ts{tb};

        while(!cg.eof()) {
//            auto ch = co_await boost::asio::co_spawn(strand.context().get_executor(), [&cg]() mutable -> ch_awaitable_t {
//                co_return co_await cg.getc();
//            }, boost::asio::detached);

            auto b = co_await ts.getc();
        }

        assert(cg.i_ == (config.numBuffers * config.bufferSize));
//        basic_osyncstream(clog) << "run() says goodbye. i=" << cg.i_ << ", end=" << cg.end_
//                                << ", eof()=" << cg.eof() << endl;
    }

    boost::asio::io_service::strand strand;
};

class AsioStacklessLazy : public Approach {
public:
    void go() override {
        boost::asio::io_service svc;
        for(auto i = 0; i < config.numSessions; ++i) {
            auto session = make_shared<StacklessSession>(svc);
            boost::asio::co_spawn(session->strand.context().get_executor(), [session] () {
                return session->run();
            }, boost::asio::detached);
            runIoService(svc, config.numThreads);
         }
     }

    string name() const override {
        return "AsioStacklessLazy";
    }
};

void addAsioStacklessLazy(approaches_t& list) {
    list.push_back(make_unique<AsioStacklessLazy>());
}
