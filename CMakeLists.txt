cmake_minimum_required(VERSION 3.13)


project (aamp VERSION 0.0.1 LANGUAGES CXX)
message(STATUS "Using ${CMAKE_CXX_COMPILER}")

if (GCC11_INCLUDE_DIR)
    include_directories(${GCC11_INCLUDE_DIR})
endif()

message(STATUS "Configuring ${PROJECT_NAME} version ${PROJECT_VERSION}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING -DBoost_NO_WARN_NEW_VERSIONS=ON)

find_package(Boost 1.75 REQUIRED COMPONENTS
    system
    program_options
    date_time
    context
    coroutine
    chrono
    )

include_directories(${Boost_INCLUDE_DIR})

option(BOOST_ERROR_CODE_HEADER_ONLY "Work-around for another boost issue" ON)
if (BOOST_ERROR_CODE_HEADER_ONLY)
    add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY=1)
endif()


if (UNIX)
  find_package(Threads REQUIRED)
endif()

add_executable(${PROJECT_NAME}
  AsioCallbacksEager.cpp
  AsioStackfulLazy.cpp
  AsioStaclessLazy.cpp
  AsioStackfulEager.cpp
  AsioStacklessEager.cpp
  AsioCpp20CoroEager.cpp
  aamp.h
  main.cpp
  util.cpp
  )

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_compile_options(${PROJECT_NAME} PRIVATE
    -std=c++20 -fcoroutines
    )
target_link_libraries(${PROJECT_NAME} PRIVATE
  ${Boost_LIBRARIES}
  stdc++fs
  ${CMAKE_THREAD_LIBS_INIT}
  )
