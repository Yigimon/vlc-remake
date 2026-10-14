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

import VLC.MainInterface
import VLC.Style
import VLC.Util

// Redesign: ON / OFF switch of the effects panel (see the "Toggle" component of the design)
T.Switch {
    id: control

    implicitWidth: VLCStyle.dp(68, VLCStyle.scale)
    implicitHeight: VLCStyle.dp(32, VLCStyle.scale)

    padding: VLCStyle.margin_xxxsmall

    focusPolicy: Qt.StrongFocus

    Accessible.role: Accessible.CheckBox
    Accessible.checkable: true
    Accessible.checked: control.checked

    Keys.priority: Keys.AfterItem
    Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View

        focused: control.visualFocus
        hovered: control.hovered
        enabled: control.enabled
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

    // FG/A4
    readonly property ColorContext _disabledTheme: ColorContext {
        id: disabledTheme
        colorSet: ColorContext.View
        enabled: false
    }

    readonly property real _radius: VLCStyle.dp(4, VLCStyle.scale)

    background: Rectangle {
        radius: control._radius
        color: sliderTheme.bg.secondary
        border.width: control.visualFocus ? VLCStyle.focus_border : 0
        border.color: theme.visualFocus
    }

    indicator: Item {
        x: control.leftPadding
        y: control.topPadding
        width: control.availableWidth
        height: control.availableHeight

        opacity: control.enabled ? 1.0 : 0.5

        Rectangle {
            anchors.fill: parent
            radius: control._radius - VLCStyle.margin_xxxsmall / 2
            color: control.checked ? sliderTheme.bg.highlight : windowTheme.bg.secondary
        }

        Rectangle {
            id: knob

            width: height
            height: parent.height
            x: control.checked ? parent.width - width : 0

            radius: control._radius - VLCStyle.margin_xxxsmall / 2
            color: control.checked ? accentTheme.bg.primary
                                   : (control.hovered ? theme.fg.secondary : disabledTheme.fg.primary)

            Behavior on x {
                enabled: control.visible
                NumberAnimation { duration: VLCStyle.duration_short; easing.type: Easing.InOutSine }
            }
        }

        Text {
            anchors {
                left: control.checked ? parent.left : knob.right
                right: control.checked ? knob.left : parent.right
                verticalCenter: parent.verticalCenter
            }

            horizontalAlignment: Text.AlignHCenter
            text: control.checked ? qsTr("ON") : qsTr("OFF")
            color: control.checked ? sliderTheme.fg.primary : theme.fg.secondary
            font.pixelSize: VLCStyle.fontSize_small
            elide: Text.ElideRight
        }
    }

    contentItem: null
}
