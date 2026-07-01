import QtQuick
import ChartPlotter as CP

// Drop-in hover tooltip for a ChartView. Usage (as a sibling of the chart):
//
//     ChartView { id: chart; ... }
//     ChartTooltip { chart: chart }
//
// It overlays the chart, highlights the data point under the mouse, and shows a
// small box with the series name and the point's x/y values. All positioning is
// driven by `chart.hoveredPoint`, which the C++ side updates on hover.
Item {
    id: root

    property CP.ChartView chart: null
    readonly property var info: chart ? chart.hoveredPoint : null
    readonly property bool active: info ? info.active === true : false

    anchors.fill: chart
    visible: active
    z: 50

    // Mouse events must keep reaching the chart underneath.
    enabled: false

    // ---- highlight marker on the hovered point ----
    Rectangle {
        id: marker
        visible: root.active
        width: 14
        height: 14
        radius: width / 2
        color: "transparent"
        border.width: 2
        border.color: root.active ? root.info.color : "transparent"
        x: (root.active ? root.info.x : 0) - width / 2
        y: (root.active ? root.info.y : 0) - height / 2

        Rectangle {
            anchors.centerIn: parent
            width: 6
            height: 6
            radius: width / 2
            color: root.active ? root.info.color : "transparent"
        }
    }

    // ---- info box, placed near the point and clamped inside the chart ----
    Rectangle {
        id: box
        visible: root.active
        radius: 6
        color: "#f8f8fb"
        border.color: "#d3d3da"
        border.width: 1
        width: content.implicitWidth + 20
        height: content.implicitHeight + 14

        // Prefer upper-right of the point; flip / clamp near edges.
        readonly property real px: root.active ? root.info.x : 0
        readonly property real py: root.active ? root.info.y : 0
        x: Math.max(0, Math.min(px + 16, root.width - width))
        y: Math.max(0, Math.min(py - height - 16, root.height - height))

        Column {
            id: content
            anchors.centerIn: parent
            spacing: 3

            Row {
                spacing: 6
                Rectangle {
                    width: 11
                    height: 11
                    radius: 2
                    color: root.active ? root.info.color : "transparent"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: root.active ? root.info.seriesName : ""
                    font.bold: true
                    color: "#1c1c22"
                }
            }
            Text {
                text: root.active ? (root.info.xLabel + " ,  " + root.info.yLabel) : ""
                color: "#444"
                font.pixelSize: 12
            }
        }
    }
}
