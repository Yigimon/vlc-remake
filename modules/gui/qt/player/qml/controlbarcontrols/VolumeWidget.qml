/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Shapes

import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Util
import VLC.Style
import VLC.Player

T.Pane {
    id: root

    property bool paintOnly: false

    readonly property color sliderColor: {
        const theme = volControl.colorContext
        return (volControl.position > _fullvolpos) ? theme.fg.negative : theme.fg.primary
    }

    readonly property var _player: paintOnly ? ({ muted: false, volume: .5 }) : Player
    readonly property int _maxvol: MainCtx.maxVolume
    readonly property real _fullvolpos: 100 / _maxvol
    readonly property real _maxvolpos: _maxvol / 100

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    contentWidth: volumeWidget.implicitWidth
    contentHeight: volumeWidget.implicitHeight

    Keys.priority: Keys.AfterItem
    Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

    RowLayout {
        id: volumeWidget

        anchors.fill: parent
        spacing: 0

        Widgets.IconToolButton{
            id: volumeBtn

            focus: true
            paintOnly: root.paintOnly
            text: {
                if (_player.muted)
                    return VLCIcons.volume_muted
                else if (_player.volume === 0)
                    return VLCIcons.volume_zero
                else if (_player.volume < .33)
                    return VLCIcons.volume_low
                else if (_player.volume <= .66)
                    return VLCIcons.volume_medium
                else
                    return VLCIcons.volume_high
            }
            description: qsTr("Mute")

            AccessibleCompat.id: "muteButton"

            onClicked: Player.muted = !Player.muted

            Accessible.onIncreaseAction: {
                Player.muted = false
                Player.setVolumeUp()
            }

            Accessible.onDecreaseAction: {
                Player.muted = false
                Player.setVolumeDown()
            }

            Navigation.parentItem: root
            Navigation.rightItem: volControl
        }

        Widgets.SliderExt {
            id: volControl

            // Redesign: classic VLC volume wedge
            implicitWidth: VLCStyle.dp(88, VLCStyle.scale)
            implicitHeight: VLCStyle.dp(28, VLCStyle.scale)

            handleWidth: 0

            // WARNING: Directly setting the insets break handle and background positioning.
            Binding on topInset {
                when: MainCtx.usingTouch
                value: VLCStyle.touchHandlerMargin
            }
            Binding on bottomInset {
                when: MainCtx.usingTouch
                value: VLCStyle.touchHandlerMargin
            }

            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: VLCStyle.margin_xsmall
            Layout.rightMargin: VLCStyle.margin_xxsmall

            property bool _inhibitPlayerVolumeUpdate: false

            // FIXME: Currently we are not updating the ShiftModifier status while dragging. This
            //        could be fixed with a custom Slider based on a MouseArea.
            property bool _shiftPressed: false

            property real _clamp: 0.01

            property bool _keyPressed: false

            property bool filterEvents: false

            color: root.sliderColor

            background: Item {
                id: wedge

                x: volControl.leftPadding
                y: volControl.topPadding + (volControl.availableHeight - height) / 2

                implicitWidth: VLCStyle.dp(88, VLCStyle.scale)
                implicitHeight: VLCStyle.dp(28, VLCStyle.scale)

                width: volControl.availableWidth
                height: Math.min(implicitHeight, volControl.availableHeight)

                // smooth the slanted edges of the wedge
                layer.enabled: true
                layer.samples: 4

                readonly property real _inset: VLCStyle.dp(2, VLCStyle.scale)
                // height of the thin (left) end of the wedge
                readonly property real _tip: height * 0.22

                // track
                Shape {
                    anchors.fill: parent

                    ShapePath {
                        strokeWidth: volControl.visualFocus ? VLCStyle.dp(1, VLCStyle.scale) : -1
                        strokeColor: volControl.colorContext.visualFocus
                        fillColor: volControl.colorContext.bg.secondary

                        startX: 0; startY: (wedge.height - wedge._tip) / 2
                        PathLine { x: wedge.width; y: 0 }
                        PathLine { x: wedge.width; y: wedge.height }
                        PathLine { x: 0; y: (wedge.height + wedge._tip) / 2 }
                        PathLine { x: 0; y: (wedge.height - wedge._tip) / 2 }
                    }
                }

                // level, clipped to the current volume
                Item {
                    width: Math.round(volControl.visualPosition * wedge.width)
                    height: wedge.height
                    clip: true

                    Shape {
                        width: wedge.width
                        height: wedge.height

                        ShapePath {
                            strokeWidth: -1

                            fillGradient: LinearGradient {
                                x1: 0; y1: 0
                                x2: wedge.width; y2: 0
                                // design colors, darkened by the 20% black overlay of the design
                                GradientStop { position: 0.0; color: "#77A913" }
                                GradientStop { position: 0.58; color: "#8E8713" }
                                GradientStop { position: 0.79; color: "#A96113" }
                                GradientStop { position: 1.0; color: "#CC2C2C" }
                            }

                            startX: wedge._inset; startY: (wedge.height - wedge._tip) / 2 + wedge._inset / 2
                            PathLine { x: wedge.width - wedge._inset; y: wedge._inset }
                            PathLine { x: wedge.width - wedge._inset; y: wedge.height - wedge._inset }
                            PathLine { x: wedge._inset; y: (wedge.height + wedge._tip) / 2 - wedge._inset / 2 }
                            PathLine { x: wedge._inset; y: (wedge.height - wedge._tip) / 2 + wedge._inset / 2 }
                        }
                    }
                }
            }

            from: 0
            to: root._maxvolpos
            opacity: _player.muted ? 0.5 : 1

            enabled: !root.paintOnly // disables event handling depending on this

            toolTipTextProvider: function (value) {
                // the real value i.e 'Player.volume' bounds can be different
                // from bounds of this widget and we want to show the current
                // volume here, so directly use Player.volume instead of "value"

                return Math.round(Player.volume * 100) + "%"
            }

            Accessible.name: qsTr("Volume")
            AccessibleCompat.id: "volumeSlider"

            Keys.onPressed: (event) => {
                if (KeyHelper.matchOk(event)) {
                    event.accepted = true

                    _keyPressed = true
                } else {
                    Navigation.defaultKeyAction(event)
                }
            }

            Keys.onReleased: (event) => {
                if (_keyPressed === false)
                    return

                _keyPressed = false

                if (KeyHelper.matchOk(event)) {
                    Player.muted = !Player.muted
                }
            }

            function _syncVolumeWithPlayer() {
                volControl._inhibitPlayerVolumeUpdate = true
                volControl.value = _player.volume
                volControl._inhibitPlayerVolumeUpdate = false
            }

            function _adjustPlayerVolume() {
                Player.muted = false

                let value = volControl.value

                // NOTE: We are clamping the value to make it easier to restore the default volume.
                if (_shiftPressed === false) {
                    if (Math.abs(value - 1.0) < _clamp)
                        value = 1.0
                    else
                        _clamp = 0.01
                }

                Player.volume = value

                volControl.value = value
            }

            Component.onCompleted: {
                root.paintOnlyChanged.connect(_syncVolumeWithPlayer)
                volControl._syncVolumeWithPlayer()
            }

            Connections {
                target: Player
                enabled: !paintOnly

                function onVolumeChanged() { volControl._syncVolumeWithPlayer() }
            }

            Binding on Widgets.PointingToolTipAttached.visible {
                when: sliderMouseArea.pressed
                value: true
            }

            Navigation.leftItem: volumeBtn
            Navigation.parentItem: root

            Keys.onUpPressed: (event) => {
                Player.muted = false
                Player.setVolumeUp()
            }

            Keys.onDownPressed: (event) => {
                Player.muted = false
                Player.setVolumeDown()
            }

            Keys.priority: Keys.BeforeItem

            onValueChanged: {
                if (paintOnly)
                    return

                if (!volControl._inhibitPlayerVolumeUpdate) {
                    if (filterEvents) {
                        Qt.callLater(volControl._adjustPlayerVolume)
                    } else {
                        volControl._adjustPlayerVolume()
                        filterEvents = true
                    }
                }
            }

            Rectangle{
                id: tickmark

                parent: volControl.background

                x : parent.width * root._fullvolpos
                width: VLCStyle.dp(1, VLCStyle.scale)
                height: parent.height
                radius: VLCStyle.dp(2, VLCStyle.scale)

                // NOTE: This shouldn't be visible when the volume stops before a 100.
                visible: (root._maxvol > 100)

                // FIXME: tick mark is not visible when it's over slider
                color: root.sliderColor

                // NOTE: This is a helper to select the default volume when clicking on the
                //       tickmark. We apply a higher clamp value to achieve that behavior on
                //       the Slider.
                MouseArea {
                    anchors.fill: parent

                    anchors.margins: -(VLCStyle.dp(4, VLCStyle.scale))

                    onPressed: (mouse) => {
                        mouse.accepted = false

                        if (mouse.modifiers === Qt.ShiftModifier)
                            return

                        volControl._clamp = 0.1
                    }
                }
            }

            MouseArea {
                id: sliderMouseArea

                anchors.fill: parent

                acceptedButtons: (Qt.LeftButton | Qt.RightButton)

                onPressed: (mouse) => {
                    if (root.paintOnly) {
                        mouse.accepted = true
                        return
                    }

                    volControl._shiftPressed = (mouse.modifiers === Qt.ShiftModifier)

                    if (mouse.button === Qt.LeftButton) {
                        mouse.accepted = false

                        return
                    }

                    volControl.filterEvents = false
                    adjustVolume(mouse)
                }

                onPositionChanged: (mouse) => {
                    if (mouse.buttons & Qt.RightButton)
                        adjustVolume(mouse)
                }

                onWheel: (wheel) => {

                    if ((Math.abs(wheel.pixelDelta.x) % 120 > 0) || (Math.abs(wheel.pixelDelta.y) % 120 > 0)) {
                        let delta = 0
                        if (Math.abs(wheel.pixelDelta.x) > Math.abs(wheel.pixelDelta.y))
                            delta = wheel.pixelDelta.x
                        else
                            delta = wheel.pixelDelta.y

                        if (wheel.inverted)
                            delta = -delta
                        volControl.value += 0.001 * delta
                    }
                    else
                        wheelToVLC.qmlWheelEvent(wheel)

                    wheel.accepted = true
                }

                function adjustVolume(mouse) {
                    mouse.accepted = true

                    const pos = mouse.x * root._maxvolpos / width

                    if (pos < 0.25)
                        volControl.value = 0
                    else if (pos < 0.75)
                        volControl.value = 0.5
                    else if (pos < 1.125)
                        volControl.value = 1
                    else
                        volControl.value = 1.25
                }

                WheelToVLCConverter {
                    id: wheelToVLC

                    function handle(steps: int) {
                        if (steps > 0)
                            Player.setVolumeUp(steps)
                        else
                            Player.setVolumeDown(-steps)
                    }

                    Component.onCompleted: {
                        wheelUpDown.connect(wheelToVLC.handle)
                        wheelLeftRight.connect(wheelToVLC.handle)
                    }
                }
            }
        }
    }
}
