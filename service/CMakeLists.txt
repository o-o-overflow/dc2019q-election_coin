cmake_minimum_required(VERSION 3.10)
project(dc2219q_election_coin)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_RELEASE "-flto -fsanitize=cfi -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/cfi_blacklist -fvisibility=hidden")

add_subdirectory(pistache)

add_executable(election_coin main.cpp Election.h Configuration.h Exchange.h Vote.h Ballot.h Candidate.h)
target_include_directories(election_coin PUBLIC
        ${CMAKE_SOURCE_DIR}/spdlog/include
        ${CMAKE_SOURCE_DIR}/json/include
        ${CMAKE_SOURCE_DIR}/pistache/include)
target_compile_options(election_coin PUBLIC "-fstack-protector-strong")
target_link_libraries(election_coin PUBLIC pistache pthread "-Wl,-z,now")
