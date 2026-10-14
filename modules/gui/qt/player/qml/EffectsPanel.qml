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
import VLC.Player
import VLC.Widgets as Widgets

// Redesign: audio and video effects as a side panel of the player (see the
// 400 px wide "Audio - ..." and "Video - ..." panels of the design). It does
// what the "Adjustments and Effects" dialog does, the dialog stays available
// from the menus.
T.Pane {
    id: root

    // 0: audio, 1: video
    property int currentTab: 0

    property color panelColor: theme.bg.primary
    property real panelOpacity: 0.95 // BG/B1 Transparent

    // the values are read again from the player, e.g. when the panel is shown
    signal refreshRequested()

    function refresh() {
        root.refreshRequested()
    }

    implicitWidth: VLCStyle.dp(400, VLCStyle.scale) + leftPadding + rightPadding
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: VLCStyle.margin_xsmall

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Effects")

    onVisibleChanged: {
        if (visible)
            refresh()
    }

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View
    }

    readonly property ColorContext _sliderTheme: ColorContext {
        id: sliderTheme
        colorSet: ColorContext.Slider
    }

    readonly property ColorContext _windowTheme: ColorContext {
        id: windowTheme
        colorSet: ColorContext.Window
    }

    Connections {
        target: Player

        // the filters and their options live in the audio and video outputs
        function onHasVideoOutputChanged() { root.refresh() }
        function onPlayingStateChanged() { root.refresh() }
    }

    Connections {
        target: PlayerEffects

        function onEqualizerChanged() { root.refresh() }
    }

    background: Item {
        // the video below does not react to the mouse over the panel
        MouseArea {
            x: root.leftPadding
            y: root.topPadding
            width: root.availableWidth
            height: root.availableHeight

            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
            onWheel: (wheel) => { wheel.accepted = true }
        }

        Rectangle {
            x: root.leftPadding
            y: root.topPadding
            width: root.availableWidth
            height: root.availableHeight

            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: Qt.alpha(root.panelColor, root.panelColor.a * root.panelOpacity)
        }
    }

    // a setting stored in an option of an audio filter
    component AudioSlider: EffectsSlider {
        id: audioSlider

        required property string option

        Layout.fillWidth: true

        function refresh() {
            audioSlider.value = PlayerEffects.audioOption(audioSlider.option)
        }

        Component.onCompleted: refresh()
        onMoved: (value) => PlayerEffects.setAudioOption(audioSlider.option, value)

        Connections {
            target: root
            function onRefreshRequested() { audioSlider.refresh() }
        }
    }

    // a setting stored in an option of a video filter
    component VideoSlider: EffectsSlider {
        id: videoSlider

        required property string option

        Layout.fillWidth: true

        function refresh() {
            const value = PlayerEffects.videoOption(videoSlider.option)
            if (value !== undefined)
                videoSlider.value = value
        }

        Component.onCompleted: refresh()
        onMoved: (value) => PlayerEffects.setVideoOption(videoSlider.option, value)

        Connections {
            target: root
            function onRefreshRequested() { videoSlider.refresh() }
        }
    }

    // a setting of a video filter that is enabled by moving the setting away
    // from its neutral value ("Off")
    component AutoFilterSlider: EffectsSlider {
        id: autoSlider

        required property string option
        required property string filter
        property real neutralValue: 0

        property bool filterEnabled: false

        Layout.fillWidth: true

        defaultValue: neutralValue
        active: filterEnabled

        textFunc: (value) => {
            if (Math.abs(value - autoSlider.neutralValue) < autoSlider.stepSize / 2)
                return qsTr("Off")
            return (value * autoSlider.displayScale).toFixed(autoSlider.decimals) + autoSlider.unit
        }

        function refresh() {
            autoSlider.filterEnabled = PlayerEffects.isVideoFilterEnabled(autoSlider.filter)
            const value = PlayerEffects.videoOption(autoSlider.option)
            autoSlider.value = (autoSlider.filterEnabled && value !== undefined) ? value
                                                                               : autoSlider.neutralValue
        }

        Component.onCompleted: refresh()

        onMoved: (value) => {
            const enable = Math.abs(value - autoSlider.neutralValue) >= autoSlider.stepSize / 2
            // the option is set first, the filter reads it when it is created
            if (enable)
                PlayerEffects.setVideoOption(autoSlider.option, value)
            if (enable !== autoSlider.filterEnabled)
                PlayerEffects.setVideoFilterEnabled(autoSlider.filter, enable)
            autoSlider.filterEnabled = PlayerEffects.isVideoFilterEnabled(autoSlider.filter)
        }

        Connections {
            target: root
            function onRefreshRequested() { autoSlider.refresh() }
        }
    }

    // a section that enables an audio filter
    component AudioSection: EffectsSection {
        id: audioSection

        required property string filter

        Layout.fillWidth: true

        checkable: true

        function refresh() {
            audioSection.checked = PlayerEffects.isAudioFilterEnabled(audioSection.filter)
        }

        Component.onCompleted: refresh()
        onToggled: (checked) => {
            PlayerEffects.setAudioFilterEnabled(audioSection.filter, checked)
            refresh()
        }

        Connections {
            target: root
            function onRefreshRequested() { audioSection.refresh() }
        }
    }

    // a line with a text and a switch that enables a video filter
    component VideoFilterSwitch: RowLayout {
        id: filterSwitch

        required property string filter
        property alias text: switchLabel.text
        property alias checked: videoSwitch.checked

        // extra control between the text and the switch
        default property alias content: extraRow.data

        Layout.fillWidth: true
        spacing: VLCStyle.margin_xsmall

        function refresh() {
            videoSwitch.checked = PlayerEffects.isVideoFilterEnabled(filterSwitch.filter)
        }

        Component.onCompleted: refresh()

        Connections {
            target: root
            function onRefreshRequested() { filterSwitch.refresh() }
        }

        Text {
            id: switchLabel

            Layout.fillWidth: true

            color: theme.fg.secondary
            font.pixelSize: VLCStyle.fontSize_normal
            elide: Text.ElideRight
        }

        RowLayout {
            id: extraRow
            spacing: VLCStyle.margin_xsmall
        }

        EffectsSwitch {
            id: videoSwitch

            Accessible.name: switchLabel.text

            onToggled: {
                PlayerEffects.setVideoFilterEnabled(filterSwitch.filter, checked)
                filterSwitch.refresh()
            }
        }
    }

    // a line with a text and a switch for a boolean option of a video filter
    component VideoOptionSwitch: RowLayout {
        id: optionSwitch

        required property string option
        property alias text: optionLabel.text
        property bool active: true

        Layout.fillWidth: true
        spacing: VLCStyle.margin_xsmall

        function refresh() {
            optionToggle.checked = !!PlayerEffects.videoOption(optionSwitch.option)
        }

        Component.onCompleted: refresh()

        Connections {
            target: root
            function onRefreshRequested() { optionSwitch.refresh() }
        }

        Text {
            id: optionLabel

            Layout.fillWidth: true

            color: theme.fg.secondary
            font.pixelSize: VLCStyle.fontSize_normal
            elide: Text.ElideRight
        }

        EffectsSwitch {
            id: optionToggle

            opacity: optionSwitch.active ? 1.0 : 0.6

            Accessible.name: optionLabel.text

            onToggled: PlayerEffects.setVideoOption(optionSwitch.option, checked)
        }
    }

    // a color stored in an integer option, e.g. "#FF0000"
    component ColorField: Rectangle {
        id: colorField

        required property string option

        readonly property color _color: "#" + ("000000" + Math.max(0, value).toString(16)).slice(-6)
        property int value: 0

        implicitWidth: VLCStyle.dp(116, VLCStyle.scale)
        implicitHeight: VLCStyle.dp(32, VLCStyle.scale)

        radius: VLCStyle.dp(4, VLCStyle.scale)
        color: sliderTheme.bg.secondary
        border.width: VLCStyle.margin_xxxsmall
        border.color: colorInput.activeFocus ? sliderTheme.fg.primary : windowTheme.bg.secondary

        function refresh() {
            const value = PlayerEffects.videoOption(colorField.option)
            if (value !== undefined)
                colorField.value = value
        }

        Component.onCompleted: refresh()

        Connections {
            target: root
            function onRefreshRequested() { colorField.refresh() }
        }

        TextInput {
            id: colorInput

            anchors {
                left: parent.left
                right: swatch.left
                verticalCenter: parent.verticalCenter
                leftMargin: VLCStyle.margin_small
                rightMargin: VLCStyle.margin_xxsmall
            }

            clip: true
            selectByMouse: true
            color: theme.fg.primary
            selectionColor: sliderTheme.bg.highlight
            font.pixelSize: VLCStyle.fontSize_normal

            validator: RegularExpressionValidator { regularExpression: /^#?[0-9a-fA-F]{0,6}$/ }

            Accessible.name: qsTr("Color")

            Binding on text {
                when: !colorInput.activeFocus
                value: colorField._color.toString().toUpperCase()
                restoreMode: Binding.RestoreNone
            }

            onEditingFinished: {
                const hex = text.replace("#", "")
                if (hex.length === 6 || hex.length === 3) {
                    const full = (hex.length === 3) ? hex.split("").map((c) => c + c).join("") : hex
                    colorField.value = parseInt(full, 16)
                    PlayerEffects.setVideoOption(colorField.option, colorField.value)
                }
                text = colorField._color.toString().toUpperCase()
            }
        }

        Rectangle {
            id: swatch

            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
                rightMargin: VLCStyle.margin_xsmall
            }

            width: VLCStyle.dp(20, VLCStyle.scale)
            height: width
            radius: VLCStyle.margin_xxxsmall
            color: colorField._color
        }
    }

    // a crop value in pixels
    component CropField: Rectangle {
        id: cropField

        required property string option
        property alias text: cropLabel.text
        property int value: 0

        signal edited(int value)

        Layout.fillWidth: true
        implicitHeight: VLCStyle.dp(32, VLCStyle.scale)

        radius: VLCStyle.dp(4, VLCStyle.scale)
        color: sliderTheme.bg.secondary
        border.width: cropInput.activeFocus ? VLCStyle.margin_xxxsmall : 0
        border.color: sliderTheme.fg.primary

        opacity: enabled ? 1.0 : 0.5

        function refresh() {
            const value = PlayerEffects.videoOption(cropField.option)
            cropField.value = (value !== undefined) ? value : 0
        }

        Component.onCompleted: refresh()

        Connections {
            target: root
            function onRefreshRequested() { cropField.refresh() }
        }

        RowLayout {
            anchors {
                fill: parent
                leftMargin: VLCStyle.margin_small
                rightMargin: VLCStyle.margin_small
            }

            spacing: VLCStyle.margin_xsmall

            Text {
                id: cropLabel

                Layout.fillWidth: true

                color: theme.fg.secondary
                font.pixelSize: VLCStyle.fontSize_normal
                elide: Text.ElideRight
            }

            TextInput {
                id: cropInput

                Layout.preferredWidth: Math.max(contentWidth, VLCStyle.dp(12, VLCStyle.scale))

                horizontalAlignment: TextInput.AlignRight
                selectByMouse: true
                color: theme.fg.primary
                selectionColor: sliderTheme.bg.highlight
                font.pixelSize: VLCStyle.fontSize_normal

                validator: IntValidator { bottom: 0; top: 99999 }

                Accessible.name: cropLabel.text

                Binding on text {
                    when: !cropInput.activeFocus
                    value: cropField.value.toString()
                    restoreMode: Binding.RestoreNone
                }

                onEditingFinished: {
                    const value = parseInt(text)
                    if (!isNaN(value) && value !== cropField.value) {
                        cropField.value = value
                        PlayerEffects.setVideoOption(cropField.option, value)
                        cropField.edited(value)
                    }
                    text = cropField.value.toString()
                }
            }

            Text {
                color: theme.fg.secondary
                opacity: 0.6
                font.pixelSize: VLCStyle.fontSize_normal
                text: qsTr("px")
            }
        }
    }

    // links two crop values
    component LinkButton: T.AbstractButton {
        id: linkButton

        checkable: true

        implicitWidth: VLCStyle.dp(24, VLCStyle.scale)
        implicitHeight: VLCStyle.dp(32, VLCStyle.scale)

        focusPolicy: Qt.StrongFocus

        Accessible.name: checked ? qsTr("Unlink the values") : qsTr("Link the values")

        background: null
        contentItem: Image {
            fillMode: Image.Pad
            horizontalAlignment: Image.AlignHCenter
            verticalAlignment: Image.AlignVCenter
            sourceSize: Qt.size(VLCStyle.dp(16, VLCStyle.scale), VLCStyle.dp(16, VLCStyle.scale))
            source: SVGColorImage.colorize(linkButton.checked ? "qrc:///icons/link.svg" : "qrc:///icons/link_off.svg")
                .color1((linkButton.checked || linkButton.visualFocus) ? sliderTheme.fg.primary
                                                                      : (linkButton.hovered ? theme.fg.primary : theme.fg.secondary))
                .uri()
        }
    }

    component Separator: Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: -VLCStyle.margin_normal
        Layout.rightMargin: -VLCStyle.margin_normal
        implicitHeight: VLCStyle.border
        color: theme.separator
    }

    // tab of the segmented control ("Values" in the design)
    component TabButton: T.AbstractButton {
        id: tabButton

        property bool selected: false

        Accessible.role: Accessible.PageTab
        Accessible.checked: selected

        Layout.fillWidth: true
        implicitHeight: VLCStyle.dp(36, VLCStyle.scale)

        focusPolicy: Qt.StrongFocus

        background: Rectangle {
            radius: VLCStyle.dp(4, VLCStyle.scale)
            color: tabButton.selected ? sliderTheme.bg.highlight : windowTheme.bg.secondary
            border.width: tabButton.visualFocus ? VLCStyle.focus_border : 0
            border.color: theme.visualFocus
        }

        contentItem: Text {
            text: tabButton.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: VLCStyle.fontSize_normal
            color: tabButton.selected ? sliderTheme.fg.primary
                                     : (tabButton.hovered ? theme.fg.primary : theme.fg.secondary)
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: VLCStyle.dp(20, VLCStyle.scale)
            Layout.rightMargin: VLCStyle.dp(20, VLCStyle.scale) - VLCStyle.margin_xxsmall
            Layout.topMargin: VLCStyle.margin_small

            Widgets.SubtitleLabel {
                Layout.fillWidth: true

                text: qsTr("Effects")
                color: theme.fg.primary
                font.weight: Font.Normal
                font.pixelSize: VLCStyle.fontSize_large

                Accessible.role: Accessible.Heading
            }

            Widgets.IconToolButton {
                text: VLCIcons.close
                description: qsTr("Close")

                onClicked: PlayerEffects.panelVisible = false
            }
        }

        // segmented control
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: VLCStyle.dp(20, VLCStyle.scale)
            Layout.rightMargin: VLCStyle.dp(20, VLCStyle.scale)
            Layout.topMargin: VLCStyle.margin_xsmall
            Layout.bottomMargin: VLCStyle.dp(20, VLCStyle.scale)

            implicitHeight: tabRow.implicitHeight + 2 * VLCStyle.margin_xxxsmall

            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: sliderTheme.bg.secondary

            RowLayout {
                id: tabRow

                anchors.fill: parent
                anchors.margins: VLCStyle.margin_xxxsmall

                spacing: VLCStyle.margin_xxxsmall

                TabButton {
                    text: qsTr("Audio")
                    selected: root.currentTab === 0
                    focus: true
                    onClicked: root.currentTab = 0
                }

                TabButton {
                    text: qsTr("Video")
                    selected: root.currentTab === 1
                    onClicked: root.currentTab = 1
                }
            }
        }

        Flickable {
            id: flickable

            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            contentWidth: width
            contentHeight: (root.currentTab === 0 ? audioColumn.implicitHeight : videoColumn.implicitHeight)
                           + VLCStyle.dp(20, VLCStyle.scale)
            boundsBehavior: Flickable.StopAtBounds

            T.ScrollBar.vertical: Widgets.ScrollBarExt { }

            // Audio
            ColumnLayout {
                id: audioColumn

                visible: root.currentTab === 0

                x: VLCStyle.dp(20, VLCStyle.scale)
                width: flickable.width - 2 * x

                spacing: VLCStyle.dp(20, VLCStyle.scale)

                AudioSection {
                    id: equalizerSection

                    title: qsTr("Equalizer")
                    filter: "equalizer"
                    expanded: true

                    Widgets.ComboBoxExt {
                        id: presetCombo

                        Layout.fillWidth: true
                        Layout.preferredHeight: VLCStyle.dp(40, VLCStyle.scale)

                        font.pixelSize: VLCStyle.fontSize_normal

                        model: [qsTr("None")].concat(PlayerEffects.equalizerPresets)

                        Accessible.name: qsTr("Preset")

                        function refresh() {
                            presetCombo.currentIndex = PlayerEffects.equalizerPreset() + 1
                        }

                        Component.onCompleted: refresh()

                        onActivated: (index) => {
                            if (index > 0)
                                PlayerEffects.applyEqualizerPreset(index - 1)
                        }

                        Connections {
                            target: root
                            function onRefreshRequested() { presetCombo.refresh() }
                        }
                    }

                    AudioSlider {
                        text: qsTr("Preamp")
                        option: "equalizer-preamp"
                        from: -20; to: 20; stepSize: 0.1
                        defaultValue: 12
                        unit: qsTr(" dB")
                        editable: true
                        tickCount: 0
                        active: equalizerSection.checked

                        // after the handler of AudioSlider that sets the option
                        onMoved: Qt.callLater(presetCombo.refresh)
                    }

                    Repeater {
                        model: PlayerEffects.equalizerBandNames

                        delegate: EffectsSlider {
                            id: bandSlider

                            required property int index
                            required property string modelData

                            Layout.fillWidth: true

                            text: modelData
                            from: -20; to: 20; stepSize: 0.1
                            defaultValue: 0
                            unit: qsTr(" dB")
                            editable: true
                            tickCount: 0
                            active: equalizerSection.checked

                            function refresh() {
                                const bands = PlayerEffects.equalizerBands()
                                bandSlider.value = bands[bandSlider.index]
                            }

                            Component.onCompleted: refresh()

                            onMoved: (value) => {
                                PlayerEffects.setEqualizerBand(bandSlider.index, value)
                                presetCombo.refresh()
                            }

                            Connections {
                                target: root
                                function onRefreshRequested() { bandSlider.refresh() }
                            }
                        }
                    }
                }

                AudioSection {
                    id: compressorSection

                    title: qsTr("Compressor")
                    filter: "compressor"

                    AudioSlider {
                        text: qsTr("RMS / Peak")
                        option: "compressor-rms-peak"
                        from: 0; to: 1; stepSize: 0.001
                        defaultValue: 0.2
                        active: compressorSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Attack")
                        option: "compressor-attack"
                        from: 1.5; to: 400; stepSize: 0.1
                        defaultValue: 25
                        unit: qsTr(" ms")
                        active: compressorSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Release")
                        option: "compressor-release"
                        from: 2; to: 800; stepSize: 0.1
                        defaultValue: 100
                        unit: qsTr(" ms")
                        active: compressorSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Threshold")
                        option: "compressor-threshold"
                        from: -30; to: 0; stepSize: 0.01
                        defaultValue: -11
                        unit: qsTr(" dB")
                        active: compressorSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Ratio")
                        option: "compressor-ratio"
                        from: 1; to: 20; stepSize: 0.01
                        defaultValue: 4
                        unit: qsTr(" : 1")
                        active: compressorSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Knee Radius")
                        option: "compressor-knee"
                        from: 1; to: 10; stepSize: 0.01
                        defaultValue: 5
                        unit: qsTr(" dB")
                        active: compressorSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Makeup Gain")
                        option: "compressor-makeup-gain"
                        from: 0; to: 24; stepSize: 0.01
                        defaultValue: 7
                        unit: qsTr(" dB")
                        active: compressorSection.checked
                    }
                }

                AudioSection {
                    id: spatializerSection

                    title: qsTr("Spatializer")
                    filter: "spatializer"

                    AudioSlider {
                        text: qsTr("Size")
                        option: "spatializer-roomsize"
                        from: 0; to: 1.1; stepSize: 0.1
                        defaultValue: 0.85
                        displayScale: 10
                        active: spatializerSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Width")
                        option: "spatializer-width"
                        from: 0; to: 1; stepSize: 0.1
                        defaultValue: 1
                        displayScale: 10
                        active: spatializerSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Wet")
                        option: "spatializer-wet"
                        from: 0; to: 1; stepSize: 0.1
                        defaultValue: 0.4
                        displayScale: 10
                        active: spatializerSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Dry")
                        option: "spatializer-dry"
                        from: 0; to: 1; stepSize: 0.1
                        defaultValue: 0.5
                        displayScale: 10
                        active: spatializerSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Damp")
                        option: "spatializer-damp"
                        from: 0; to: 1; stepSize: 0.1
                        defaultValue: 0.5
                        displayScale: 10
                        active: spatializerSection.checked
                    }
                }

                AudioSection {
                    id: stereoWidenerSection

                    title: qsTr("Stereo Widener")
                    filter: "stereo_widen"

                    AudioSlider {
                        text: qsTr("Delay Time")
                        option: "stereowiden-delay"
                        from: 1; to: 100; stepSize: 1
                        defaultValue: 20
                        unit: qsTr(" ms")
                        tickCount: 0
                        active: stereoWidenerSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Feedback Gain")
                        option: "stereowiden-feedback"
                        from: 0; to: 0.9; stepSize: 0.1
                        defaultValue: 0.3
                        displayScale: 100; decimals: 0
                        unit: qsTr(" %")
                        active: stereoWidenerSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Crossfeed")
                        option: "stereowiden-crossfeed"
                        from: 0; to: 0.8; stepSize: 0.1
                        defaultValue: 0.3
                        displayScale: 100; decimals: 0
                        unit: qsTr(" %")
                        active: stereoWidenerSection.checked
                    }
                    AudioSlider {
                        text: qsTr("Dry Mix")
                        option: "stereowiden-dry-mix"
                        from: 0; to: 1; stepSize: 0.1
                        defaultValue: 0.8
                        displayScale: 100; decimals: 0
                        unit: qsTr(" %")
                        active: stereoWidenerSection.checked
                    }
                }
            }

            // Video
            ColumnLayout {
                id: videoColumn

                visible: root.currentTab === 1

                x: VLCStyle.dp(20, VLCStyle.scale)
                width: flickable.width - 2 * x

                spacing: VLCStyle.dp(20, VLCStyle.scale)

                EffectsSection {
                    id: essentialSection

                    Layout.fillWidth: true

                    title: qsTr("Essential")
                    expanded: true

                    // the image adjust filter is enabled while one of its settings
                    // is not neutral
                    property bool adjustEnabled: false

                    function refresh() {
                        essentialSection.adjustEnabled = PlayerEffects.isVideoFilterEnabled("adjust")
                    }

                    function updateAdjust() {
                        const neutral = Math.abs(hueSlider.value) < 0.5
                                     && Math.abs(brightnessSlider.value - 1) < 0.005
                                     && Math.abs(contrastSlider.value - 1) < 0.005
                                     && Math.abs(saturationSlider.value - 1) < 0.005
                                     && Math.abs(gammaSlider.value - 1) < 0.005
                        if (neutral === essentialSection.adjustEnabled) {
                            PlayerEffects.setVideoFilterEnabled("adjust", !neutral)
                            refresh()
                        }
                    }

                    Component.onCompleted: refresh()

                    Connections {
                        target: root
                        function onRefreshRequested() { essentialSection.refresh() }
                    }

                    VideoSlider {
                        id: hueSlider
                        text: qsTr("Hue")
                        option: "hue"
                        from: -180; to: 180; stepSize: 1
                        defaultValue: 0
                        decimals: 0
                        unit: "°"
                        active: essentialSection.adjustEnabled
                        onMoved: Qt.callLater(essentialSection.updateAdjust)
                    }
                    VideoSlider {
                        id: brightnessSlider
                        text: qsTr("Brightness")
                        option: "brightness"
                        from: 0; to: 2; stepSize: 0.01
                        defaultValue: 1
                        decimals: 2
                        active: essentialSection.adjustEnabled
                        onMoved: Qt.callLater(essentialSection.updateAdjust)
                    }
                    VideoSlider {
                        id: contrastSlider
                        text: qsTr("Contrast")
                        option: "contrast"
                        from: 0; to: 2; stepSize: 0.01
                        defaultValue: 1
                        decimals: 2
                        active: essentialSection.adjustEnabled
                        onMoved: Qt.callLater(essentialSection.updateAdjust)
                    }
                    VideoSlider {
                        id: saturationSlider
                        text: qsTr("Saturation")
                        option: "saturation"
                        from: 0; to: 3; stepSize: 0.01
                        defaultValue: 1
                        decimals: 2
                        tickCount: 3
                        active: essentialSection.adjustEnabled
                        onMoved: Qt.callLater(essentialSection.updateAdjust)
                    }
                    VideoSlider {
                        id: gammaSlider
                        text: qsTr("Gamma")
                        option: "gamma"
                        from: 0.01; to: 10; stepSize: 0.01
                        defaultValue: 1
                        decimals: 2
                        tickCount: 10
                        active: essentialSection.adjustEnabled
                        onMoved: Qt.callLater(essentialSection.updateAdjust)
                    }

                    AutoFilterSlider {
                        text: qsTr("Sharpen Sigma")
                        option: "sharpen-sigma"
                        filter: "sharpen"
                        from: 0; to: 2; stepSize: 0.01
                        decimals: 2
                        tickCount: 4
                    }
                    AutoFilterSlider {
                        text: qsTr("Banding Removal Radius")
                        option: "gradfun-radius"
                        filter: "gradfun"
                        // 2 is "Off", the filter takes 4 to 32
                        from: 2; to: 32; stepSize: 2
                        neutralValue: 2
                        decimals: 0
                        tickCount: 0
                    }
                    AutoFilterSlider {
                        text: qsTr("Film Grain Variance")
                        option: "grain-variance"
                        filter: "grain"
                        from: 0; to: 10; stepSize: 0.1
                        tickCount: 10
                    }
                }

                EffectsSection {
                    id: cropSection

                    Layout.fillWidth: true

                    title: qsTr("Crop")

                    enabled: Player.hasVideoOutput

                    GridLayout {
                        Layout.fillWidth: true

                        columns: 3
                        columnSpacing: VLCStyle.margin_xsmall
                        rowSpacing: VLCStyle.margin_normal

                        CropField {
                            id: cropTop
                            text: qsTr("Top")
                            option: "crop-top"
                            onEdited: (value) => {
                                if (topBottomLink.checked) {
                                    cropBottom.value = value
                                    PlayerEffects.setVideoOption("crop-bottom", value)
                                }
                            }
                        }
                        CropField {
                            id: cropBottom
                            text: qsTr("Bottom")
                            option: "crop-bottom"
                            enabled: !topBottomLink.checked
                        }
                        LinkButton {
                            id: topBottomLink
                            onToggled: {
                                if (checked) {
                                    cropBottom.value = cropTop.value
                                    PlayerEffects.setVideoOption("crop-bottom", cropTop.value)
                                }
                            }
                        }

                        CropField {
                            id: cropLeft
                            text: qsTr("Left")
                            option: "crop-left"
                            onEdited: (value) => {
                                if (leftRightLink.checked) {
                                    cropRight.value = value
                                    PlayerEffects.setVideoOption("crop-right", value)
                                }
                            }
                        }
                        CropField {
                            id: cropRight
                            text: qsTr("Right")
                            option: "crop-right"
                            enabled: !leftRightLink.checked
                        }
                        LinkButton {
                            id: leftRightLink
                            onToggled: {
                                if (checked) {
                                    cropRight.value = cropLeft.value
                                    PlayerEffects.setVideoOption("crop-right", cropLeft.value)
                                }
                            }
                        }
                    }
                }

                EffectsSection {
                    id: colorsSection

                    Layout.fillWidth: true

                    title: qsTr("Colors")

                    VideoFilterSwitch {
                        text: qsTr("Color Extraction")
                        filter: "extract"

                        ColorField { option: "extract-component" }
                    }

                    Separator { }

                    VideoFilterSwitch {
                        id: colorThreshold
                        text: qsTr("Color Threshold")
                        filter: "colorthres"

                        ColorField { option: "colorthres-color" }
                    }
                    VideoSlider {
                        text: qsTr("Saturation")
                        option: "colorthres-saturationthres"
                        from: 0; to: 100; stepSize: 1
                        defaultValue: 20
                        decimals: 0
                        tickCount: 0
                        active: colorThreshold.checked
                    }
                    VideoSlider {
                        text: qsTr("Similarity")
                        option: "colorthres-similaritythres"
                        from: 0; to: 100; stepSize: 1
                        defaultValue: 15
                        decimals: 0
                        tickCount: 0
                        active: colorThreshold.checked
                    }

                    Separator { }

                    VideoFilterSwitch {
                        text: qsTr("Negate Colors")
                        filter: "invert"
                    }
                    VideoFilterSwitch {
                        id: posterize
                        text: qsTr("Posterize")
                        filter: "posterize"
                    }
                    VideoSlider {
                        text: qsTr("Posterize Level")
                        option: "posterize-level"
                        from: 2; to: 256; stepSize: 1
                        defaultValue: 6
                        decimals: 0
                        tickCount: 0
                        active: posterize.checked
                    }

                    Separator { }

                    VideoFilterSwitch {
                        id: gradient
                        text: qsTr("Gradient")
                        filter: "gradient"
                    }
                    Widgets.ComboBoxExt {
                        id: gradientMode

                        Layout.fillWidth: true
                        Layout.preferredHeight: VLCStyle.dp(40, VLCStyle.scale)

                        font.pixelSize: VLCStyle.fontSize_normal

                        readonly property var _info: PlayerEffects.optionInfo("gradient-mode")

                        model: _info.texts ?? []

                        Accessible.name: qsTr("Gradient mode")

                        function refresh() {
                            const values = gradientMode._info.values ?? []
                            gradientMode.currentIndex = Math.max(0, values.indexOf(PlayerEffects.videoOption("gradient-mode")))
                        }

                        Component.onCompleted: refresh()

                        onActivated: (index) => {
                            const values = gradientMode._info.values ?? []
                            if (index >= 0 && index < values.length)
                                PlayerEffects.setVideoOption("gradient-mode", values[index])
                        }

                        Connections {
                            target: root
                            function onRefreshRequested() { gradientMode.refresh() }
                        }
                    }
                    VideoOptionSwitch {
                        text: qsTr("Color")
                        option: "gradient-color"
                        active: gradient.checked
                    }
                    VideoOptionSwitch {
                        text: qsTr("Cartoon")
                        option: "gradient-cartoon"
                        active: gradient.checked
                    }

                    Separator { }

                    AutoFilterSlider {
                        text: qsTr("Sepia Intensity")
                        option: "sepia-intensity"
                        filter: "sepia"
                        from: 0; to: 255; stepSize: 1
                        decimals: 0
                        tickCount: 0
                    }

                }
            }
        }
    }
}
