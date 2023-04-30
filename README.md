# asio-async-model-perftest
Performance test to measure the relative performance between asio's different async handlers

This project implements a simple async server work-flow 
that "reads" a number of input buffers, process the data and
then "writes" a number  of output buffers. 

In other words, we aim to simulate typical memory usage of a server application - draining the CPU cache.

The same logic is implemented using:

- Asio async methods with completion callbacks
- Asio stackful coroutines (via `spawn()`)
- Asio legacy stackless coroutines
- Asio stackless C++ 20 coroutines (via `co_spawn()`)

The motivation is to measure the performance regression using the 
more convenient methods, like stateful coroutines or C++ 20 coroutines. 

In addition, the test provides implementations of layered C++20 generators, using
lazy fetching/flushing of data at the lowest layer. This test is implemented for:

- Asio stackful coroutines (via `spawn()`)
- Asio stackless C++ 20 coroutines (via `co_spawn()`)

# Requirements
- gcc 11 or newer
- Boost 1.76.0 or newer (1.81 is broken, 1.82 works).

# Status
In progress. Once done, I will post results and graphs with results from a
number of hardware configurations. 

# Run all tests:

Example on how to run all the performance-tests.

From the build/bin directory with boost in `/opt/boost/boost_1_82_0`:
```sh
LD_LIBRARY_PATH=/opt/boost/boost_1_82_0/stage/lib PATH="$PATH:."  bash ~/src/asio-async-model-perftest/run-all.sh

```
