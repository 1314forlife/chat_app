cmake_minimum_required(VERSION 3.16)

project(chat_client VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# 明确指定源码目录
set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR})

set(PROJECT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/mainwindow.cpp
    ${CMAKE_SOURCE_DIR}/src/mainwindow.h
)

qt_add_executable(chat_client ${PROJECT_SOURCES})

target_link_libraries(chat_client PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)