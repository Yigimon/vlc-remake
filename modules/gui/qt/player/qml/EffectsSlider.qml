/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts

import VLC.MainInterface
import VLC.Style
import VLC.Util

// Redesign: a setting of the effects panel, a label with the value and a
// pill slider below (see "Audio - Compressor Settings" and "Audio - Equalizer"
// in the design)
FocusScope {
    id: root

    property alias text: label.text

    property alias from: slider.from
    property alias to: slider.to
    property alias stepSize: slider.stepSize
    property alias value: slider.value

    // value set by a double click
    property real defaultValue: from

    // the value is shown multiplied by this factor, with the unit
    property real displayScale: 1.0
    property int decimals: 1
    property string unit: ""

    // shows the value in an editable field instead of a text
    property bool editable: false

    // whether the effect is in use: an inactive setting keeps a grey handle and no fill
    property bool active: true

    // number of intervals marked on the track, 0 for no marks
    property int tickCount: {
        if (stepSize <= 0)
            return 0
        const count = Math.round((to - from) / stepSize)
        return (count > 1 && count <= 20) ? count : 0
    }

    // text of the value, can be replaced (e.g. "Off" for a neutral value)
    property var textFunc: (value) => {
        return (value * root.displayScale).toFixed(root.decimals) + root.unit
    }

    // emitted when the user changes the value
    signal moved(real value)

    implicitWidth: layout.implicitWidth
    implicitHeight: layout.implicitHeight

    Accessible.role: Accessible.Slider
    Accessible.name: label.text

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View

        focused: slider.visualFocus
        hovered: slider.hovered
        enabled: root.enabled
    }

    readonly property ColorContext _sliderTheme: ColorContext {
        id: sliderTheme
        colorSet: ColorContext.Slider
    }

    readonly property ColorContext _windowTheme: ColorContext {
        id: windowTheme
        colorSet: ColorContext.Window
    }

    // Accent/Dim
    readonly property ColorContext _accentTheme: ColorContext {
        id: accentTheme
        colorSet: ColorContext.ButtonAccent
        pressed: true
    }

    function _setValue(value) {
        const v = Helpers.clamp(value, slider.from, slider.to)
        if (v === slider.value)
            return
        slider.value = v
        root.moved(slider.value)
    }

    ColumnLayout {
        id: layout

        anchors.fill: parent

        spacing: VLCStyle.margin_normal

        RowLayout {
            Layout.fillWidth: true

            spacing: VLCStyle.margin_xsmall

            Text {
                id: label

                Layout.fillWidth: true

                color: theme.fg.secondary
                font.pixelSize: VLCStyle.fontSize_normal
                elide: Text.ElideRight
            }

            Text {
                visible: !root.editable

                color: theme.fg.secondary
                font.pixelSize: VLCStyle.fontSize_normal
                text: root.textFunc(slider.value)
            }

            Rectangle {
                visible: root.editable

                Layout.preferredWidth: VLCStyle.dp(136, VLCStyle.scale)
                Layout.preferredHeight: VLCStyle.dp(32, VLCStyle.scale)

                radius: VLCStyle.dp(4, VLCStyle.scale)
                color: sliderTheme.bg.secondary
                border.width: VLCStyle.margin_xxxsmall
                border.color: valueInput.activeFocus ? sliderTheme.fg.primary : windowTheme.bg.secondary

                TextInput {
                    id: valueInput

                    anchors.fill: parent
                    anchors.leftMargin: VLCStyle.margin_small
                    anchors.rightMargin: VLCStyle.margin_small

                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                    selectByMouse: true

                    color: theme.fg.primary
                    selectionColor: sliderTheme.bg.highlight
                    font.pixelSize: VLCStyle.fontSize_small

                    validator: RegularExpressionValidator { regularExpression: /^\s*[-+]?\d*([.,]\d*)?\s*[^\d]*$/ }

                    Accessible.name: label.text

                    Binding on text {
                        when: !valueInput.activeFocus
                        value: root.textFunc(slider.value)
                        restoreMode: Binding.RestoreNone
                    }

                    function _apply() {
                        const match = text.replace(",", ".").match(/[-+]?\d*\.?\d+/)
                        if (match)
                            root._setValue(parseFloat(match[0]) / root.displayScale)
                        text = root.textFunc(slider.value)
                    }

                    onEditingFinished: _apply()

                    Keys.onEscapePressed: (event) => {
                        text = root.textFunc(slider.value)
                        slider.forceActiveFocus(Qt.OtherFocusReason)
                    }
                }
            }
        }

        T.Slider {
            id: slider

            Layout.fillWidth: true

            implicitWidth: VLCStyle.dp(200, VLCStyle.scale)
            implicitHeight: VLCStyle.dp(24, VLCStyle.scale)

            focus: true
            focusPolicy: Qt.StrongFocus
            wheelEnabled: false
            live: true
            snapMode: T.Slider.SnapAlways

            onMoved: root.moved(slider.value)

            Accessible.name: label.text

            Keys.priority: Keys.AfterItem
            Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

            readonly property real _trackHeight: VLCStyle.dp(16, VLCStyle.scale)
            readonly property real _inset: VLCStyle.margin_xxxsmall

            background: Rectangle {
                id: track

                x: slider.leftPadding
                y: slider.topPadding + (slider.availableHeight - height) / 2
                width: slider.availableWidth
                height: slider._trackHeight

                radius: height / 2
                color: sliderTheme.bg.secondary
                border.width: slider.visualFocus ? VLCStyle.focus_border : 0
                border.color: theme.visualFocus

                Rectangle {
                    id: fill

                    visible: root.active && width > 0

                    x: slider._inset
                    y: slider._inset
                    height: parent.height - 2 * slider._inset
                    width: Math.max(0, slider.handle.x - slider.leftPadding + slider.handle.width / 2 - slider._inset)

                    radius: height / 2
                    color: sliderTheme.bg.highlight
                }

                Repeater {
                    model: Math.max(0, root.tickCount - 1)

                    delegate: Rectangle {
                        required property int index

                        x: Math.round((index + 1) * track.width / root.tickCount - width / 2)
                        anchors.verticalCenter: parent.verticalCenter
                        width: VLCStyle.margin_xxxsmall
                        height: VLCStyle.margin_xsmall

                        color: Qt.alpha(theme.fg.primary, 0.1)
                    }
                }
            }

            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + (slider.availableHeight - height) / 2

                implicitWidth: VLCStyle.margin_xsmall
                implicitHeight: VLCStyle.dp(24, VLCStyle.scale)

                radius: VLCStyle.dp(3, VLCStyle.scale)
                color: root.active ? accentTheme.bg.primary : theme.fg.secondary
            }

            // a double click resets the value
            property double _lastPressTime: 0

            onPressedChanged: {
                if (!pressed)
                    return
                const now = Date.now()
                if (now - _lastPressTime < Qt.styleHints.mouseDoubleClickInterval) {
                    _lastPressTime = 0
                    Qt.callLater(root._setValue, root.defaultValue)
                } else {
                    _lastPressTime = now
                }
            }
        }
    }
}
