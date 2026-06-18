import QtQuick

Item {
    id: root
    property alias text: label.text
    property alias color: label.color
    property alias font: label.font

    implicitHeight: label.implicitHeight

    Text {
        id: label
        anchors.centerIn: parent
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        font.pixelSize: 20
        font.bold: true
        color: "#1a1a1a"
    }
}
