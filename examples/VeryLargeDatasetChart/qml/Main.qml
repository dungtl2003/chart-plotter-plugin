pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQml
import ChartPlotter

Window {
    width: 800
    height: 600
    minimumWidth: 320
    minimumHeight: 240
    visible: true
    title: qsTr("Large Dataset Example (5GB file)")
    color: "white"

    ChartView {
        id: chart

        DataSource {
            id: dataSrc
            url: "file:///home/ilikeblue/personal/projects/chart-plotter-plugin/temp/very_large_dataset_5GB.csv"
            format: ChartEnums.DataFormat.Csv
            skipRows: 0

            columns: [
                Column {
                    idx: 0
                    name: "time"
                    type: ChartEnums.DataType.Date
                }
            ]
        }

        Instantiator {
            model: 20

            // Instantiator delegates are parented to the Instantiator, not added
            // to ChartView's `content`, so register/unregister them explicitly.
            onObjectAdded: (index, object) => chart.addSeries(object)
            onObjectRemoved: (index, object) => chart.removeSeries(object)

            delegate: LineSeries {

                required property int index

                source: dataSrc
                xColumn: 0
                yColumn: index + 1

                strokeColor: {
                    var palette = ["#E6194B" // Red
                        , "#3CB44B" // Green
                        , "#4363D8" // Blue
                        , "#F58231" // Orange
                        , "#911EB4" // Purple
                        , "#42D4F4" // Cyan
                        , "#F032E6" // Magenta
                        , "#008080" // Teal
                        , "#9A6324" // Brown
                        , "#800000" // Maroon
                        , "#808000" // Olive
                        , "#000075" // Navy
                        , "#696969" // Dim Gray
                        , "#000000" // Black
                        , "#9932CC" // Dark Orchid
                        , "#8B4513" // Saddle Brown
                        , "#2E8B57" // Sea Green
                        , "#D2691E" // Chocolate
                        , "#FF1493" // Deep Pink
                        , "#4B0082"  // Indigo
                    ];

                    return palette[index % palette.length];
                }
            }
        }

        anchors.fill: parent
        name: "Trend Sample"
        title: chart.name
        legendPosition: ChartEnums.LegendPosition.Bottom

        generalConfig {
            lineWidth: 2
            antialiasing: 2
            fps: 60
            maxCacheRows: 10000000
        }
        titleItem: ChartTitle {
            text: chart.title
        }
        legendItem: Legend {
            model: chart.legendModel
        }
    }

    ChartSettings {
        chart: chart
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
    }

    ChartTooltip {
        chart: chart
    }
}
