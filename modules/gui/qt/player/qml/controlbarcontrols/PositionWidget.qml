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
import VLC.Player
import VLC.Widgets as Widgets
import VLC.Style
import VLC.Util

// Redesign timeline: current time | seek bar | duration (or remaining time),
// placed inline in the control bar. When this control is part of the layout
// the control bar does not show its own seek bar row (see ControlBar.qml).
FocusScope {
    id: root

    property bool paintOnly: false

    // Expanding control: it takes the free space of its row (see ControlRepeater.qml).
    Layout.fillWidth: true
    Layout.minimumWidth: VLCStyle.dp(240, VLCStyle.scale)

    implicitWidth: paintOnly ? VLCStyle.dp(240, VLCStyle.scale) : VLCStyle.dp(320, VLCStyle.scale)
    implicitHeight: VLCStyle.dp(28, VLCStyle.scale)

    Accessible.role: Accessible.Grouping
    Accessible.name: qsTr("Timeline")

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.ToolButton
    }

    TextMetrics {
        id: timeTextMetrics
        font.pixelSize: VLCStyle.dp(15, VLCStyle.scale)
        // Same width for both labels, with some extra room for non-monospaced digits:
        text: (Player.time.isSubHour() && Player.length.isSubHour()) ? "-00:00-" : "-00:00:00-"
    }

    RowLayout {
        anchors.fill: parent
        spacing: VLCStyle.margin_xsmall

        T.Label {
            id: mediaTime

            Layout.preferredWidth: timeTextMetrics.width + 2 * VLCStyle.margin_xsmall
            Layout.fillHeight: true

            text: {
                if (root.paintOnly)
                    return "00:00"
                const length = Player.length
                return Player.time.formatHMS(length.isSubSecond() ? VLCTick.SubSecondFormattedAsMS : 0)
            }
            color: theme.fg.primary
            font: timeTextMetrics.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            Accessible.name: qsTr("Elapsed time")

            // TODO: Do not use `Binding` when minimum Qt is 6.12:
            Binding {
                target: mediaTime
                property: "mutabilityGroup"
                when: (mediaTime.mutabilityGroup !== undefined)
                value: (Player.playingState === Player.PLAYING_STATE_PLAYING) ? Item.ModerateMutabilityGroup
                                                                              : Item.StaticMutabilityGroup
            }
        }

        SliderBar {
            id: trackPositionSlider

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            pillStyle: true
            barHeight: root.height

            enabled: !root.paintOnly &&
                     (Player.playingState === Player.PLAYING_STATE_PLAYING || Player.playingState === Player.PLAYING_STATE_PAUSED)

            Accessible.role: Accessible.Slider
            Accessible.name: qsTr("Progress")

            focus: true
            activeFocusOnTab: true

            Navigation.parentItem: root

            Keys.onPressed: (event) => {
                Navigation.defaultKeyAction(event)
            }
        }

        T.Label {
            id: mediaRemainingTime

            Layout.preferredWidth: timeTextMetrics.width + 2 * VLCStyle.margin_xsmall
            Layout.fillHeight: true

            readonly property bool showRemainingTime: (MainCtx.showRemainingTime && Player.remainingTime.valid())

            text: {
                if (root.paintOnly)
                    return "00:00"
                return showRemainingTime ? "-" + Player.remainingTime.formatHMS()
                                         : Player.length.formatHMS()
            }
            color: theme.fg.primary
            font: timeTextMetrics.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            Accessible.name: showRemainingTime ? qsTr("Remaining time") : qsTr("Duration")

            background: Rectangle {
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: remainingTimeHover.hovered ? theme.bg.secondary : "transparent"
            }

            // TODO: Do not use `Binding` when minimum Qt is 6.12:
            Binding {
                target: mediaRemainingTime
                property: "mutabilityGroup"
                when: (mediaRemainingTime.mutabilityGroup !== undefined)
                value: ((Player.playingState === Player.PLAYING_STATE_PLAYING) &&
                        mediaRemainingTime.showRemainingTime) ? Item.ModerateMutabilityGroup
                                                              : Item.StaticMutabilityGroup
            }

            TapHandler {
                enabled: !root.paintOnly
                onTapped: (eventPoint, button) => {
                    MainCtx.showRemainingTime = !MainCtx.showRemainingTime
                }
            }

            HoverHandler {
                id: remainingTimeHover
                enabled: !root.paintOnly
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}
