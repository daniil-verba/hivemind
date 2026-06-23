cmake_minimum_required(VERSION 3.10)
project(Hivemind VERSION 0.5.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/core/Hivemind.cpp
    src/network/Transport.cpp
    src/network/Stun.cpp
    src/storage/UserRegistry.cpp
    src/hivemind.cpp
)

add_library(hivemind SHARED ${SOURCES})

target_include_directories(hivemind PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(hivemind PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hivemind pthread)

install(TARGETS hivemind
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/hivemind
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)