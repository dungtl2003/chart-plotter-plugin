import QtQuick
import QtQuick.Window
import ChartPlotter

Window {
    width: 800
    height: 600
    visible: true
    title: qsTr("Chart Plotter Example")
    color: "white"

    ChartView {
        LineChart {}

        name: "iloveyou"
        color: "red"
        implicitWidth: 400
        implicitHeight: 400
        anchors.centerIn: parent
    }

    ChartView {
        BarChart {}

        name: "ihateyou"
        color: "blue"
        implicitWidth: 100
        implicitHeight: 100
        anchors.centerIn: parent
    }
}
