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
import QtQuick.Templates as T
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window


import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets
import VLC.Util

T.ComboBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + spacing + implicitIndicatorWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    // Redesign: see the "Dropdown" component of the design
    padding: VLCStyle.margin_xxsmall
    leftPadding: VLCStyle.margin_small
    rightPadding: VLCStyle.margin_xsmall
    spacing: VLCStyle.margin_xsmall

    font.pixelSize: VLCStyle.fontSize_large

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.ComboBox

        focused: control.activeFocus
        enabled: control.enabled
        hovered: control.hovered
        pressed: control.pressed
    }

    readonly property ColorContext popupColorContext: ColorContext {
        id: popupTheme
        colorSet: ColorContext.Window
    }

    readonly property ColorContext itemColorContext: ColorContext {
        id: itemTheme
        colorSet: ColorContext.Item
        hovered: true
    }

    property color color: theme.fg.primary
    property color bgColor: theme.bg.primary
    property color borderColor: theme.border

    readonly property real _radius: VLCStyle.dp(6, VLCStyle.scale)

    // Optional, returns the text of an entry of the popup from its model data
    property var delegateTextFunc: null

    Keys.priority: Keys.AfterItem
    Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

    delegate: T.ItemDelegate {
        id: itemDelegate

        width: ListView.view ? ListView.view.width : control.width
        implicitWidth: implicitContentWidth + leftPadding + rightPadding
        implicitHeight: Math.max(VLCStyle.dp(32, VLCStyle.scale), implicitContentHeight + topPadding + bottomPadding)

        padding: VLCStyle.margin_xxsmall
        leftPadding: control.leftPadding - VLCStyle.margin_xxxsmall
        rightPadding: control.rightPadding

        highlighted: control.highlightedIndex === index

        readonly property bool isCurrent: control.currentIndex === index

        // the popup is as large as its largest entry:
        function _updatePopupWidth() {
            const view = ListView.view
            if (view)
                view.maxItemImplicitWidth = Math.max(view.maxItemImplicitWidth, implicitWidth)
        }

        onImplicitWidthChanged: _updatePopupWidth()
        Component.onCompleted: _updatePopupWidth()

        contentItem: RowLayout {
            spacing: VLCStyle.margin_xsmall

            Widgets.ListLabel {
                Layout.fillWidth: true

                text: {
                    if (control.delegateTextFunc)
                        return control.delegateTextFunc(model)
                    return control.textRole ? (Helpers.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
                }
                color: itemDelegate.highlighted ? itemTheme.fg.highlight : control.color
                font.pixelSize: control.font.pixelSize
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Widgets.IconLabel {
                visible: itemDelegate.isCurrent
                text: VLCIcons.check
                color: theme.accent
                font.pixelSize: VLCStyle.icon_small
            }
        }

        background: Rectangle {
            radius: VLCStyle.dp(4, VLCStyle.scale)
            color: itemDelegate.highlighted ? itemTheme.bg.primary : "transparent"
        }
    }

    indicator: Widgets.IconLabel {
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        font.pixelSize: VLCStyle.icon_small
        text: control.popup.visible ? VLCIcons.chevron_up : VLCIcons.chevron_down
        color: control.color
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    contentItem: Widgets.ListLabel {
        rightPadding: control.spacing + control.indicator.width

        text: control.displayText
        color: control.color
        font.pixelSize: control.font.pixelSize
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Widgets.AnimatedBackground {
        enabled: theme.initialized
        color: control.bgColor
        border.color: (control.visualFocus || control.popup.visible) ? theme.visualFocus
                                                                   : Qt.alpha(theme.visualFocus, 0.0)
        border.width: VLCStyle.border
        radius: control._radius
    }

    popup: Popup {
        y: control.height + VLCStyle.margin_xxsmall

        // NOTE: This Popup should be on top of other Popup(s) most of the time.
        z: 100

        // keep the popup inside the window, e.g. for a combo box at the bottom
        margins: VLCStyle.margin_xsmall

        width: Math.min(Math.max(control.width, implicitContentWidth + leftPadding + rightPadding),
                        control.Window.width - VLCStyle.margin_normal)
        padding: VLCStyle.margin_xxxsmall

        // keep the popup inside the window:
        height: Math.min(implicitHeight, control.Window.height - VLCStyle.margin_normal)

        contentItem: ListView {
            id: popupListView

            clip: true

            property real maxItemImplicitWidth: 0

            // NOTE: The height of all the entries is needed to size the popup,
            //       the model is always set (it is small).
            implicitHeight: contentHeight
            implicitWidth: maxItemImplicitWidth
            model: control.delegateModel
            currentIndex: control.highlightedIndex

            spacing: VLCStyle.margin_xxxsmall

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: popupTheme.bg.secondary
            border.color: popupTheme.border
            border.width: VLCStyle.border
            radius: control._radius

            Widgets.DefaultShadow {
                radius: control._radius
                z: -1
            }
        }
    }
}
