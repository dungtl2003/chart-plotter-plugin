import QtQuick
import QtQuick.Window
import QtQuick.Controls
import ChartPlotter as CP

Window {
    id: root

    width: 980
    height: 820
    visible: true
    title: qsTr("ChartPlotter Gallery")
    color: "#f4f4f7"

    readonly property string dataDir: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/"

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: 18
        contentWidth: width
        contentHeight: col.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
        }

        Column {
            id: col
            width: flick.width - 16 // leave room for the scrollbar
            spacing: 30

            Column {
                width: parent.width
                spacing: 6

                Text {
                    text: "ChartPlotter Demo Gallery"
                    font.pixelSize: 27
                    font.bold: true
                    color: "#1a1a1a"
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: "#666666"
                    font.pixelSize: 14
                    text: "Wheel-scroll or drag anywhere (including over a chart) to move the list. " + "Hold Ctrl over a chart to interact: Ctrl+wheel zooms, Ctrl+drag pans."
                }
            }

            // ---- 1. Line -------------------------------------------------
            ChartCard {
                heading: "1. Line"
                subtitle: "A single line series on a numeric x axis. Hover for the tooltip, Ctrl+wheel to zoom, Ctrl+drag to pan."

                CP.ChartView {
                    id: lineChart
                    anchors.fill: parent
                    zoomWheelRequiresModifier: true
                    panDragRequiresModifier: true

                    CP.DataSource {
                        id: lineSrc
                        url: root.dataDir + "trendrand.csv"
                        format: CP.ChartEnums.DataFormat.Csv
                        hasHeader: true
                    }
                    CP.LineSeries {
                        source: lineSrc
                        xColumn: 0
                        yColumn: 1
                        name: "Alpha"
                        strokeColor: "#3d7eff"
                    }

                    name: "Line"
                    generalConfig {
                        antialiasing: 2
                        lineWidth: 3
                    }
                    legendItem: CP.Legend {
                        model: lineChart.legendModel
                    }
                    legendPosition: CP.ChartEnums.LegendPosition.Bottom
                }
                CP.ChartTooltip {
                    chart: lineChart
                }
            }

            // ---- 2. Bar --------------------------------------------------
            ChartCard {
                heading: "2. Bar"
                subtitle: "A categorical bar chart. Bars auto-size to their band and rescale as you zoom."

                CP.ChartView {
                    id: barChart
                    anchors.fill: parent
                    zoomWheelRequiresModifier: true
                    panDragRequiresModifier: true

                    CP.DataSource {
                        id: barSrc
                        url: root.dataDir + "bars.csv"
                        format: CP.ChartEnums.DataFormat.Csv
                        hasHeader: true
                    }
                    CP.BarSeries {
                        source: barSrc
                        xColumn: 0
                        yColumn: 1
                        name: "Revenue"
                        color: "#3d7eff"
                    }

                    name: "Bar"
                    legendItem: CP.Legend {
                        model: barChart.legendModel
                    }
                    legendPosition: CP.ChartEnums.LegendPosition.Bottom
                }
            }

            // ---- 3. Pie --------------------------------------------------
            ChartCard {
                heading: "3. Pie"
                subtitle: "A single pie series (values normalised to fill the circle). The legend labels each slice; use the Settings button to recolor slices."

                CP.ChartView {
                    id: pieChart
                    anchors.fill: parent
                    zoomWheelRequiresModifier: true
                    panDragRequiresModifier: true

                    CP.DataSource {
                        id: pieSrc
                        url: root.dataDir + "pie.csv"
                        format: CP.ChartEnums.DataFormat.Csv
                        hasHeader: true
                    }
                    CP.PieSeries {
                        source: pieSrc
                        label: "browser"
                        value: "share"
                        name: "Browser share"
                    }

                    name: "Pie"
                    legendItem: CP.Legend {
                        model: pieChart.legendModel
                    }
                    legendPosition: CP.ChartEnums.LegendPosition.Right
                }
                CP.ChartSettings {
                    chart: pieChart
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                }
            }

            // ---- 4. Multiple lines --------------------------------------
            ChartCard {
                heading: "4. Multiple Lines"
                subtitle: "Several line series sharing one x axis, with dashed/dotted patterns and markers."

                CP.ChartView {
                    id: multiLineChart
                    anchors.fill: parent
                    zoomWheelRequiresModifier: true
                    panDragRequiresModifier: true

                    CP.DataSource {
                        id: multiSrc
                        url: root.dataDir + "trendrand.csv"
                        format: CP.ChartEnums.DataFormat.Csv
                        hasHeader: true
                    }
                    CP.LineSeries {
                        source: multiSrc
                        xColumn: 0
                        yColumn: 1
                        name: "Alpha"
                        strokePattern: CP.ChartEnums.StrokePattern.Dash
                        strokeColor: "#3d7eff"
                    }
                    CP.LineSeries {
                        source: multiSrc
                        xColumn: 0
                        yColumn: 2
                        name: "Beta"
                        strokeColor: "#111111"
                    }
                    CP.LineSeries {
                        source: multiSrc
                        xColumn: 0
                        yColumn: 3
                        name: "Gamma"
                        strokeColor: "#9a3d48"
                        strokePattern: CP.ChartEnums.StrokePattern.Dot
                    }
                    CP.LineSeries {
                        source: multiSrc
                        xColumn: 0
                        yColumn: 4
                        name: "Delta"
                        strokeColor: "#ea72b9"
                        markerColor: "#a55555"
                        markerVisible: true
                    }

                    name: "Multiple Lines"
                    title: multiLineChart.name
                    generalConfig {
                        antialiasing: 2
                        lineWidth: 3
                    }
                    titleItem: CP.ChartTitle {
                        text: multiLineChart.title
                    }
                    legendItem: CP.Legend {
                        model: multiLineChart.legendModel
                    }
                    legendPosition: CP.ChartEnums.LegendPosition.Bottom
                }
                CP.ChartTooltip {
                    chart: multiLineChart
                }
                CP.ChartSettings {
                    chart: multiLineChart
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                }
            }

            // ---- 5. Mixed bar + lines -----------------------------------
            ChartCard {
                heading: "5. Mixed Bar + Lines"
                subtitle: "One bar series with line series overlaid on a shared category x axis."

                CP.ChartView {
                    id: mixedChart
                    anchors.fill: parent
                    zoomWheelRequiresModifier: true
                    panDragRequiresModifier: true

                    CP.DataSource {
                        id: mixedSrc
                        url: root.dataDir + "mixed_data.csv"
                        format: CP.ChartEnums.DataFormat.Csv
                        hasHeader: true
                    }
                    CP.BarSeries {
                        source: mixedSrc
                        xColumn: 0
                        yColumn: 1
                        name: "Revenue"
                        color: "#bcd2ff"
                    }
                    CP.LineSeries {
                        source: mixedSrc
                        xColumn: 0
                        yColumn: 2
                        name: "Target"
                        strokeColor: "#d4453f"
                        strokePattern: CP.ChartEnums.StrokePattern.Dash
                        antialias: 2
                    }
                    CP.LineSeries {
                        source: mixedSrc
                        xColumn: 0
                        yColumn: 3
                        name: "Forecast"
                        strokeColor: "#1f9d55"
                        markerColor: "#147a40"
                        markerVisible: true
                        antialias: 2
                    }

                    name: "Revenue vs Target & Forecast"
                    title: mixedChart.name
                    titleItem: CP.ChartTitle {
                        text: mixedChart.title
                    }
                    legendItem: CP.Legend {
                        model: mixedChart.legendModel
                    }
                    legendPosition: CP.ChartEnums.LegendPosition.Bottom
                }
                CP.ChartTooltip {
                    chart: mixedChart
                }
                CP.ChartSettings {
                    chart: mixedChart
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                }
            }

            // ---- 6. Realtime — combined multi-source chart --------------
            ChartCard {
                heading: "6. Realtime — BTC price across Binance markets (live WebSocket)"
                subtitle: "Three live @trade streams for the SAME asset from different markets (BTC vs USDT, FDUSD, USDC). They share one chart and one y-scale; because the axis auto-scales to the data, the small cross-market price differences fan out into a live spread. Many updates/sec. Needs internet."
                chartHeight: 500

                CP.ChartView {
                    id: btcSpreadChart
                    anchors.fill: parent
                    zoomWheelRequiresModifier: true
                    panDragRequiresModifier: true

                    name: "BTC — cross-market spread"
                    title: btcSpreadChart.name
                    titleItem: CP.ChartTitle {
                        text: btcSpreadChart.title
                    }
                    legendItem: CP.Legend {
                        model: btcSpreadChart.legendModel
                    }
                    legendPosition: CP.ChartEnums.LegendPosition.Bottom
                    generalConfig {
                        antialiasing: 2
                        lineWidth: 2
                        maxCacheRows: 20000
                    }

                    CP.DataSource {
                        id: usdtSrc
                        url: "wss://stream.binance.com:9443/ws/btcusdt@trade"
                        format: CP.ChartEnums.DataFormat.BinanceTradeJson
                        hasHeader: false
                        columns: [
                            CP.Column {
                                idx: 1
                                type: CP.ChartEnums.DataType.Date
                            }
                        ]
                    }
                    CP.DataSource {
                        id: fdusdSrc
                        url: "wss://stream.binance.com:9443/ws/btcfdusd@trade"
                        format: CP.ChartEnums.DataFormat.BinanceTradeJson
                        hasHeader: false
                        columns: [
                            CP.Column {
                                idx: 1
                                type: CP.ChartEnums.DataType.Date
                            }
                        ]
                    }
                    CP.DataSource {
                        id: usdcSrc
                        url: "wss://stream.binance.com:9443/ws/btcusdc@trade"
                        format: CP.ChartEnums.DataFormat.BinanceTradeJson
                        hasHeader: false
                        columns: [
                            CP.Column {
                                idx: 1
                                type: CP.ChartEnums.DataType.Date
                            }
                        ]
                    }

                    // All three bind the same x column (index 1 = E), so they
                    // share the datetime axis; y = column 4 (p, trade price).
                    CP.LineSeries {
                        source: usdtSrc
                        xColumn: 1
                        yColumn: 4
                        name: "BTC/USDT"
                        strokeColor: "#f7931a"
                    }
                    CP.LineSeries {
                        source: fdusdSrc
                        xColumn: 1
                        yColumn: 4
                        name: "BTC/FDUSD"
                        strokeColor: "#627eea"
                    }
                    CP.LineSeries {
                        source: usdcSrc
                        xColumn: 1
                        yColumn: 4
                        name: "BTC/USDC"
                        strokeColor: "#14b88a"
                    }
                }
                CP.ChartTooltip {
                    chart: btcSpreadChart
                }
                CP.ChartSettings {
                    chart: btcSpreadChart
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                }
            }
        }
    }
}
