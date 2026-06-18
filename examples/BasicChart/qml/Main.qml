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
        DataSource {
            id: dataSrc
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/trendrand.csv"
            format: ChartEnums.DataFormat.Csv
            skipRows: 0
            hasHeader: true
        }
        // DataSource {
        //     id: dataSrc
        //     url: "pipe:///tmp/chartplotter.csvpipe"
        //     format: ChartEnums.DataFormat.Csv
        //     totalColumns: 4
        //     columns: [
        //         Column {
        //             idx: 0
        //             name: "time"
        //             type: ChartEnums.DataType.Number
        //         },
        //         Column {
        //             idx: 1
        //             name: "temperature"
        //             type: ChartEnums.DataType.Number
        //         },
        //         Column {
        //             idx: 2
        //             type: ChartEnums.DataType.Number
        //         },
        //         Column {
        //             idx: 3
        //             type: ChartEnums.DataType.Number
        //         }
        //     ]
        // }
        LineSeries {
            source: dataSrc
            // x: "time"
            // y: "temperature"
            xColumn: 0
            yColumn: 1
            strokePattern: ChartEnums.StrokePattern.Dash
            antialias: 2
        }
        LineSeries {
            source: dataSrc
            // x: "time"
            // y: "temperature"
            xColumn: 0
            yColumn: 2
            strokeColor: "#000000"
            antialias: 2
            // markerVisible: true
        }
        LineSeries {
            source: dataSrc
            // x: "time"
            // y: "temperature"
            xColumn: 0
            yColumn: 3
            strokeColor: "#9a3d48"
            strokePattern: ChartEnums.StrokePattern.Dot
            antialias: 2
        }
        LineSeries {
            source: dataSrc
            // x: "time"
            // y: "temperature"
            xColumn: 0
            yColumn: 4
            strokeColor: "#ea72b9"
            markerColor: "#a55555"
            markerVisible: true
            antialias: 2
        }
        // BarSeries {}

        name: "I Love You"
        anchors.fill: parent
    }
}
