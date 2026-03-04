cmake_minimum_required(VERSION 3.22)
project(Phase7_Dashboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Qt6 ───────────────────────────────────────────────────────────────────────
set(CMAKE_PREFIX_PATH "/home/abdo/Qt/6.10.2/gcc_64" CACHE STRING "Qt6 prefix path")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# ── Project root (for includes like inc/Facade/...) ───────────────────────────
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
include_directories(
    ${PROJECT_ROOT}
    ${PROJECT_ROOT}/inc/logging
    ${PROJECT_ROOT}/inc/SmartDataHub
    ${PROJECT_ROOT}/inc/Formatter
    ${PROJECT_ROOT}/inc/AsyncLogging
    ${PROJECT_ROOT}/inc/Facade
)

# ── Phase 7 Source files ──────────────────────────────────────────────────────
set(PHASE7_SOURCES
    main.cpp
    GaugeWidget.cpp
    GaugeWidget.hpp
    DashboardWindow.cpp
    DashboardWindow.hpp
    TelemetryBridge.cpp
    TelemetryBridge.hpp
)

# ── Core library sources (from existing project) ──────────────────────────────
set(CORE_SOURCES
    # Logging
    ${PROJECT_ROOT}/src/logging/ConsoleSinkImpl.cpp
    ${PROJECT_ROOT}/src/logging/FileSinkImpl.cpp
    ${PROJECT_ROOT}/src/logging/LogManager.cpp

    # SmartDataHub
    ${PROJECT_ROOT}/src/SmartDataHub/FileTelemetrySourceImpl.cpp
    ${PROJECT_ROOT}/src/SmartDataHub/TcpTelemetrySourceImpl.cpp
    ${PROJECT_ROOT}/src/SmartDataHub/SomeIPTelemetrySourceImpl.cpp
    ${PROJECT_ROOT}/src/SmartDataHub/SomeIPTelemetryClient.cpp
    ${PROJECT_ROOT}/src/SmartDataHub/ITelemetrySource.cpp
    ${PROJECT_ROOT}/src/SmartDataHub/SafeFile.cpp
    ${PROJECT_ROOT}/src/SmartDataHub/SafeSocket.cpp
    ${PROJECT_ROOT}/src/SmartDataHub/TelemetryParser.cpp

    # Formatter
    ${PROJECT_ROOT}/src/Formatter/LogSinkFactory.cpp
    ${PROJECT_ROOT}/src/Formatter/LogManagerBuilder.cpp

    # AsyncLogging
    ${PROJECT_ROOT}/src/AsyncLogging/AsyncLogManager.cpp
    ${PROJECT_ROOT}/src/AsyncLogging/ThreadPool.cpp

    # Facade
    ${PROJECT_ROOT}/src/Facade/TelemetryApp.cpp
    ${PROJECT_ROOT}/src/Facade/AppConfig.cpp
)

# ── Executable ────────────────────────────────────────────────────────────────
qt_add_executable(phase7_dashboard
    ${PHASE7_SOURCES}
    ${CORE_SOURCES}
)

# ── vsomeip3 ──────────────────────────────────────────────────────────────────
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(VSOMEIP QUIET vsomeip3)
endif()
if(NOT VSOMEIP_FOUND)
    set(VSOMEIP_INCLUDE_DIRS "/usr/local/include")
    set(VSOMEIP_LIBRARY_DIRS "/usr/local/lib")
    set(VSOMEIP_LIBRARIES "vsomeip3")
endif()

target_include_directories(phase7_dashboard PRIVATE ${VSOMEIP_INCLUDE_DIRS})
target_link_directories(phase7_dashboard PRIVATE ${VSOMEIP_LIBRARY_DIRS})

target_link_libraries(phase7_dashboard PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${VSOMEIP_LIBRARIES}
)

# ── Copy config.json next to executable ───────────────────────────────────────
add_custom_command(TARGET phase7_dashboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:phase7_dashboard>/config.json"
    COMMENT "Copying phase7 config.json to build directory"
)

message(STATUS "Phase 7 Dashboard: Qt ${Qt6_VERSION} found at ${Qt6_DIR}")
