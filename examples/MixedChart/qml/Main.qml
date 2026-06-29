import QtQuick
import QtQuick.Window
import ChartPlotter

Window {
    width: 800
    height: 600
    visible: true
    title: qsTr("Mixed Bar + Line Chart Example")
    color: "white"

    ChartView {
        id: chart

        // Mixed chart: one Bar series (revenue) with several Line series (target,
        // forecast) overlaid on a shared category x axis and numeric y axis.
        // Only ONE bar series is allowed, but any number of line series can be
        // mixed in. Declare the bar first so it renders behind the lines.
        DataSource {
            id: dataSrc
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/mixed_data.csv"
            format: ChartEnums.DataFormat.Csv
            skipRows: 0
            hasHeader: true
        }

        BarSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 1
            name: "Revenue"
            color: "#bcd2ff"
        }

        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 2
            name: "Target"
            strokeColor: "#d4453f"
            strokePattern: ChartEnums.StrokePattern.Dash
            antialias: 2
        }

        LineSeries {
            source: dataSrc
            xColumn: 0
            yColumn: 3
            name: "Forecast"
            strokeColor: "#1f9d55"
            markerColor: "#147a40"
            markerVisible: true
            antialias: 2
        }

        anchors.fill: parent
        name: "Revenue vs Target & Forecast"
        title: chart.name

        titleItem: ChartTitle {
            text: chart.title
        }
        legendItem: Legend {
            model: chart.legendModel
        }
        legendPosition: ChartEnums.LegendPosition.Bottom
    }

    ChartTooltip {
        chart: chart
    }

    ChartSettings {
        chart: chart
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
    }
}
