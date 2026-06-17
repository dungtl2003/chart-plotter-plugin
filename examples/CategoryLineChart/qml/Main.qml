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
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/category_data.csv"
            format: ChartEnums.DataFormat.Csv
            skipRows: 0
            hasHeader: true
        }
        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 1
            strokePattern: ChartEnums.StrokePattern.Dash
            antialias: 2
        }
        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 2
            strokeColor: "#000000"
            antialias: 2
        }
        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 3
            strokeColor: "#9a3d48"
            strokePattern: ChartEnums.StrokePattern.Dot
            antialias: 2
        }
        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 4
            strokeColor: "#ea72b9"
            markerColor: "#a55555"
            markerVisible: true
            antialias: 2
        }

        name: "category_line_chart"
        anchors.fill: parent
    }
}
