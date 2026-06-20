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
        name: "I Love You"            // logger id (unchanged)

        title: "Trend Sample"
        legendPosition: ChartEnums.LegendPosition.Bottom
        // title: ""
        // legendPosition: ChartEnums.LegendPosition.None

        titleItem: ChartTitle {
            text: chart.title
        }
        legendItem: Legend {
            model: chart.legendModel
        }

        DataSource {
            id: dataSrc
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/trendrand.csv"
            format: ChartEnums.DataFormat.Csv
            skipRows: 0
            hasHeader: true
        }

        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 1
            name: "Alpha"
            strokePattern: ChartEnums.StrokePattern.Dash
            antialias: 2
        }
        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 2
            name: "Beta"
            strokeColor: "#000000"
            antialias: 2
        }
        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 3
            name: "Gamma"
            strokeColor: "#9a3d48"
            strokePattern: ChartEnums.StrokePattern.Dot
            antialias: 2
        }
        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 4
            name: "Delta"
            strokeColor: "#ea72b9"
            markerColor: "#a55555"
            markerVisible: true
            antialias: 2
        }
    }

    ChartSettings {
        chart: chart
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
    }
}
