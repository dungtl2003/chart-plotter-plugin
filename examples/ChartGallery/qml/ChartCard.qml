import QtQuick

// A titled container for one demo chart. The heading band above the framed
// chart doubles as a "gutter": a wheel scroll there moves the list, while a
// wheel scroll over the chart itself is captured by the chart (zoom). Declare
// the ChartView (and optional ChartTooltip) as direct children — they are
// reparented into the framed area, so `anchors.fill: parent` fills the frame.
Column {
    id: root

    property string heading: ""
    property string subtitle: ""
    property real chartHeight: 460

    default property alias content: frame.data

    width: parent ? parent.width : 600
    spacing: 8

    Column {
        width: parent.width
        spacing: 2

        Text {
            text: root.heading
            font.pixelSize: 20
            font.bold: true
            color: "#222222"
        }
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: root.subtitle
            font.pixelSize: 13
            color: "#777777"
            visible: text.length > 0
        }
    }

    Rectangle {
        id: frame
        width: parent.width
        height: root.chartHeight
        color: "white"
        radius: 8
        border.color: "#e0e0e5"
        border.width: 1
        clip: true
    }
}
