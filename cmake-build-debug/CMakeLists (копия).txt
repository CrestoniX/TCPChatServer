cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
include("tools/gate/cmake/HunterGate.cmake")
HunterGate(
        URL "https://github.com/cpp-pm/hunter/archive/v0.23.304.tar.gz"
        SHA1 "cae9026e69d7d8333897663688a11f4232fb8826"
)

project(Server)
hunter_add_package(Boost COMPONENTS atomic chrono container context contract coroutine date_time exception fiber filesystem graph graph_parallel iostreams json locale log math mpi nowide program_options python random regex serialization stacktrace system test thread timer type_erasure wave)
find_package(Boost CONFIG REQUIRED date_time system thread filesystem regex)
find_package(Threads REQUIRED)
add_executable(Server main.cpp)
target_link_libraries (Server pthread)





