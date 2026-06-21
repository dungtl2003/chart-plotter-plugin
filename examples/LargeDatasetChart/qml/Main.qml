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
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/large.csv"
            format: ChartEnums.DataFormat.Csv
            skipRows: 0
            hasHeader: true
        }

        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 1
            strokeColor: "#000000"
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
