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
import VLC.Widgets as Widgets

// Redesign: an always visible search field (search icon | text | clear button).
// It provides the same interface as SearchBox where it is used in place of it.
FocusScope {
    id: root

    property string searchPattern
    property alias textField: textField
    property alias compressSearchPatternChanges: searchPatternBinding.delayed
    compressSearchPatternChanges: true

    readonly property bool expanded: true

    implicitWidth: VLCStyle.widthSearchField
    implicitHeight: VLCStyle.dp(34, VLCStyle.scale)

    Binding on searchPattern {
        id: searchPatternBinding
        value: textField.text
    }

    // same functions as SearchBox:
    function expandAndFocus() {
        textField.forceActiveFocus(Qt.ShortcutFocusReason)
    }

    function retract() {
        textField.clear()
    }

    TextFieldExt {
        id: textField

        property bool _keyPressed: false

        anchors.fill: parent

        focus: true

        radius: VLCStyle.dp(6, VLCStyle.scale)

        topPadding: 0
        bottomPadding: 0
        leftPadding: searchIcon.x + searchIcon.width + VLCStyle.margin_xsmall
        rightPadding: clearButton.visible ? (clearButton.width + VLCStyle.margin_xsmall)
                                          : VLCStyle.margin_xsmall

        selectByMouse: true

        placeholderText: qsTr("Search")

        Accessible.searchEdit: true

        Navigation.parentItem: root
        Navigation.rightItem: clearButton.visible ? clearButton : null
        Navigation.cancelAction: function() {
            if (textField.text.length > 0)
                textField.clear()
            else
                root.Navigation.defaultNavigationUp()
        }

        //ideally we should use Keys.onShortcutOverride but it doesn't
        //work with TextField before 5.13 see QTBUG-68711
        onActiveFocusChanged: {
            MainCtx.useGlobalShortcuts = !activeFocus
        }

        Keys.priority: Keys.AfterItem

        Keys.onPressed: (event) => {
            _keyPressed = true

            //we don't want Navigation.cancelAction to match Backspace
            if (event.matches(StandardKey.Backspace))
                event.accepted = true

            Navigation.defaultKeyAction(event)
        }

        Keys.onReleased: (event) => {
            if (_keyPressed === false)
                return

            _keyPressed = false

            //we don't want Navigation.cancelAction to match Backspace
            if (event.matches(StandardKey.Backspace))
                event.accepted = true

            Navigation.defaultKeyReleaseAction(event)
        }

        Widgets.IconLabel {
            id: searchIcon

            x: VLCStyle.margin_xsmall
            anchors.verticalCenter: parent.verticalCenter

            text: VLCIcons.search
            font.pixelSize: VLCStyle.icon_normal
            color: textField.activeFocus ? textField.color : textField.placeholderTextColor

            Accessible.ignored: true
        }

        Widgets.IconToolButton {
            id: clearButton

            anchors.right: parent.right
            anchors.rightMargin: VLCStyle.margin_xxsmall
            anchors.verticalCenter: parent.verticalCenter

            font.pixelSize: VLCStyle.icon_small
            text: VLCIcons.close

            description: qsTr("Clear")

            visible: (textField.text.length > 0)

            onVisibleChanged: {
                if (activeFocus && !visible) {
                    textField.focus = true
                }
            }

            onClicked: textField.clear()

            Navigation.parentItem: textField
            Navigation.leftItem: textField
        }
    }
}
