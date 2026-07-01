pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var model: null
    property bool horizontal: false

    // Natural (unclipped) content size. relayout() reserves space along the
    // legend's *main* axis from these — the width for a Left/Right legend, the
    // height for a Top/Bottom one — and caps the cross axis to what's available.
    // When the series list overflows that cross axis, the Flickable scrolls.
    readonly property real pad: 14
    implicitWidth: layout.implicitWidth + pad * 2
    implicitHeight: layout.implicitHeight + pad * 2

    Rectangle {
        id: card
        anchors.fill: parent
        radius: 12
        color: "#ffffff"
    }

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: root.pad
        clip: true

        // Scroll along the axis the legend grows on: horizontal legends (Top/
        // Bottom) flick sideways, vertical legends (Left/Right) flick up/down.
        // Keep the content area at least the viewport size so a short legend
        // doesn't become scrollable — it just gets centered (see layout.x/y).
        flickableDirection: root.horizontal ? Flickable.HorizontalFlick : Flickable.VerticalFlick
        contentWidth: root.horizontal ? Math.max(width, layout.implicitWidth) : width
        contentHeight: root.horizontal ? height : Math.max(height, layout.implicitHeight)
        boundsBehavior: Flickable.StopAtBounds

        GridLayout {
            id: layout
            // Single row when horizontal (scrolls right), single column when
            // vertical (scrolls down).
            flow: GridLayout.LeftToRight
            columns: root.horizontal ? Math.max(rep.count, 1) : 1
            rowSpacing: 6
            columnSpacing: 16

            // Center the items in the viewport. On the scroll axis, clamp the
            // offset to >= 0 so once the legend overflows it pins to the start
            // and scrolls from the beginning instead of being pushed off.
            x: root.horizontal
               ? Math.max(0, (flick.width - implicitWidth) / 2)
               : (flick.width - implicitWidth) / 2
            y: root.horizontal
               ? (flick.height - implicitHeight) / 2
               : Math.max(0, (flick.height - implicitHeight) / 2)

            Repeater {
                id: rep
                model: root.model

                delegate: Rectangle {
                    id: delegateRect

                    required property var model
                    required property int index

                    radius: 6
                    color: hover.hovered ? "#f2f2f5" : "transparent"
                    implicitWidth: row.implicitWidth + 12
                    implicitHeight: row.implicitHeight + 8
                    opacity: delegateRect.model.visible ? 1.0 : 0.35
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 120
                        }
                    }

                    Row {
                        id: row
                        anchors.centerIn: parent
                        spacing: 8

                        Rectangle {
                            width: 22
                            height: 4
                            radius: 2
                            color: delegateRect.model.color
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: delegateRect.model.name
                            color: "#2a2a32"
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    HoverHandler {
                        id: hover
                    }
                    TapHandler {
                        onTapped: root.model.toggleSeries(delegateRect.index)
                    }
                }
            }
        }

        // Only the active scroll axis shows an indicator.
        ScrollBar.vertical: ScrollBar {
            policy: !root.horizontal && flick.contentHeight > flick.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }
        ScrollBar.horizontal: ScrollBar {
            policy: root.horizontal && flick.contentWidth > flick.width ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        }
    }
}
