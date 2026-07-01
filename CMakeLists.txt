cmake_minimum_required(VERSION 3.20)

project("ChartPlotter" VERSION 0.1 LANGUAGES CXX)

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

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(ENABLE_ASAN)
    add_compile_options(
        -fsanitize=address
        -fno-omit-frame-pointer
        -g
    )

    add_link_options(
        -fsanitize=address
    )
endif()

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

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Qml 
    Quick 
    Concurrent 
    WebSockets
)

qt_standard_project_setup(REQUIRES 6.8)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_compile_options(
        -g
        -O0
        -fno-omit-frame-pointer
    )
endif()

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0002 NEW)
qt_policy(SET QTP0003 NEW)
qt_policy(SET QTP0004 NEW)

################################################################################

set(CHARTPLOTTER_QMLS
    qml/ChartPlotter/Legend.qml
    qml/ChartPlotter/ChartTitle.qml
    qml/ChartPlotter/ChartSettings.qml
    qml/ChartPlotter/ChartZoomButton.qml
    qml/ChartPlotter/ChartTooltip.qml
)

set(CHARTPLOTTER_HEADERS
    include/ChartPlotter/ChartView.hpp
    include/ChartPlotter/ChartLayoutPlanner.hpp
    include/ChartPlotter/ViewportController.hpp

    include/ChartPlotter/legend/LegendModel.hpp

    include/ChartPlotter/series/AbstractSeries.hpp
    include/ChartPlotter/series/XYSeries.hpp
    include/ChartPlotter/series/LineSeries.hpp
    include/ChartPlotter/series/BarSeries.hpp
    include/ChartPlotter/series/PieSeries.hpp
    include/ChartPlotter/series/ResolvedSeriesData.hpp
    include/ChartPlotter/series/SeriesDataResolver.hpp

    include/ChartPlotter/strategy/ISeriesStrategy.hpp
    include/ChartPlotter/strategy/LineSeriesStrategy.hpp
    include/ChartPlotter/strategy/BarSeriesStrategy.hpp

    include/ChartPlotter/renderer/IOpenGLRenderer.hpp
    include/ChartPlotter/renderer/OpenGLLineRenderer.hpp
    include/ChartPlotter/renderer/OpenGLBarRenderer.hpp
    include/ChartPlotter/renderer/OpenGLAxisRenderer.hpp

    include/ChartPlotter/factory/ISeriesComponentFactory.hpp
    include/ChartPlotter/factory/LineSeriesComponentFactory.hpp
    include/ChartPlotter/factory/BarSeriesComponentFactory.hpp
    include/ChartPlotter/factory/SeriesComponentFactoryProvider.hpp

    include/ChartPlotter/data/RenderData.hpp
    include/ChartPlotter/data/LineRenderData.hpp
    include/ChartPlotter/data/BarRenderData.hpp
    include/ChartPlotter/data/AxisRenderData.hpp
    include/ChartPlotter/data/DataSource.hpp
    include/ChartPlotter/data/Column.hpp
    include/ChartPlotter/data/DataRow.hpp
    include/ChartPlotter/data/DataBuffer.hpp
    include/ChartPlotter/data/DataBufferUpdater.hpp
    include/ChartPlotter/data/DataManager.hpp
    include/ChartPlotter/data/DataManagerPool.hpp
    include/ChartPlotter/data/DataReadConfig.hpp

    include/ChartPlotter/data/reader/AbstractDataReader.hpp
    include/ChartPlotter/data/reader/FileDataReader.hpp
    include/ChartPlotter/data/reader/FifoDataReader.hpp
    include/ChartPlotter/data/reader/SocketDataReader.hpp
    include/ChartPlotter/data/reader/WebSocketDataReader.hpp
    include/ChartPlotter/data/reader/DataReaderFactory.hpp

    include/ChartPlotter/data/parser/AbstractDataParser.hpp
    include/ChartPlotter/data/parser/CsvDataParser.hpp
    include/ChartPlotter/data/parser/FastCsvDataParser.hpp
    include/ChartPlotter/data/parser/BinanceBasicJsonDataParser.hpp
    include/ChartPlotter/data/parser/DataParserFactory.hpp
    include/ChartPlotter/data/CsvRowBatch.hpp

    include/ChartPlotter/axis/Axis.hpp
    include/ChartPlotter/axis/AxisBuilder.hpp
    include/ChartPlotter/axis/ValueAxis.hpp
    include/ChartPlotter/axis/CategoryAxis.hpp

    include/ChartPlotter/utils/LoggerManager.hpp
    include/ChartPlotter/utils/Gl.hpp
    include/ChartPlotter/utils/RenderMath.hpp
    include/ChartPlotter/utils/RenderMath.tpp
    include/ChartPlotter/utils/Variant.hpp
    include/ChartPlotter/utils/DataRangeCalculator.hpp

    include/ChartPlotter/downsample/DataDownsampler.hpp
    include/ChartPlotter/downsample/LargestTriangleThreeBuckets.hpp
    include/ChartPlotter/downsample/HybridDownsampler.hpp
    include/ChartPlotter/downsample/StationaryMinMaxDownsampler.hpp
    include/ChartPlotter/downsample/LodPyramid.hpp

    include/ChartPlotter/types/ChartEnums.hpp
    include/ChartPlotter/types/DataRange.hpp

    include/ChartPlotter/constants/ChartConstants.hpp
)

