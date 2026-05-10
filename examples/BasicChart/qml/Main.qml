import QtQuick
import QtQuick.Window
import ChartPlotter

Window {
    width: 800
    height: 600
    visible: true
    title: qsTr("Chart Plotter Example")
    color: "#1e1e2e"

    ChartView {
        anchors.centerIn: parent
        width: 600
        height: 400

        // dataFile: "sample_data.csv"
        // chartColor: "#89b4fa"
    }
}
