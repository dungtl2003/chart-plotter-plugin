pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property var model: null
    property bool horizontal: false

    implicitWidth: card.width
    implicitHeight: card.height

    Rectangle {
        id: card
        anchors.centerIn: parent

        width: layout.implicitWidth + 28
        height: layout.implicitHeight + 20

        radius: 12
        color: "#ffffff"
        border.width: 0
        // border.color: "#e6e6ec"
        // border.width: 1

        GridLayout {
            id: layout
            anchors.centerIn: parent

            flow: GridLayout.LeftToRight
            columns: root.horizontal ? Math.max(rep.count, 1) : 1
            rowSpacing: 6
            columnSpacing: 16

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
    }
}
