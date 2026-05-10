cmake_minimum_required(VERSION 3.20)

project("ChartPlotter" VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ORG_DOMAIN "qt.lyndun.me")
set(ORG_NAME "LynDun")
set(REV_DOMAIN "me.lyndun.qt")
set(FETCHCONTENT_QUIET FALSE)
set(QT_QML_GENERATE_QMLLS_INI ON)

################################################################################

message(STATUS "[CMAKE]   CMAKE_VERSION             = ${CMAKE_VERSION}")
message(STATUS "[CMAKE]   CMAKE_PREFIX_PATH         = ${CMAKE_PREFIX_PATH}")
message(STATUS "[CMAKE]   CMAKE_INSTALL_PREFIX      = ${CMAKE_INSTALL_PREFIX}")
message(STATUS "[SYSTEM]  (HOST) SYSTEM NAME        = ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "[SYSTEM]  (HOST) SYSTEM VERSION     = ${CMAKE_HOST_SYSTEM_VERSION}")
message(STATUS "[SYSTEM]  (HOST) SYSTEM PROCESSOR   = ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "[SYSTEM]  (TARGET) SYSTEM NAME      = ${CMAKE_SYSTEM_NAME}")
message(STATUS "[SYSTEM]  (TARGET) SYSTEM VERSION   = ${CMAKE_SYSTEM_VERSION}")
message(STATUS "[SYSTEM]  (TARGET) SYSTEM PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "[PROJECT] CURRENT_SOURCE_DIR        = ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "[PROJECT] SOURCE_DIR                = ${CMAKE_SOURCE_DIR}")
message(STATUS "[PROJECT] BUILD_TYPE                = ${CMAKE_BUILD_TYPE}")
message(STATUS "[PROJECT] TOOLCHAIN_FILE            = ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "[PROJECT] QT_HOST_PATH              = ${QT_HOST_PATH}")

################################################################################

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    ON   
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum
    GIT_TAG        v0.9.8
    GIT_SHALLOW    ON   
)
FetchContent_MakeAvailable(magic_enum)

################################################################################

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0002 NEW)
qt_policy(SET QTP0003 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
    SOURCES
        src/backend/ChartModel.hpp src/backend/ChartModel.cpp
    QML_FILES
        qml/ChartPlotter/ChartView.qml
)

# Debug only
# target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -g -fno-omit-frame-pointer)
# target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)

################################################################################

# Qt libraries
# If you don't use header only version, it will create spdlog.so file, and may conflict with other apps that use this plugin (don't worry, cmake does not allow to build it anyway)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    spdlog::spdlog_header_only # the library will be baked directly in the plugin, leave no trace that this plugin has any libraries
    magic_enum::magic_enum # this is fine because it only has header
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
)

################################################################################

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PLUGIN_NAME="${PROJECT_NAME}"
    PLUGIN_VERSION="${PROJECT_VERSION}"
    ORG_NAME="${ORG_NAME}"
    ORG_DOMAIN="${ORG_DOMAIN}"
)

## Release build? Set "no debug" macros
if(CMAKE_BUILD_TYPE STREQUAL Release)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        NDEBUG
        QT_NO_DEBUG
        QT_NO_DEBUG_OUTPUT
    )
endif()

################################################################################

include(GNUInstallDirs)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/qml/ChartPlotter
)

################################################################################

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Option to turn examples on/off, defaults to ON
    option(BUILD_EXAMPLES "Build the example applications" ON)
    
    if(BUILD_EXAMPLES)
        add_subdirectory(examples/BasicChart)
    endif()
endif()