set(CHARTPLOTTER_SOURCES
    src/ChartView.cpp
    src/ChartLayoutPlanner.cpp
    src/ViewportController.cpp

    src/legend/LegendModel.cpp

    src/series/AbstractSeries.cpp
    src/series/XYSeries.cpp
    src/series/LineSeries.cpp
    src/series/BarSeries.cpp
    src/series/PieSeries.cpp
    src/series/SeriesDataResolver.cpp

    src/strategy/LineSeriesStrategy.cpp
    src/strategy/BarSeriesStrategy.cpp

    src/renderer/OpenGLLineRenderer.cpp
    src/renderer/OpenGLBarRenderer.cpp
    src/renderer/OpenGLAxisRenderer.cpp

    src/factory/LineSeriesComponentFactory.cpp
    src/factory/BarSeriesComponentFactory.cpp
    src/factory/SeriesComponentFactoryProvider.cpp

    src/data/DataSource.cpp
    src/data/Column.cpp
    src/data/DataRow.cpp
    src/data/DataBuffer.cpp
    src/data/DataBufferUpdater.cpp
    src/data/DataManager.cpp
    src/data/DataManagerPool.cpp

    src/data/reader/AbstractDataReader.cpp
    src/data/reader/FileDataReader.cpp
    src/data/reader/FifoDataReader.cpp
    src/data/reader/SocketDataReader.cpp
    src/data/reader/WebSocketDataReader.cpp
    src/data/reader/DataReaderFactory.cpp

    src/data/parser/AbstractDataParser.cpp
    src/data/parser/CsvDataParser.cpp
    src/data/parser/FastCsvDataParser.cpp
    src/data/parser/BinanceBasicJsonDataParser.cpp
    src/data/parser/DataParserFactory.cpp

    src/axis/AxisBuilder.cpp
    src/axis/ValueAxis.cpp
    src/axis/CategoryAxis.cpp

    src/node/ChartRenderNode.cpp

    src/utils/ChartPlotterStartup.cpp
    src/utils/LoggerManager.cpp
    src/utils/Gl.cpp
    src/utils/RenderMath.cpp
    src/utils/Variant.cpp
    src/utils/DataRangeCalculator.cpp

    src/downsample/LargestTriangleThreeBuckets.cpp
    src/downsample/HybridDownsampler.cpp
    src/downsample/StationaryMinMaxDownsampler.cpp
    src/downsample/LodPyramid.cpp
)

set(CHARTPLOTTER_SHADERS
    shaders/line/stroke.vert
    shaders/line/stroke.frag
    shaders/line/marker.vert
    shaders/line/marker.frag
    shaders/bar/bar.vert
    shaders/bar/bar.frag
    shaders/circle/circle.vert
    shaders/circle/circle.frag
    shaders/circle/circle-sdf.vert
    shaders/circle/circle-sdf.frag
    shaders/axis/line.vert
    shaders/axis/line.frag
    shaders/axis/text.vert
    shaders/axis/text.frag
)

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}

    QML_FILES
        ${CHARTPLOTTER_QMLS}

    SOURCES
        ${CHARTPLOTTER_HEADERS}
        ${CHARTPLOTTER_SOURCES}

    RESOURCES
        ${CHARTPLOTTER_SHADERS}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include

    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ChartPlotter
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ChartPlotter/constants
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ChartPlotter/chart
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ChartPlotter/legend
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ChartPlotter/series
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ChartPlotter/data
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ChartPlotter/data/reader
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ChartPlotter/data/parser
)


# Qt libraries
# If you don't use header only version, it will create spdlog.so file, and may conflict with other apps that use this plugin (don't worry, cmake does not allow to build it anyway)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Concurrent
    Qt6::WebSockets
    spdlog::spdlog_header_only # the library will be baked directly in the plugin, leave no trace that this plugin has any libraries
    magic_enum::magic_enum # this is fine because it only has header
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
        add_subdirectory(examples/BarChart)
        add_subdirectory(examples/NumericBarChart)
        add_subdirectory(examples/MixedChart)
        add_subdirectory(examples/BinanceChart)
        add_subdirectory(examples/LargeDatasetChart)
        add_subdirectory(examples/VeryLargeDatasetChart)
        add_subdirectory(examples/RealTimeChart)
        add_subdirectory(examples/CategoryLineChart)
        add_subdirectory(examples/DateTimeLineChart)
    endif()
endif()
