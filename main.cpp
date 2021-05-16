
#include <vector>
#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>

#include <sys/time.h>
#include <sys/resource.h>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/program_options.hpp>

#include "aamp.h"

using namespace std;

Config config;

int main(int argc, char **argv) {    
    try {
        locale loc("");
    } catch (const std::exception& e) {
        std::cout << "Locales in Linux are fundamentally broken. Never worked. Never will. Overriding the current mess with LC_ALL=C" << endl;
        setenv("LC_ALL", "C", 1);
    }

    approaches_t approaches;
    // Eager
    addAsioCallbacksEager(approaches);
    addAsioCallbacksEagerAsyncPost(approaches);
    addAsioStackfulEager(approaches);
    addAsioStacklessEager(approaches);
    addAsioCpp20CoroEager(approaches);

    // Lazy
    addAsioStackfulLazy(approaches);
    addAsioStacklessLazy(approaches);


    namespace po = boost::program_options;
    po::options_description general("Options");
    general.add_options()
        ("help,h", "Print help and exit")
        ("approach,a",
            po::value<size_t>(&config.approach)->default_value(config.approach),
            "What approach (implementation) to use.")
        ("sessions",
            po::value<uint64_t>(&config.numSessions)->default_value(config.numSessions),
            "Number of \"sessions\" to run.")
        ("threads",
            po::value<uint64_t>(&config.numThreads)->default_value(config.numThreads),
            "Number of threads to use.")
        ("buffers",
            po::value<uint64_t>(&config.numBuffers)->default_value(config.numBuffers),
            "Number of \"buffers\" to work with for each session.")
        ("buffer-size",
            po::value<uint64_t>(&config.bufferSize)->default_value(config.bufferSize),
            "Size of one buffer in bytes.")
        ("sleep-interval",
            po::value<uint64_t>(&config.numCharsBetweenWait)->default_value(config.numCharsBetweenWait),
            "Number of \"input\" characters between sleep.")
        ("sleep-time",
            po::value<int>(&config.waitTimeMillisec)->default_value(config.waitTimeMillisec),
            "Number of milliseconds to sleep.")
        ("report-path,r",
            po::value<string>(&config.reportPath)->default_value(config.reportPath),
            "CSV file where to append the results")
        ;

    po::options_description cmdline_options;
    cmdline_options.add(general);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
        cout << filesystem::path(argv[0]).stem().string() << " [options]";
        cout << cmdline_options << endl;
        cout << "Approaches: " << endl;

        int i = 0;
        for(const auto& a: approaches) {
            cout << "  " << i++ << ' ' << a->name() << endl;
        }
        return -1;
    }

    const auto sessionBytes = config.numBuffers * config.bufferSize;
    const auto totalBytes = sessionBytes * config.numSessions;
    const auto totalSleep = (totalBytes / config.numCharsBetweenWait) * config.waitTimeMillisec;

    cout << "Starting up." << endl
         << "Will run " << format(config.numSessions) << " sessions using "
         << format(config.numThreads) << " thread(s), each manipulating "
         << format(sessionBytes) << " bytes" << endl
         << "Total bytes is " << format(totalBytes) << endl
         << "Will suspend sessions " << format(totalBytes / config.numCharsBetweenWait)
         << " times for a total of "
         << format(totalSleep)
         << " milliseconds (" << format(totalSleep / 1000) << " seconds)." << endl;

    cout << "Exceuting " << approaches.at(config.approach)->name() << endl;
    approaches.at(config.approach)->go();

    rusage ru = {};
    getrusage(RUSAGE_SELF, &ru);

    cout << "cputime=" << (ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1000000.0)
         << ", system=" << (ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1000000.0)
         << ", minflt=" << ru.ru_minflt
         << ", majflt=" << ru.ru_majflt
         << ", nswap=" << ru.ru_nswap
         << ", nsignals=" << ru.ru_nsignals
         << ", nvcsw=" << ru.ru_nvcsw
         << ", nivcsw=" << ru.ru_nivcsw
         << ", maxrss=" << ru.ru_maxrss
         << ", ixrss=" << ru.ru_ixrss
         << ", idrss=" << ru.ru_idrss
         << ", isrss=" << ru.ru_isrss
         << endl;

    if (!config.reportPath.empty()) {
        if (!filesystem::is_regular_file(config.reportPath)) {
            ofstream hdr{config.reportPath};
            hdr << "Approach, cputime, system, minflt, majflt, nswap, "
                << "nsignals, nvcsw, nivcsw, maxrss, ixrss, idrss, isrss" << endl;
        }

        ofstream data{config.reportPath, ::ios_base::app};
        data << approaches.at(config.approach)->name()
             << "," << (ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1000000.0)
             << "," << (ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1000000.0)
             << "," << ru.ru_minflt
             << "," << ru.ru_majflt
             << "," << ru.ru_nswap
             << "," << ru.ru_nsignals
             << "," << ru.ru_nvcsw
             << "," << ru.ru_nivcsw
             << "," << ru.ru_maxrss
             << "," << ru.ru_ixrss
             << "," << ru.ru_idrss
             << "," << ru.ru_isrss
             << endl;
    }

    cout << "Done" << endl;
    return 0;
}
