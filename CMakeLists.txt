cmake_minimum_required(VERSION 3.10)
project(Hivemind)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Пути к исходникам
set(SOURCES
    src/core/Hivemind.cpp
    src/network/Transport.cpp
    src/hivemind.cpp
)

# Создаём shared библиотеку
add_library(hivemind SHARED ${SOURCES})

# Указываем где искать заголовки для компиляции библиотеки
target_include_directories(hivemind PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Публичные заголовки
target_include_directories(hivemind PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Линковка
if(UNIX)
    target_link_libraries(hivemind pthread)
endif()

# Установка
install(TARGETS hivemind
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
