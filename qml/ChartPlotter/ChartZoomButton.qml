pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Button {
    id: zoomButton

    property var chart: null

    text: "Reset Zoom"

    onClicked: {
        chart.resetZoom();
    }
}
