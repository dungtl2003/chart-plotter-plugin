import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import ChartPlotter

Window {
    width: 800
    height: 600
    visible: true
    title: qsTr("Chart Plotter Example")
    color: "white"

    ChartView {
        id: chart

        DataSource {
            id: dataSrc
            url: "pipe:///tmp/pipe1"
            format: ChartEnums.DataFormat.Csv
            // totalColumns: 2
            // columns: [
            //     Column {
            //         idx: 0
            //         name: "time"
            //         type: ChartEnums.DataType.Number
            //     },
            //     Column {
            //         idx: 1
            //         name: "temperature"
            //         type: ChartEnums.DataType.Number
            //     }
            // ]
        }
        LineSeries {
            source: dataSrc
            x: "time"
            y: "temperature"
            xColumn: 0
            yColumn: 1
        }

        name: "Real-time Data Chart"
        title: chart.name
        anchors.fill: parent

        titleItem: ChartTitle {
            text: chart.title
        }
        legendPosition: ChartEnums.LegendPosition.Bottom
        legendItem: Legend {
            model: chart.legendModel
        }
        generalConfig {
            antialiasing: 1
            lineWidth: 3
            fps: 144
            pointSpacingPx: 10
        }
    }

    RowLayout {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12

        ChartZoomButton {
            chart: chart
        }

        ChartSettings {
            chart: chart
        }
    }
}
