import QtQuick
import QtQuick.Window
import ChartPlotter

Window {
    width: 800
    height: 600
    visible: true
    title: qsTr("Bar Chart Example")
    color: "white"

    ChartView {
        id: chart

        DataSource {
            id: dataSrc
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/bars.csv"
            format: ChartEnums.DataFormat.Csv
            skipRows: 0
            hasHeader: true
        }

        BarSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 1
            name: "Revenue"
            color: "#3d7eff"
        }

        anchors.fill: parent
        name: "Monthly Revenue"
        title: chart.name

        titleItem: ChartTitle {
            text: chart.title
        }
        legendItem: Legend {
            model: chart.legendModel
        }
        legendPosition: ChartEnums.LegendPosition.Bottom
    }

    ChartSettings {
        chart: chart
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
    }
}
