pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ChartPlotter

Button {
    id: root

    property var chart: null

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

    // Pie slices (label + editable color swatch). Populated only for a pie chart.
    ListModel {
        id: pieSlicesModel
    }

    function loadDraft() {
        if (!chart)
            return;
        gWidth.value = chart.generalConfig.lineWidth;
        gAa.value = chart.generalConfig.antialiasing;
        gXTicks.value = chart.generalConfig.xPreferredTickCount;
        gYTicks.value = chart.generalConfig.yPreferredTickCount;
        gFps.value = chart.generalConfig.fps;
        // <= 0 means unlimited: leave the limit unchecked and the box at its
        // floor; checking it switches to a finite cap.
        const cap = chart.generalConfig.maxCacheRows;
        gLimitRows.checked = cap > 0;
        gMaxRows.value = cap > 0 ? cap : gMaxRows.from;

        lines.clear();
        pieSlicesModel.clear();
        const list = chart.seriesList;
        for (let i = 0; i < list.length; ++i) {
            const s = list[i];

            // Pie has no per-series stroke/marker settings — its slices are
            // edited in a dedicated section below, so skip it here.
            if (s.seriesType === ChartEnums.SeriesType.Pie)
                continue;

            const title = (s.name && s.name.length) ? s.name : ("Series " + (i + 1));

            // Unified row — keep every role present so ListModel roles stay stable.
            const row = {
                kind: "line",
                title: title,
                swatch: "#000000",
                useGlobalWidth: true,
                width: 3,
                useGlobalAa: true,
                aa: 1,
                marker: "#ff3333",
                showMarker: false,
                pattern: ChartEnums.StrokePattern.Solid
            };

            if (s.seriesType === ChartEnums.SeriesType.Bar) {
                // Bars auto-size to their band — only the color is editable.
                row.kind = "bar";
                row.swatch = "" + s.color;
            } else {
                row.kind = "line";
                row.swatch = "" + s.strokeColor;
                row.useGlobalWidth = s.useGlobalStrokeWidth;
                row.width = s.strokeWidth;
                row.useGlobalAa = s.useGlobalAntialias;
                row.aa = s.antialias;
                row.marker = "" + s.markerColor;
                row.showMarker = s.markerVisible;
                row.pattern = s.strokePattern;
            }

            lines.append(row);
        }

        // Pie slices come from the chart (label + current color).
        const slices = chart.pieSlices;
        for (let j = 0; j < slices.length; ++j) {
            pieSlicesModel.append({
                label: slices[j].label,
                swatch: "" + slices[j].color
            });
        }
    }

    function applyDraft() {
        if (!chart)
            return;
        const list = chart.seriesList;
        for (let i = 0; i < list.length && i < lines.count; ++i) {
            const d = lines.get(i), s = list[i];
            if (d.kind === "bar") {
                s.color = d.swatch;
            } else {
                s.useGlobalStrokeWidth = d.useGlobalWidth;
                s.strokeColor = d.swatch;
                s.strokeWidth = d.width;
                s.useGlobalAntialias = d.useGlobalAa;
                s.antialias = d.aa;
                s.markerColor = d.marker;
                s.markerVisible = d.showMarker;
                s.strokePattern = d.pattern;
            }
        }
        // Pie slice colors → the pie series' `colors` list (index-aligned to the
        // slices). applySettings() below triggers the rebuild that repaints them.
        if (pieSlicesModel.count > 0) {
            const cols = [];
            for (let j = 0; j < pieSlicesModel.count; ++j)
                cols.push(pieSlicesModel.get(j).swatch);
            for (let k = 0; k < list.length; ++k) {
                if (list[k].seriesType === ChartEnums.SeriesType.Pie) {
                    list[k].colors = cols;
                    break;
                }
            }
        }

        // global + one rebuild. Unchecked "limit" => 0 (unlimited). The downsample
        // mode is no longer user-editable here, so pass through its current value.
        const maxRows = gLimitRows.checked ? gMaxRows.value : 0;
        chart.applySettings(gWidth.value, gAa.value, gXTicks.value, gYTicks.value, gFps.value, chart.generalConfig.downsampleMode, maxRows);
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
            Layout.preferredWidth: 95
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
                title: "Global — applies to every series"
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    RowLayout {
                        spacing: 10
                        Label {
                            text: "FPS"
                            Layout.preferredWidth: 95
                        }
                        SpinBox {
                            id: gFps
                            Layout.fillWidth: true
                            editable: true
                            from: ChartConstants.FPS_MIN
                            to: ChartConstants.FPS_MAX
                            stepSize: 1
                            // Only accept integers within [FPS_MIN, FPS_MAX]; the
                            // SpinBox itself clamps out-of-range typed values.
                            validator: IntValidator {
                                bottom: gFps.from
                                top: gFps.to
                            }
                        }
                    }
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
                    SliderRow {
                        id: gXTicks
                        label: "Desired X-Ticks"
                        from: ChartConstants.TICK_COUNT_MIN
                        to: ChartConstants.TICK_COUNT_MAX
                        stepSize: 1
                    }
                    SliderRow {
                        id: gYTicks
                        label: "Desired Y-Ticks"
                        from: ChartConstants.TICK_COUNT_MIN
                        to: ChartConstants.TICK_COUNT_MAX
                        stepSize: 1
                    }
                    RowLayout {
                        spacing: 10
                        CheckBox {
                            id: gLimitRows
                            text: "Limit cache rows"
                            Layout.preferredWidth: 150
                            // Unchecked = unlimited (oldest rows never evicted).
                        }
                        SpinBox {
                            id: gMaxRows
                            Layout.fillWidth: true
                            enabled: gLimitRows.checked
                            editable: true
                            from: 100000
                            to: 1000000000
                            stepSize: 100000
                            validator: IntValidator {
                                bottom: gMaxRows.from
                                top: gMaxRows.to
                            }
                            textFromValue: (value, locale) => Number(value).toLocaleString(locale, 'f', 0)
                            valueFromText: (text, locale) => Number(text.replace(/[^0-9]/g, "")) || gMaxRows.from
                        }
                    }
                    // Cache-rows changes are not free: switching unlimited -> limited,
                    // or moving the limit by a large amount, forces the cache to
                    // evict (and the affected series to rebuild) in bulk. Mid-load
                    // that shows up as a brief stall. Warn before the user does it.
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        color: "#b06a00"
                        text: "⚠ Change this carefully. Switching from unlimited to "
                            + "limited, or moving the limit by a large amount while "
                            + "data is streaming, can briefly stall the chart as the "
                            + "cache re-evicts in bulk. Prefer setting it before "
                            + "loading data."
                    }
                }
            }

            // ---- Pie: per-slice colors (shown only for a pie chart) ----
            GroupBox {
                title: "Pie slices — click a swatch to recolor"
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: pieSlicesModel.count > 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    Repeater {
                        model: pieSlicesModel

                        delegate: RowLayout {
                            id: sliceRow
                            required property var model
                            required property int index
                            Layout.fillWidth: true
                            spacing: 10

                            Label {
                                text: sliceRow.model.label
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Rectangle {
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 22
                                radius: 4
                                color: sliceRow.model.swatch
                                border.color: "#cfcfd6"
                                border.width: 1
                                TapHandler {
                                    onTapped: {
                                        pieColorDlg.slice = sliceRow.index;
                                        pieColorDlg.selectedColor = sliceRow.model.swatch;
                                        pieColorDlg.open();
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }

            Label {
                text: "Per series"
                font.bold: true
                Layout.topMargin: 4
                visible: pieSlicesModel.count === 0
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 10
                visible: pieSlicesModel.count === 0
                model: lines

                delegate: Frame {
                    id: card

                    required property var model
                    required property int index
                    readonly property int line: index   // capture before inner scopes shadow `index`
                    readonly property bool isBar: model.kind === "bar"
                    readonly property int patternIdx: Math.max(0, root.patternValues.indexOf(model.pattern))

                    width: ListView.view.width

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Rectangle {
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: card.isBar ? 14 : 4
                                radius: 2
                                color: card.model.swatch
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Label {
                                text: card.model.title + (card.isBar ? "  (Bar)" : "")
                                font.bold: true
                                Layout.fillWidth: true
                            }
                        }

                        // ---- line-only: stroke width (bars auto-size to band) ----
                        CheckBox {
                            visible: !card.isBar
                            text: "Stroke width — follow global"
                            checked: card.model.useGlobalWidth
                            onToggled: lines.setProperty(card.line, "useGlobalWidth", checked)
                        }
                        SliderRow {
                            visible: !card.isBar && !card.model.useGlobalWidth
                            label: "Width"
                            from: ChartConstants.LINE_STROKE_WIDTH_MIN
                            to: ChartConstants.LINE_STROKE_WIDTH_MAX
                            stepSize: 1
                            value: card.model.width
                            onMoved: lines.setProperty(card.line, "width", value)
                        }

                        // ---- line-only: antialiasing ----
                        CheckBox {
                            visible: !card.isBar
                            text: "Antialiasing — follow global"
                            checked: card.model.useGlobalAa
                            onToggled: lines.setProperty(card.line, "useGlobalAa", checked)
                        }
                        SliderRow {
                            visible: !card.isBar && !card.model.useGlobalAa
                            label: "AA"
                            from: ChartConstants.LINE_AA_MIN
                            to: ChartConstants.LINE_AA_MAX
                            stepSize: 1
                            value: card.model.aa
                            onMoved: lines.setProperty(card.line, "aa", value)
                        }

                        // ---- line-only: markers ----
                        CheckBox {
                            id: markerCheckbox
                            visible: !card.isBar
                            text: "Show markers"
                            checked: card.model.showMarker
                            onToggled: lines.setProperty(card.line, "showMarker", checked)
                        }
                        RowLayout {
                            visible: !card.isBar
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
                                        if (!markerCheckbox.checked) {
                                            return;
                                        }

                                        markerColorDlg.line = card.line;
                                        markerColorDlg.selectedColor = card.model.marker;
                                        markerColorDlg.open();
                                    }
                                }
                            }
                        }

                        // ---- color (line or bar) ----
                        RowLayout {
                            Label {
                                text: card.isBar ? "Bar color" : "Line color"
                                Layout.fillWidth: true
                            }
                            Rectangle {
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 22
                                radius: 4
                                color: card.model.swatch
                                border.color: "#cfcfd6"
                                border.width: 1
                                TapHandler {
                                    onTapped: {
                                        swatchColorDlg.line = card.line;
                                        swatchColorDlg.selectedColor = card.model.swatch;
                                        swatchColorDlg.open();
                                    }
                                }
                            }
                        }

                        // ---- line-only: line style ----
                        RowLayout {
                            visible: !card.isBar
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
            id: markerColorDlg
            property int line: -1
            onAccepted: if (line >= 0)
                lines.setProperty(line, "marker", "" + selectedColor)
        }

        ColorDialog {
            id: swatchColorDlg
            property int line: -1
            onAccepted: if (line >= 0)
                lines.setProperty(line, "swatch", "" + selectedColor)
        }

        ColorDialog {
            id: pieColorDlg
            property int slice: -1
            onAccepted: if (slice >= 0)
                pieSlicesModel.setProperty(slice, "swatch", "" + selectedColor)
        }
    }
}
