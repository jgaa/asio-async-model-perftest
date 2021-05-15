# asio-async-model-perftest
Performance test to measure the relative performance between asio's different async handlers

This project implements a simple async server work-flow 
that "reads" a number of input buffers, process the data and
then "writes" a number  of output buffers. 

The same logic is implemented as:

- Asio stackful coroutines (via spawn)
- Asio async methods with completion callbacks
- Asio stackless C++ 20 coroutines (via co_spawn)

The motivation is to measure the performance regression using the 
more convenient methods, like stateful coroutines or C++ 20 coroutines. 

In addition, the test provides implementations of layered C++20 generators, using
lazy fetching/flushing of data at the lowest layer. This test is implemented for:

- Asio stackful coroutines (via spawn)
- Asio stackless C++ 20 coroutines (via co_spawn)

# Requirements
- gcc 11
- Boost 1.76.0 or newer

# Status
In progress. Once done, I will post results and graphs with results from a
number of hardware configurations. 

