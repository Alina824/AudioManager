cmake_minimum_required(VERSION 3.16)

project(AudioPlayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia MultimediaWidgets Sql)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/audioplayer.cpp
    src/databasemanager.cpp
    src/playlistmodel.cpp
)

set(HEADERS
    src/mainwindow.h
    src/audioplayer.h
    src/databasemanager.h
    src/playlistmodel.h
)

add_executable(AudioPlayer ${SOURCES} ${HEADERS})

target_link_libraries(AudioPlayer
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Sql
)

target_include_directories(AudioPlayer PRIVATE
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6Multimedia_INCLUDE_DIRS}
    ${Qt6MultimediaWidgets_INCLUDE_DIRS}
    ${Qt6Sql_INCLUDE_DIRS}
)
