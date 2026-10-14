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
import VLC.Widgets as Widgets

// Redesign: collapsible card of the effects panel ("Expand Settings" /
// "Collapse Settings" in the design), with an optional ON / OFF switch
FocusScope {
    id: root

    property alias title: titleLabel.text

    property bool expanded: false

    // shows a switch in the header
    property bool checkable: false
    property bool checked: false

    // the settings of the section
    default property alias content: contentColumn.data

    signal toggled(bool checked)

    implicitWidth: VLCStyle.dp(360, VLCStyle.scale)
    implicitHeight: header.height + (root.expanded ? contentColumn.implicitHeight + contentColumn.anchors.topMargin
                                                     + VLCStyle.margin_large
                                                   : 0)

    Accessible.role: Accessible.Grouping
    Accessible.name: titleLabel.text

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View

        hovered: headerArea.containsMouse
    }

    readonly property ColorContext _sliderTheme: ColorContext {
        id: sliderTheme
        colorSet: ColorContext.Slider
    }

    readonly property ColorContext _windowTheme: ColorContext {
        id: windowTheme
        colorSet: ColorContext.Window
    }

    Rectangle {
        anchors.fill: parent

        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: root.expanded ? windowTheme.bg.primary : sliderTheme.bg.secondary
        border.width: headerButton.visualFocus ? VLCStyle.focus_border : 0
        border.color: theme.visualFocus
    }

    Item {
        id: header

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: VLCStyle.dp(48, VLCStyle.scale)

        MouseArea {
            id: headerArea

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor

            onClicked: root.expanded = !root.expanded
        }

        // keyboard access to the header
        T.AbstractButton {
            id: headerButton

            anchors.fill: parent
            focus: true
            focusPolicy: Qt.TabFocus

            background: null
            contentItem: null

            Accessible.name: titleLabel.text
            Accessible.role: Accessible.Button
            Accessible.description: root.expanded ? qsTr("Collapse") : qsTr("Expand")

            onClicked: root.expanded = !root.expanded

            Keys.priority: Keys.AfterItem
            Keys.onPressed: (event) => Navigation.defaultKeyAction(event)
        }

        RowLayout {
            anchors {
                fill: parent
                leftMargin: VLCStyle.margin_normal
                rightMargin: VLCStyle.margin_normal
            }

            spacing: VLCStyle.margin_small

            Text {
                id: titleLabel

                Layout.fillWidth: true

                color: root.checked ? theme.fg.primary : theme.fg.secondary
                font.pixelSize: VLCStyle.fontSize_normal
                elide: Text.ElideRight
            }

            EffectsSwitch {
                visible: root.checkable

                checked: root.checked
                onToggled: root.toggled(checked)

                Accessible.name: titleLabel.text
            }

            Widgets.IconLabel {
                font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                color: theme.fg.secondary
                text: root.expanded ? VLCIcons.ic_fluent_chevron_down_24 : VLCIcons.ic_fluent_chevron_right_24
            }
        }
    }

    ColumnLayout {
        id: contentColumn

        visible: root.expanded

        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            topMargin: VLCStyle.margin_small - VLCStyle.margin_xxxsmall
            leftMargin: VLCStyle.margin_normal
            rightMargin: VLCStyle.margin_normal
        }

        spacing: VLCStyle.margin_large
    }
}
