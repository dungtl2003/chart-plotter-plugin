import QtQuick
import QtQuick.Window
import ChartPlotter

Window {
    width: 800
    height: 600
    visible: true
    title: qsTr("Numeric Bar Chart Example")
    color: "white"

    ChartView {
        id: chart

        // Numeric x axis (a histogram): the x column holds numbers, not category
        // strings. The bar width auto-derives from the smallest gap between
        // neighbouring x values (here 10), so the bars sit on a real value axis
        // and rescale as you zoom.
        DataSource {
            id: dataSrc
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/numeric_bars.csv"
            format: ChartEnums.DataFormat.Csv
            skipRows: 0
            hasHeader: true
        }

        BarSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 1
            name: "Count"
            color: "#1f9d55"
        }

        anchors.fill: parent
        name: "Score Distribution"
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
