# ==============================================
# Hivemind Protocol — CMakeLists.txt
# ==============================================
cmake_minimum_required(VERSION 3.10)

project(Hivemind VERSION 0.6.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================
# 3RD PARTY: miniupnpc (только для beacon server на VPS)
# ==============================================

set(MINIUPNPC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniupnpc)

set(MINIUPNPC_SOURCES
    ${MINIUPNPC_DIR}/src/miniupnpc.c
    ${MINIUPNPC_DIR}/src/upnpcommands.c
    ${MINIUPNPC_DIR}/src/upnpreplyparse.c
    ${MINIUPNPC_DIR}/src/miniwget.c
    ${MINIUPNPC_DIR}/src/minisoap.c
    ${MINIUPNPC_DIR}/src/minissdpc.c
    ${MINIUPNPC_DIR}/src/upnperrors.c
    ${MINIUPNPC_DIR}/src/connecthostport.c
    ${MINIUPNPC_DIR}/src/portlistingparse.c
    ${MINIUPNPC_DIR}/src/receivedata.c
)

add_library(miniupnpc STATIC ${MINIUPNPC_SOURCES})

target_include_directories(miniupnpc PUBLIC
    ${MINIUPNPC_DIR}/include
    ${MINIUPNPC_DIR}/src
)

if(WIN32)
    target_link_libraries(miniupnpc ws2_32 iphlpapi)
endif()

# ==============================================
# ОСНОВНАЯ БИБЛИОТЕКА HIVEMIND (shared)
# ==============================================

set(HIVEMIND_SOURCES
    src/core/Hivemind.cpp
    src/network/Transport.cpp
    src/network/Stun.cpp
    src/network/PacketManager.cpp
    src/network/BeaconClient.cpp
    src/storage/SimpleRegistry.cpp
    src/utils/Logger.cpp
    src/utils/Timer.cpp
    src/hivemind.cpp
)

set(HIVEMIND_HEADERS
    include/hivemind/hivemind.h
    include/hivemind/Hivemind.hpp
    include/hivemind/SimpleRegistry.hpp
    include/hivemind/PacketManager.hpp
    include/hivemind/BeaconClient.hpp
    include/hivemind/api.h
    include/hivemind/callbacks.h
    include/hivemind/types.h
    include/hivemind/version.h
)

add_library(hivemind SHARED ${HIVEMIND_SOURCES})

target_include_directories(hivemind PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${MINIUPNPC_DIR}/include
    ${MINIUPNPC_DIR}/src
)

target_include_directories(hivemind PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hivemind
    pthread
)

if(WIN32)
    target_link_libraries(hivemind ws2_32 iphlpapi)
endif()

# ==============================================
# BEACON SERVER (standalone, для VPS)
# ==============================================

add_executable(hivemind_beacon src/network/BeaconServer.cpp)

target_include_directories(hivemind_beacon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(hivemind_beacon pthread)

if(WIN32)
    target_link_libraries(hivemind_beacon ws2_32 iphlpapi)
endif()

# ==============================================
# УСТАНОВКА
# ==============================================

install(TARGETS hivemind hivemind_beacon miniupnpc
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/hivemind
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)