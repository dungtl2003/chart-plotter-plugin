pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ChartPlotter

Button {
    id: root

    property var chart: null        // assign your ChartView

    text: "Settings"
    onClicked: {
        loadDraft();
        win.show();
        win.raise();
        win.requestActivate();
    }

    // line-style enum in display order — named constants, so the int values don't matter
    readonly property var patternValues: [ChartEnums.StrokePattern.Solid, ChartEnums.StrokePattern.Dash, ChartEnums.StrokePattern.Dot]
    readonly property var patternLabels: ["Solid", "Dash", "Dot"]

    // ---------- draft (live chart untouched until Apply) ----------
    ListModel {
        id: lines
    }

    function loadDraft() {
        if (!chart)
            return;
        gWidth.value = chart.generalConfig.lineWidth;
        gAa.value = chart.generalConfig.antialiasing;

        lines.clear();
        const list = chart.seriesList;
        for (let i = 0; i < list.length; ++i) {
            const s = list[i];
            lines.append({
                title: (s.name && s.name.length) ? s.name : ("Series " + (i + 1)),
                swatch: "" + s.strokeColor,
                useGlobalWidth: s.useGlobalStrokeWidth,
                width: s.strokeWidth,
                useGlobalAa: s.useGlobalAntialias,
                aa: s.antialias,
                marker: "" + s.markerColor,
                showMarker: s.markerVisible,
                pattern: s.strokePattern
            });
        }
    }

    function applyDraft() {
        if (!chart)
            return;
        const list = chart.seriesList;
        for (let i = 0; i < list.length && i < lines.count; ++i) {
            const d = lines.get(i), s = list[i];
            s.useGlobalStrokeWidth = d.useGlobalWidth;
            s.strokeWidth = d.width;
            s.useGlobalAntialias = d.useGlobalAa;
            s.antialias = d.aa;
            s.markerColor = d.marker;
            s.markerVisible = d.showMarker;
            s.strokePattern = d.pattern;
        }
        chart.applySettings(gWidth.value, gAa.value);   // global + one rebuild
    }

    component SliderRow: RowLayout {
        id: sr
        property string label
        property alias from: sld.from
        property alias to: sld.to
        property alias stepSize: sld.stepSize
        property alias value: sld.value
        signal moved
        spacing: 10
        Label {
            text: sr.label
            Layout.preferredWidth: 92
        }
        Slider {
            id: sld
            Layout.fillWidth: true
            snapMode: Slider.SnapAlways
            onMoved: sr.moved()
        }
        Label {
            Layout.preferredWidth: 24
            horizontalAlignment: Text.AlignRight
            text: sld.value.toFixed(0)
        }
    }

    Window {
        id: win
        title: "Chart settings"
        color: "#fbfbfc"
        width: 470
        height: 640
        minimumWidth: 400
        minimumHeight: 420

        // ---- Wayland: float as a dialog, don't get tiled ----
        flags: Qt.Dialog
        transientParent: root.Window.window
        modality: Qt.NonModal

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 14

            GroupBox {
                title: "Global — applies to every line"
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6
                    SliderRow {
                        id: gWidth
                        label: "Stroke width"
                        from: ChartConstants.LINE_STROKE_WIDTH_MIN
                        to: ChartConstants.LINE_STROKE_WIDTH_MAX
                        stepSize: 1
                    }
                    SliderRow {
                        id: gAa
                        label: "Antialiasing"
                        from: ChartConstants.LINE_AA_MIN
                        to: ChartConstants.LINE_AA_MAX
                        stepSize: 1
                    }
                }
            }

            Label {
                text: "Per line"
                font.bold: true
                Layout.topMargin: 4
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 10
                model: lines

                delegate: Frame {
                    id: card

                    required property var model
                    required property int index
                    readonly property int line: index   // capture before inner scopes shadow `index`
                    readonly property int patternIdx: Math.max(0, root.patternValues.indexOf(model.pattern))

                    width: ListView.view.width

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Rectangle {
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 4
                                radius: 2
                                color: card.model.swatch
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Label {
                                text: card.model.title
                                font.bold: true
                                Layout.fillWidth: true
                            }
                        }

                        CheckBox {
                            text: "Stroke width — follow global"
                            checked: card.model.useGlobalWidth
                            onToggled: lines.setProperty(card.line, "useGlobalWidth", checked)
                        }
                        SliderRow {
                            visible: !card.model.useGlobalWidth
                            label: "Width"
                            from: 1
                            to: 10
                            stepSize: 1
                            value: card.model.width
                            onMoved: lines.setProperty(card.line, "width", value)
                        }

                        CheckBox {
                            text: "Antialiasing — follow global"
                            checked: card.model.useGlobalAa
                            onToggled: lines.setProperty(card.line, "useGlobalAa", checked)
                        }
                        SliderRow {
                            visible: !card.model.useGlobalAa
                            label: "AA"
                            from: 0
                            to: 4
                            stepSize: 1
                            value: card.model.aa
                            onMoved: lines.setProperty(card.line, "aa", value)
                        }

                        // ---- markers ----
                        CheckBox {
                            text: "Show markers"
                            checked: card.model.showMarker
                            onToggled: lines.setProperty(card.line, "showMarker", checked)
                        }
                        RowLayout {
                            opacity: card.model.showMarker ? 1.0 : 0.45
                            Label {
                                text: "Marker color"
                                Layout.fillWidth: true
                            }
                            Rectangle {
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 22
                                radius: 4
                                color: card.model.marker
                                border.color: "#cfcfd6"
                                border.width: 1
                                TapHandler {
                                    onTapped: {
                                        colorDlg.line = card.line;
                                        colorDlg.selectedColor = card.model.marker;
                                        colorDlg.open();
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "Line style"
                                Layout.fillWidth: true
                            }
                            ComboBox {
                                model: root.patternLabels
                                currentIndex: card.patternIdx
                                onActivated: lines.setProperty(card.line, "pattern", root.patternValues[currentIndex])
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: "Cancel"
                    onClicked: win.close()
                }
                Button {
                    text: "Apply"
                    highlighted: true
                    onClicked: {
                        root.applyDraft();
                        win.close();
                    }
                }
            }
        }

        ColorDialog {
            id: colorDlg
            property int line: -1
            onAccepted: if (line >= 0)
                lines.setProperty(line, "marker", "" + selectedColor)
        }
    }
}
