import QtQuick
import QtQuick.Window
import ChartPlotter

Window {
    width: 800
    height: 600
    visible: true
    title: qsTr("Pie Chart Example")
    color: "white"

    ChartView {
        id: chart

        DataSource {
            id: dataSrc
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/pie.csv"
            format: ChartEnums.DataFormat.Csv
            skipRows: 0
            hasHeader: true
        }

        PieSeries {
            source: dataSrc
            label: "browser"
            value: "share"
            name: "Browser share"
        }

        anchors.fill: parent
        name: "Browser Market Share"
        title: chart.name

        titleItem: ChartTitle {
            text: chart.title
        }
        legendItem: Legend {
            model: chart.legendModel
        }
        legendPosition: ChartEnums.LegendPosition.Right
    }
}
