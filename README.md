# ChartPlotter

A high-performance charting library shipped as a **Qt 6 QML plugin** (`libChartPlotter.so`).

A C++23 backend handles data management, layout, downsampling, and OpenGL rendering; QML is the frontend API. ChartPlotter is built to draw **million-point datasets** interactively, from both static (offline) files and live (streaming) sources, with SDF-based line antialiasing done in custom GLSL shaders.

## Features

- **Series types** — Line, Bar (categorical & numeric), and Pie.
- **Static & streaming data** — load a CSV file once, or stream continuously from a FIFO, socket, or WebSocket (e.g. live Binance ticks).
- **Millions of points, interactively** — an LOD min/max mip-pyramid gives O(visible pixels) zoom/pan, with LTTB / hybrid downsamplers as fallbacks.
- **Interactive** — mouse zoom/pan, per-point hover tooltip, legend, title, and an in-chart settings panel.
- **Custom GLSL rendering** — line strokes, markers, bars, pie slices, and axes drawn on OpenGL 3.3 Core; SDF shaders do self-written antialiasing.
- **Self-contained** — third-party deps (`spdlog`, `magic_enum`) are baked in header-only, so the shipped `.so` leaves no external library trace.

## Requirements

- Qt **6.8+** (Core, Gui, Qml, Quick, Concurrent, WebSockets)
- CMake **3.20+**
- Ninja
- A **C++23** compiler
- OpenGL **3.3 Core** capable GPU/driver

`spdlog` and `magic_enum` are fetched automatically via CMake `FetchContent` on first configure.

## Build

```bash
# Configure + build everything (plugin + all examples)
cmake -B build -G Ninja
cmake --build build

# Build a single target
cmake --build build --target BasicChartExample

# Run an example
./build/examples/BasicChart/BasicChartExample
```

Reconfigure whenever `CMakeLists.txt` changes (e.g. after adding a source, header, shader, or example). Toggle the examples with `-DBUILD_EXAMPLES=OFF`.

### Release / benchmarking

The default configure produces an **unoptimized** build (~10× slower). For any performance measurement, configure a Release tree:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j $(nproc)
CHARTPLOTTER_PLUGIN_LOG_LEVEL=info ./build/examples/LargeDatasetChart/LargeDatasetChartExample # or with no env
```

Reference: `LargeDatasetChart` ingests 10M rows (2 numeric columns, ~137 MB CSV) in ~1 s on a Release build.

### AddressSanitizer

```bash
cmake -B build-asan -G Ninja -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan
```

## Usage

Import `ChartPlotter` in QML, drop a `ChartView`, and declare `DataSource`s and series as children:

```qml
import QtQuick
import QtQuick.Window
import ChartPlotter

Window {
    width: 800; height: 600
    visible: true

    ChartView {
        id: chart
        anchors.fill: parent
        name: "Trend Sample"
        title: chart.name

        DataSource {
            id: dataSrc
            url: "file:///path/to/data.csv"
            format: ChartEnums.DataFormat.Csv
            hasHeader: true
        }

        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 1
            name: "Alpha"
            strokePattern: ChartEnums.StrokePattern.Dash
        }

        generalConfig {
            antialiasing: 2
            lineWidth: 3
        }
        titleItem: ChartTitle { text: chart.title }
        legendItem: Legend { model: chart.legendModel }
        legendPosition: ChartEnums.LegendPosition.Bottom
    }

    // Optional overlays
    ChartTooltip  { chart: chart }
    ChartSettings { chart: chart }
}
```

See the `examples/` directory for complete, runnable programs.

## Examples

| Example | What it shows |
| --- | --- |
| `BasicChart` | Multiple line series over a CSV |
| `ChartGallery` | A gallery of the supported chart types |
| `BarChart` | Categorical bar chart |
| `NumericBarChart` | Bars on a numeric / value x axis |
| `PieChart` | Pie series |
| `MixedChart` | One bar + multiple lines on a shared category axis |
| `CategoryLineChart` | Line series over string categories |
| `DateTimeLineChart` | Line series over a datetime x axis |
| `LargeDatasetChart` | 10M-row CSV ingest benchmark |
| `VeryLargeDatasetChart` | Stress test with an even larger dataset |
| `RealTimeChart` | Streaming / online data |
| `BinanceChart` | Live market data over WebSocket |

Each example uses `qt_add_qml_module` and loads `qrc:/qt/qml/<Uri>/qml/Main.qml`.

## Architecture

The data path flows one direction:

```
DataSource → DataManager(Pool) → snapshot → ChartView → Strategy → RenderData → Renderer → ChartRenderNode (scene graph)
```

- **Frontend** — `ChartView` is the central `QQuickItem`; it owns the data pool, viewport controller, strategies, layout plan, legend model, and the point cache, and builds the scene graph in `updatePaintNode()`.
- **Series / Strategy / Factory / Renderer** — four parallel hierarchies (in `series/`, `strategy/`, `factory/`, `renderer/`) that define each chart type. Adding a new series type means extending all four.
- **Data layer** — `reader/` (File, FIFO, Socket, WebSocket) supplies raw bytes; `parser/` interprets them. CSV uses the high-throughput `FastCsvDataParser` (byte-span batches → bulk columnar append).
- **Downsampling** — `LodPyramid` (default line path), plus LTTB / hybrid / stationary fallbacks.
- **Point cache** — `ChartView` keeps a `std::deque`-backed cache keyed by `(sourceId, xCol, yCol)`, so already-processed rows are never recomputed and sliding-window eviction is O(evicted).
- **Shaders** — GLSL in `shaders/`, grouped by `line/`, `bar/`, `pie/`, `circle/`, and `axis/`; SDF variants do the antialiasing.

## License

MIT — see [`LICENSE.md`](LICENSE.md).
