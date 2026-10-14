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
#ifndef VLC_QT_EFFECTS_CONTROLLER_HPP
#define VLC_QT_EFFECTS_CONTROLLER_HPP

#include <QObject>
#include <QStringList>
#include <QVariant>

#include "qt.hpp"

/**
 * Redesign: audio and video effects for the effects panel of the player
 * (see player/qml/EffectsPanel.qml).
 *
 * It mirrors what the "Adjustments and Effects" dialog does
 * (dialogs/extended/extended_panels.cpp): the filters are enabled in the
 * "audio-filter" / "video-filter" chains, and the options are applied to the
 * current audio output or video output when they exist. Unlike the dialog,
 * every change is also stored in the configuration right away.
 */
class EffectsController : public QObject
{
    Q_OBJECT

    // names of the equalizer presets (translated)
    Q_PROPERTY(QStringList equalizerPresets READ equalizerPresets CONSTANT FINAL)
    // names of the equalizer bands ("60 Hz", ...)
    Q_PROPERTY(QStringList equalizerBandNames READ equalizerBandNames CONSTANT FINAL)
    // whether the effects panel of the player is shown
    Q_PROPERTY(bool panelVisible READ isPanelVisible WRITE setPanelVisible NOTIFY panelVisibleChanged FINAL)

public:
    explicit EffectsController(qt_intf_t* intf, QObject* parent = nullptr);

    /// whether the filter module `name` is in the audio filter chain
    Q_INVOKABLE bool isAudioFilterEnabled(const QString& name) const;
    Q_INVOKABLE void setAudioFilterEnabled(const QString& name, bool enabled);

    /// float option of an audio filter, from the audio output or the configuration
    Q_INVOKABLE double audioOption(const QString& option) const;
    Q_INVOKABLE void setAudioOption(const QString& option, double value);

    Q_INVOKABLE QVariantList equalizerBands() const;
    Q_INVOKABLE void setEqualizerBand(int index, double value);
    /// index of the current preset in equalizerPresets, -1 when the bands were changed
    Q_INVOKABLE int equalizerPreset() const;
    Q_INVOKABLE void applyEqualizerPreset(int index);

    /// whether the filter module `name` is in its video filter chain
    Q_INVOKABLE bool isVideoFilterEnabled(const QString& name) const;
    Q_INVOKABLE void setVideoFilterEnabled(const QString& name, bool enabled);

    /// option of a video filter or of the video output (e.g. "crop-top")
    Q_INVOKABLE QVariant videoOption(const QString& option) const;
    Q_INVOKABLE void setVideoOption(const QString& option, const QVariant& value);

    /// description of a configuration option: "text", "min", "max", "default"
    Q_INVOKABLE QVariantMap optionInfo(const QString& option) const;

    QStringList equalizerPresets() const;
    QStringList equalizerBandNames() const;

    bool isPanelVisible() const { return m_panelVisible; }
    void setPanelVisible(bool visible);

signals:
    void panelVisibleChanged();
    /// the equalizer bands or preamp were changed at once (preset)
    void equalizerChanged();

private:
    QStringList bands() const;
    void storeBands(const QStringList& bands);

    qt_intf_t* m_intf = nullptr;
    bool m_panelVisible = false;
};

#endif // VLC_QT_EFFECTS_CONTROLLER_HPP
