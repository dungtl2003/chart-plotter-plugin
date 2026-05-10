import QtQuick
import ChartPlotter

Item {
    id: root
    width: 200
    height: 100

    ChartModel {
        id: backend
    }

    Rectangle {
        anchors.fill: parent
        color: "#1a1a1a"
        border.color: "cyan"

        Text {
            anchors.centerIn: parent
            text: "Status: " + backend.provider
            color: "white"
        }
    }
}
