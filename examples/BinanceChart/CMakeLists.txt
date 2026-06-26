cmake_minimum_required(VERSION 3.20)
project(BinanceChartExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI OFF) # Maybe you want to turn this ON

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)
qt_standard_project_setup(REQUIRES 6.8)

if(NOT TARGET ChartPlotter)
    # Change this to GitHub Repository
    include(FetchContent)
    FetchContent_Declare(
        chartplotter_module
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    )
    FetchContent_MakeAvailable(chartplotter_module)
    
    set(PLUGIN_IMPORT_DIR "${chartplotter_module_BINARY_DIR}")
    message(STATUS "BinanceChart: Fetching plugin via FetchContent")
else()
    set(PLUGIN_IMPORT_DIR "${CMAKE_BINARY_DIR}")
    message(STATUS "BinanceChart: Using existing ChartPlotter target from root build")
endif()

qt_add_executable(BinanceChartExample src/main.cpp)

qt_add_qml_module(BinanceChartExample
    URI BinanceChart
    VERSION 1.0
    QML_FILES qml/Main.qml
)

target_link_libraries(BinanceChartExample PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)

target_compile_definitions(BinanceChartExample PRIVATE 
    PLUGIN_IMPORT_PATH=\"${PLUGIN_IMPORT_DIR}\"
)
