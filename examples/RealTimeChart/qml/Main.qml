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
            id: dataSrc1
            url: "pipe:///tmp/pipe1"
            format: ChartEnums.DataFormat.Csv
            totalColumns: 2
            columns: [
                Column {
                    idx: 0
                    name: "time"
                    type: ChartEnums.DataType.Number
                },
                Column {
                    idx: 1
                    name: "temperature"
                    type: ChartEnums.DataType.Number
                }
            ]
        }
        DataSource {
            id: dataSrc2
            url: "pipe:///tmp/pipe2"
            format: ChartEnums.DataFormat.Csv
            totalColumns: 2
            columns: [
                Column {
                    idx: 0
                    name: "time"
                    type: ChartEnums.DataType.Number
                },
                Column {
                    idx: 1
                    name: "temperature"
                    type: ChartEnums.DataType.Number
                }
            ]
        }
        LineSeries {
            source: dataSrc1
            x: "time"
            y: "temperature"
            xColumn: 0
            yColumn: 1
            strokeWidth: 2
            strokeMiterLimit: 1
            antialias: 1
        }
        LineSeries {
            source: dataSrc2
            x: "time"
            y: "temperature"
            xColumn: 0
            yColumn: 1
            strokeColor: "#000000"
            strokeWidth: 2
            // markerVisible: true
        }

        name: "iloveyou"
        anchors.fill: parent
    }
}
