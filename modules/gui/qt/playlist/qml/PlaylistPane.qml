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
import QtQuick.Window
import QtQuick.Templates as T
import QtQuick.Layouts
import QtQml.Models


import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Util
import VLC.Playlist
import VLC.Style
import VLC.Dialogs

T.Pane {
    id: root

    property var model: PlaylistListModel {
        playlist: MainPlaylistController.playlist
    }
    readonly property ListSelectionModel selectionModel: listView?.selectionModel ?? null

    property bool useAcrylic: true

    // Redesign: the queue lives on a rounded panel placed inside the paddings
    // of the pane, the area around it is filled with the content background
    // (it can be disabled when the pane is shown over the video).
    property bool showOuterBackground: true
    property color panelColor: windowTheme.bg.secondary
    property real panelOpacity: (useAcrylic && AcrylicController.enabled && showOuterBackground) ? 0.7 : 1.0

    readonly property color _effectivePanelColor: Qt.alpha(panelColor, panelColor.a * panelOpacity)

    // horizontal margin between the panel border and the queue entries
    readonly property real listMargin: VLCStyle.margin_xsmall
    // horizontal margin between the panel border and the header
    readonly property real headerMargin: VLCStyle.margin_normal

    readonly property real minimumWidth: contentItem.Layout.minimumWidth +
                                         leftPadding +
                                         rightPadding

    readonly property ListView listView: contentItem.listView

    property alias contextMenu: contextMenu

    property alias dragItem: dragItem

    property alias isDropAcceptableFunc: listView.isDropAcceptableFunc
    property alias acceptDropFunc: listView.acceptDropFunc

    property alias view: listView

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: VLCStyle.margin_xsmall

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Play Queue")
    AccessibleCompat.id: "playqueuePane"

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View

        focused: root.activeFocus
        hovered: root.hovered
        enabled: root.enabled
    }

    readonly property ColorContext windowColorContext: ColorContext {
        id: windowTheme
        colorSet: ColorContext.Window
    }

    Widgets.DragItem {
        id: dragItem

        view: root.listView

        onRequestData: (indexes, resolve, reject) => {
            resolve(indexes.map((index) => {
                const item = root.model.itemAt(index)
                return {
                    "title": item.title,
                    "cover": (!!item.artwork && item.artwork.toString() !== "") ? item.artwork : VLCStyle.noArtAlbumCover,
                    "url": item.url
                }
            }))
        }

        onRequestInputItems: (indexes, data, resolve, reject) => {
            resolve(root.model.getItemsForIndexes((indexes?.length > 0) ? indexes : root.selectionModel.selectedIndexesFlat))
        }
    }

    PlaylistContextMenu {
        id: contextMenu
        model: root.model
        proxyModel: (root.contentItem?.listView?.model instanceof AbstractProxyModel) ? root.contentItem.listView.model
                                                                                        : null
        selectionModel: root.selectionModel
        controler: MainPlaylistController
        ctx: MainCtx

        onJumpToCurrentPlaying: listView.positionViewAtIndex( MainPlaylistController.currentIndex, ItemView.Center)
    }

    background: Item {
        Widgets.AcrylicBackground {
            anchors.fill: parent

            visible: root.showOuterBackground
            enabled: root.useAcrylic
            tintColor: theme.bg.primary
        }

        Rectangle {
            x: root.leftPadding
            y: root.topPadding
            width: root.availableWidth
            height: root.availableHeight

            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: root._effectivePanelColor
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        Layout.minimumWidth: Math.min(Math.max(noContentInfoColumn.implicitWidth + 2 * root.listMargin,
                                               summaryRowLayout.implicitWidth + 2 * root.headerMargin),
                                      /* Minimum width should not be too large, we rely on elision otherwise: */
                                      Window.width / 3)

        readonly property ListView listView: listView

        // Redesign: "Play Queue" title, then a summary row with the number
        // of entries, their duration and the queue actions (see the "Queue"
        // frame of the miniplayer in the design).
        Widgets.SubtitleLabel {
            id: titleLabel

            Layout.fillWidth: true
            Layout.leftMargin: root.headerMargin
            Layout.rightMargin: root.headerMargin
            Layout.topMargin: VLCStyle.margin_normal

            text: qsTr("Play Queue")
            color: theme.fg.primary
            font.weight: Font.Normal
            font.pixelSize: VLCStyle.fontSize_large

            Accessible.role: Accessible.Heading
        }

        RowLayout {
            id: summaryRowLayout

            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.leftMargin: root.headerMargin
            // the icon of the buttons is centered, align it to the duration of the entries:
            Layout.rightMargin: root.headerMargin - VLCStyle.margin_xxsmall
            Layout.topMargin: VLCStyle.margin_xsmall
            Layout.bottomMargin: VLCStyle.margin_small

            spacing: VLCStyle.margin_xsmall

            Widgets.CaptionLabel {
                Layout.fillWidth: true

                color: theme.fg.secondary
                font.pixelSize: VLCStyle.fontSize_normal
                elide: Text.ElideRight

                text: {
                    if (listView.count <= 0)
                        return "" // the list shows its own placeholder

                    let duration
                    const model = listView.model
                    if (model instanceof PlaylistListModel) {
                        duration = model.duration
                    } else {
                        // Proxy model, we need to use `QAbstractItemModel` api:

                        duration = []
                        for (let i = 0; i < listView.count; ++i) {
                            const itemDuration = model.data(model.index(i, 0), PlaylistListModel.DurationRole)
                            console.assert(itemDuration !== undefined && itemDuration !== null) // typeof `vlcDuration`
                            duration.push(itemDuration)
                        }
                        duration = duration[0].sum(duration)
                    }

                    return qsTr("%1 elements, %2").arg(listView.count).arg(duration.formatLong())
                }
            }

            Widgets.SearchBox {
                id: searchBox

                enabled: MainPlaylistController.count > 1

                toggleButton.font.pixelSize: VLCStyle.icon_normal

                popBelow: true

                displayRegexToggleButton: true
                displayCaseSensitiveToggleButton: true

                popup.width: Math.max(popup.implicitWidth, root.contentItem?.width * 0.6)
                popup.topPadding: VLCStyle.margin_xxsmall

                Navigation.parentItem: root
                Navigation.rightItem: sort
                Navigation.downItem: listView

                Connections {
                    target: contextMenu

                    function onDiscardedFilteredOutItems() {
                        searchBox.retract()
                    }
                }
            }

            Widgets.SortControl {
                id: sort

                font.pixelSize: VLCStyle.icon_normal

                description : qsTr("Sort")

                AccessibleCompat.id: "playqueueSort"

                enabled: MainPlaylistController.count > 1

                checked: MainPlaylistController.sortKey !== PlaylistController.SORT_KEY_NONE

                popupAbove: false

                model: MainPlaylistController.sortKeyTitleList

                onSortSelected: key => {
                    MainPlaylistController.sortKey = key
                }

                onSortOrderSelected: type => {
                    if (type === Qt.AscendingOrder)
                        MainPlaylistController.sortOrder = PlaylistController.SORT_ORDER_ASC
                    else if (type === Qt.DescendingOrder)
                        MainPlaylistController.sortOrder = PlaylistController.SORT_ORDER_DESC

                    MainPlaylistController.sort()
                }

                sortOrder: {
                    if (MainPlaylistController.sortOrder === PlaylistController.SORT_ORDER_ASC) {
                        Qt.AscendingOrder
                    }
                    else if (MainPlaylistController.sortOrder === PlaylistController.SORT_ORDER_DESC) {
                        Qt.DescendingOrder
                    }
                }

                sortKey: MainPlaylistController.sortKey

                Navigation.parentItem: root
                Navigation.leftItem: searchBox
                Navigation.rightItem: removeButton
                Navigation.downItem: listView
            }

            Widgets.IconToolButton {
                id: removeButton

                font.pixelSize: VLCStyle.icon_normal
                width: VLCStyle.bannerButton_width
                height: VLCStyle.bannerButton_height

                text: VLCIcons.remove
                description: qsTr("Remove Selected")

                AccessibleCompat.id: "playqueueRemove"

                enabled: (root.selectionModel?.hasSelection ?? false) && listView.count > 0

                onClicked: listView.removeSelectedItems()

                Navigation.parentItem: root
                Navigation.leftItem: sort
                Navigation.rightItem: addButton
                Navigation.downItem: listView
            }

            Widgets.IconToolButton {
                id: addButton

                font.pixelSize: VLCStyle.icon_normal
                width: VLCStyle.bannerButton_width
                height: VLCStyle.bannerButton_height

                text: VLCIcons.add
                description: qsTr("Add File...")

                AccessibleCompat.id: "playqueueAdd"

                // add the files to the queue without starting the playback:
                onClicked: DialogsProvider.simpleOpenDialog(false)

                Navigation.parentItem: root
                Navigation.leftItem: removeButton
                Navigation.downItem: listView
            }
        }

        Widgets.ListViewExt {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true

            selectionModel: ListSelectionModel {
                model: listView.model
            }

            focus: true

            clip: !fadingEdge.implicitClipping && (height < contentHeight)

            model: proxyModel ?? root.model

            onWidthChanged: {
                if (searchBox.expanded) {
                    if (width < searchBox.textField.width)
                        searchBox.retract()
                }
            }

            property QtSortFilterProxyModel proxyModel

            function adjustFiltering() {
                // Once text changes, we switch to the proxy model.
                // We do not switch back to the source model after that.
                if (!proxyModel) {
                    proxyModel = proxyModelComponent.createObject(listView)
                }

                if (searchBox.regexButtonToggled)
                    proxyModel.setFilterRegularExpression(searchBox.searchPattern)
                else
                    proxyModel.setFilterFixedString(searchBox.searchPattern)
            }

            Component {
                id: proxyModelComponent

                QtSortFilterProxyModel {
                    filterRole: PlaylistListModel.FilterRole

                    sourceModel: root.model

                    filterCaseSensitivity: searchBox.caseSensitiveButtonToggled ? Qt.CaseSensitive
                                                                                : Qt.CaseInsensitive
                }
            }

            Layout.leftMargin: root.listMargin
            Layout.rightMargin: root.listMargin
            Layout.bottomMargin: VLCStyle.margin_xsmall

            fadingEdge.backgroundColor: (root._effectivePanelColor.a >= 1.0) ? root._effectivePanelColor
                                                                             : Qt.alpha(root._effectivePanelColor, 0.0)

            function removeSelectedItems() {
                const items = listView.mappedSelectedIndexes()
                if (items.length > 0)
                    root.model.removeItems(items)
            }

            function mappedSelectedIndexes() : var {
                let mappedSelectedIndexes

                if (!listView.selectionModel)
                    return [] // unlikely

                const selectedIndexes = listView.selectionModel.selectedIndexesFlat
                if (listView.proxyModel && listView.model === listView.proxyModel) {
                    mappedSelectedIndexes = []
                    for (let i = 0; i < selectedIndexes.length; ++i) {
                        const mappedIndex = listView.model.mapToSource(listView.model.index(selectedIndexes[i], 0))
                        mappedSelectedIndexes.push(mappedIndex.row)
                    }
                } else {
                    mappedSelectedIndexes = selectedIndexes
                }

                return mappedSelectedIndexes
            }

            isDropAcceptableFunc: function(drop, index) {
                if (drop.source === dragItem)
                    return Helpers.itemsMovable(listView.mappedSelectedIndexes(), index)
                else if (Helpers.isValidInstanceOf(drop.source, Widgets.DragItem))
                    return true
                else if (drop.hasUrls)
                    return true
                else
                    return false
            }

            acceptDropFunc: function(index, drop) {
                const item = drop.source;

                // NOTE: Move implementation.
                if (dragItem === item) {
                    root.model.moveItemsPre(listView.mappedSelectedIndexes(), index);
                    listView.forceActiveFocus();
                // NOTE: Dropping medialibrary content into the queue.
                } else if (Helpers.isValidInstanceOf(item, Widgets.DragItem)) {
                    return item.getSelectedInputItem().then((inputItems) => {
                            if (!Helpers.isArray(inputItems) || inputItems.length === 0) {
                                console.warn("can't convert items to input items");
                                return
                            }
                            MainPlaylistController.insert(index, inputItems, false)
                        }).then(() => { listView.forceActiveFocus(); })
                // NOTE: Dropping an external item (i.e. filesystem) into the queue.
                } else if (drop.hasUrls) {
                    const urlList = [];

                    for (let url in drop.urls)
                        urlList.push(drop.urls[url]);

                    MainPlaylistController.insert(index, urlList, false);

                    // NOTE This is required otherwise backend may handle the drop as well yielding double addition.
                    drop.accept(Qt.IgnoreAction);
                    listView.forceActiveFocus();
                }

                return Promise.resolve()
            }

            property int shiftIndex: -1

            onShowContextMenu: (globalPos) => {
                contextMenu.popup(-1, globalPos)
            }

            Behavior on contentY {
                id: contentYBehavior

                enabled: false

                // NOTE: Usage of `SmoothedAnimation` is intentional here.
                SmoothedAnimation {
                    duration: VLCStyle.duration_veryLong
                    easing.type: Easing.InOutSine
                }
            }

            Component.onCompleted: {
                // WARNING: Tracking the current item and not the current index is intentional here.
                MainPlaylistController.currentItemChanged.connect(listView, () => {
                    // FIXME: Qt does not provide the `contentY` with `positionViewAtIndex()` for us
                    //        to animate. For that reason, we capture the new `contentY`, adjust
                    //        `contentY` to it is old value then enable the animation and set `contentY`
                    //        to its new value.
                    const oldContentY = listView.contentY
                    listView.positionViewAtIndex(MainPlaylistController.currentIndex, ListView.Contain)
                    const newContentY = listView.contentY
                    if (Math.abs(oldContentY - newContentY) >= Number.EPSILON) {
                        contentYBehavior.enabled = false
                        listView.contentY = oldContentY
                        contentYBehavior.enabled = true
                        listView.contentY = newContentY
                        contentYBehavior.enabled = false
                    }
                })

                searchBox.searchPatternChanged.connect(listView, listView.adjustFiltering)
                searchBox.regexButtonToggledChanged.connect(listView, listView.adjustFiltering)
            }

            Connections {
                target: listView.model

                function onRowsInserted() {
                    if (listView.currentIndex === -1)
                        listView.currentIndex = 0
                }

                function onModelReset() {
                    if (listView.currentIndex === -1 && listView.count > 0)
                        listView.currentIndex = 0
                }
            }

            delegate: PlaylistDelegate {
                id: delegate

                width: listView.contentWidth
                // keep the duration clear of the scroll bar:
                rightPadding: Math.max(listView.T.ScrollBar.vertical.width, VLCStyle.margin_small)

                contextMenu: root.contextMenu

                dragItem: root.dragItem

                isDropAcceptable: listView.isDropAcceptableFunc
                acceptDrop: listView.acceptDropFunc

                onContainsDragChanged: listView.updateItemContainsDrag(this, containsDrag)

                onIsCurrentChanged: {
                    if (isCurrent)
                        listView.fadingEdge.excludeItem = delegate
                    else if (listView.fadingEdge.excludeItem === delegate)
                        listView.fadingEdge.excludeItem = null
                }
            }

            Keys.onDeletePressed: listView.removeSelectedItems()

            Navigation.parentItem: root
            Navigation.upItem: sort

            onActionAtIndex: (index) => {
                if (index < 0)
                    return

                MainPlaylistController.goTo(index, true)
            }

            Column {
                id: noContentInfoColumn

                anchors.centerIn: parent

                visible: false
                enabled: visible

                spacing: VLCStyle.margin_xsmall

                opacity: (listView.activeFocus) ? 1.0 : 0.8

                Binding on visible {
                    delayed: true
                    value: (listView.count === 0 && !listView.footerItem.firstItemIndicatorVisible)
                }

                Widgets.IconLabel {
                    id: label

                    anchors.horizontalCenter: parent.horizontalCenter

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: VLCIcons.playlist

                    color: theme.fg.secondary

                    font.pixelSize: VLCStyle.dp(40, VLCStyle.scale)
                }

                T.Label {
                    anchors.horizontalCenter: parent.horizontalCenter

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight

                    text: (listView.proxyModel &&
                           listView.model === listView.proxyModel &&
                           listView.count !== root.model.count) ? qsTr("No results")
                                                                : qsTr("No content yet")

                    color: theme.fg.primary

                    font.pixelSize: VLCStyle.fontSize_large
                }

                T.Label {
                    anchors.horizontalCenter: parent.horizontalCenter

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight

                    text: qsTr("Drag & Drop some content here!")
                    visible: (listView.proxyModel && listView.model === listView.proxyModel) ? (listView.count === root.model.count)
                                                                                             : true

                    color: theme.fg.secondary

                    font.pixelSize: VLCStyle.fontSize_normal
                }
            }
        }
    }

    Keys.priority: Keys.AfterItem
    Keys.forwardTo: listView
    Keys.onPressed: (event) => root.Navigation.defaultKeyAction(event)

    component PlaylistToggleButton : Widgets.IconToolButton {
        id: playlistBtn

        focus: true

        font.pixelSize: VLCStyle.icon_banner
        text: VLCIcons.playlist
        description: qsTr("Play queue")

        AccessibleCompat.id: "playqueueToggle"

        width: VLCStyle.bannerButton_width
        height: VLCStyle.bannerButton_height
        //highlighted: MainCtx.playlistVisible

        property Item plListView

        DropArea {
            anchors.fill: parent

            onContainsDragChanged: {
                if (containsDrag) {
                    _timer.restart()

                    if (playlistBtn.plListView)
                        MainCtx.setCursor(Qt.DragCopyCursor)
                } else {
                    _timer.stop()

                    if (playlistBtn.plListView)
                        MainCtx.restoreCursor()
                }
            }

            onEntered: (drag) => {
                const plListView = playlistBtn.plListView
                if (plListView) {
                    console.assert(plListView.isDropAcceptableFunc)
                    console.assert(plListView.model)
                    if (plListView.isDropAcceptableFunc(drag, plListView.model.count)) {
                        drag.accept()
                    } else {
                        drag.accepted = false
                    }
                } else {
                    drag.accepted = false
                }
            }

            onDropped: (drop) => {
                const plListView = playlistBtn.plListView
                if (plListView) {
                    console.assert(plListView.acceptDropFunc)
                    plListView.acceptDropFunc(plListView.model.count, drop)
                }
            }

            Timer {
                id: _timer
                interval: VLCStyle.duration_humanMoment

                onTriggered: {
                    MainCtx.playqueuePanel.visible = true
                }
            }
        }
    }
}
