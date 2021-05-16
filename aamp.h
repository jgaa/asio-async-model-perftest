#pragma once

#include <string>
#include <thread>

#include <boost/asio.hpp>

struct Config {
    size_t approach = 0;
    uint64_t numSessions = std::thread::hardware_concurrency() * 1600;
    size_t numThreads = std::thread::hardware_concurrency();
    uint64_t numBuffers = 1000;
    uint64_t bufferSize = 1024 * 4;
    uint64_t numCharsBetweenWait = bufferSize * 3;
    int waitTimeMillisec = 1;
    std::string reportPath;
};

extern Config config;

class Approach {
public:
    virtual std::string name() const = 0;
    virtual void go() = 0;
};

std::string format(uint64_t val, char sep = ' ');

void runIoService(boost::asio::io_service& svc, size_t numThreads);

using approaches_t = std::vector<std::unique_ptr<Approach>>;

void addAsioStackfulLazy(approaches_t& list);
void addAsioStacklessLazy(approaches_t& list);
void addAsioCallbacksEager(approaches_t& list);
void addAsioCallbacksEagerAsyncPost(approaches_t& list);
void addAsioStackfulEager(approaches_t& list);
void addAsioStacklessEager(approaches_t& list);
void addAsioCpp20CoroEager(approaches_t& list);

template <typename Fn, typename CompletionToken, typename Executor>
auto async_post(Executor&& executor, Fn&& fn, CompletionToken&& token) {

    auto initiation = [](auto && completionHandler, Executor& executor, Fn&& fn) mutable {

        boost::asio::post(executor, [fn=std::move(fn), ch=std::move(completionHandler)]() mutable {
            fn();
            ch();
        });
    };

    return boost::asio::async_initiate<CompletionToken, void()> (
                initiation, token, std::ref(executor), std::move(fn));
}
