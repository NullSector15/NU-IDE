cmake_minimum_required(VERSION 3.16)

# ---------------------------
# Project Definition
# ---------------------------
project(NU-IDE VERSION 0.1 LANGUAGES CXX)

# ---------------------------
# Qt Auto Settings
# ---------------------------
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------
# C++ Standard
# ---------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------
# Find Qt Packages
# ---------------------------
# Detect Qt6 first, fallback to Qt5 if needed
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets SerialPort Network)

# ---------------------------
# Project Sources
# ---------------------------
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    highlighting.h
)

# ---------------------------
# Executable / Library Setup
# ---------------------------
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(NU-IDE
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(NU-IDE SHARED ${PROJECT_SOURCES})
    else()
        add_executable(NU-IDE ${PROJECT_SOURCES})
    endif()
endif()

# ---------------------------
# Link Qt Libraries
# ---------------------------
target_link_libraries(NU-IDE
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::SerialPort
        Qt${QT_VERSION_MAJOR}::Network
)

# ---------------------------
# Cross-Platform Properties
# ---------------------------
# macOS bundle setup for older Qt versions
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.NU-IDE)
endif()

set_target_properties(NU-IDE PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ---------------------------
# Installation
# ---------------------------
include(GNUInstallDirs)
install(TARGETS NU-IDE
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---------------------------
# Finalize Qt Executable for Qt6
# ---------------------------
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(NU-IDE)
endif()