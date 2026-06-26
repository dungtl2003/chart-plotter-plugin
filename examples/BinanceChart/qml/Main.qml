import QtQuick
import QtQuick.Window
import ChartPlotter

Window {
    width: 800
    height: 600
    visible: true
    title: qsTr("Chart Plotter Example")
    color: "white"

    ChartView {
        id: chart
        anchors.fill: parent
        name: "Binance"

        title: chart.name
        legendPosition: ChartEnums.LegendPosition.Bottom

        titleItem: ChartTitle {
            text: chart.title
        }
        legendItem: Legend {
            model: chart.legendModel
        }

        DataSource {
            id: dataSrc
            url: "wss://stream.binance.com:9443/ws/btcusdt@kline_1s"
            format: ChartEnums.DataFormat.BinanceBasicJson
            totalColumns: 19
            columns: [
                Column {
                    idx: 1 // E
                    type: ChartEnums.DataType.Date
                },
                Column {
                    idx: 8 // o
                    type: ChartEnums.DataType.Number
                },
                Column {
                    idx: 9 // c
                    type: ChartEnums.DataType.Number
                },
                Column {
                    idx: 10 // h
                    type: ChartEnums.DataType.Number
                },
                Column {
                    idx: 11 // l
                    type: ChartEnums.DataType.Number
                },
                Column {
                    idx: 12 // v
                    type: ChartEnums.DataType.Number
                },
                Column {
                    idx: 16 // V
                    type: ChartEnums.DataType.Number
                }
            ]
        }

        LineSeries {
            source: dataSrc
            xColumn: 1
            yColumn: 16
            name: "Buy Volume"
            strokeColor: "#089981"
            strokeWidth: 3
            antialias: 2
            useGlobalStrokeWidth: false
            useGlobalAntialias: false
        }
        LineSeries {
            source: dataSrc
            xColumn: 1
            yColumn: 12
            name: "Total Volume"
            strokeColor: "#B2B5BE"
            strokeWidth: 2
            antialias: 2
            useGlobalStrokeWidth: false
            useGlobalAntialias: false
        }

        // LineSeries {
        //     source: dataSrc
        //     xColumn: 1
        //     yColumn: 9
        //     name: "Close Price"
        //     strokeColor: "#2962FF"
        //     antialias: 2
        // }
        // LineSeries {
        //     source: dataSrc
        //     xColumn: 1
        //     yColumn: 8
        //     name: "Open Price"
        //     strokeColor: "#B2B5BE"
        //     strokePattern: ChartEnums.StrokePattern.Dash
        //     antialias: 2
        // }
        // LineSeries {
        //     source: dataSrc
        //     xColumn: 1
        //     yColumn: 10
        //     name: "High Price"
        //     strokeColor: "#089981"
        //     strokePattern: ChartEnums.StrokePattern.Dash
        //     antialias: 2
        // }
        // LineSeries {
        //     source: dataSrc
        //     xColumn: 1
        //     yColumn: 11
        //     name: "Low Price"
        //     strokeColor: "#F23645"
        //     strokePattern: ChartEnums.StrokePattern.Dash
        //     antialias: 2
        // }
    }

    ChartSettings {
        chart: chart
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
    }
}
