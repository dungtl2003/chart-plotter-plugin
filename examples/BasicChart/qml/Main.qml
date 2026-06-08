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
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/testrand.csv"
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
            strokeWidth: 40
            markerVisible: true
            markerRadius: 40
        }
        // LineSeries {
        //     source: dataSrc
        //     // x: "time"
        //     // y: "temperature"
        //     xColumn: 0
        //     yColumn: 2
        //     strokeColor: "#000000"
        // }
        // LineSeries {
        //     source: dataSrc
        //     // x: "time"
        //     // y: "temperature"
        //     xColumn: 0
        //     yColumn: 3
        //     strokeColor: "#00FFFF"
        // }
        // LineSeries {
        //     source: dataSrc
        //     // x: "time"
        //     // y: "temperature"
        //     xColumn: 0
        //     yColumn: 3
        //     strokeColor: "#00FFFF"
        // }
        // BarSeries {}

        name: "iloveyou"
        anchors.fill: parent
    }
}
