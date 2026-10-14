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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "effects_controller.hpp"

#include <QLocale>

#include <algorithm>
#include <cassert>
#include <cmath>

#include <vlc_common.h>
#include <vlc_aout.h>
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_player.h>
#include <vlc_plugin.h>
#include <vlc_variables.h>
#include <vlc_vout.h>

#include "player/player_controller.hpp"

#include "../../../audio_filter/equalizer_presets.h"

namespace {

constexpr int EQUALIZER_BANDS = 10;

QStringList chainItems(const char* chain)
{
    return QString::fromUtf8(chain ? chain : "").split(QLatin1Char(':'), Qt::SkipEmptyParts);
}

QString changeChain(const char* chain, const QString& name, bool add)
{
    QStringList items = chainItems(chain);
    if (add && !items.contains(name))
        items << name;
    else if (!add)
        items.removeAll(name);
    return items.join(QLatin1Char(':'));
}

const char* videoFilterType(const char* name)
{
    module_t* module = module_find(name);
    if (!module)
        return nullptr;

    if (module_provides(module, "video splitter"))
        return "video-splitter";
    if (module_provides(module, "video filter"))
        return "video-filter";
    if (module_provides(module, "sub source"))
        return "sub-source";
    if (module_provides(module, "sub filter"))
        return "sub-filter";
    return nullptr;
}

QString bandName(float frequency)
{
    if (frequency < 1000.f)
        return QStringLiteral("%1 Hz").arg(frequency, 0, 'f', 0);
    return QStringLiteral("%1 KHz").arg(frequency / 1000.f, 0, 'g', 3);
}

} // anonymous namespace

EffectsController::EffectsController(qt_intf_t* intf, QObject* parent)
    : QObject(parent)
    , m_intf(intf)
{
    assert(m_intf);
}

bool EffectsController::isAudioFilterEnabled(const QString& name) const
{
    SharedAOut aout = m_intf->p_mainPlayerController->getAout();
    char* chain = aout ? var_GetNonEmptyString(aout.get(), "audio-filter")
                       : var_InheritString(m_intf, "audio-filter");
    const bool enabled = chainItems(chain).contains(name);
    free(chain);
    return enabled;
}

void EffectsController::setAudioFilterEnabled(const QString& name, bool enabled)
{
    const QByteArray module = name.toUtf8();
    if (!module_exists(module.constData()))
    {
        msg_Err(m_intf, "Unable to find filter module \"%s\".", module.constData());
        return;
    }

    // the equalizer does not start without bands (when no preset was chosen yet)
    if (enabled && name == QLatin1String("equalizer"))
        storeBands(bands());

    char* chain = config_GetPsz("audio-filter");
    const QString result = changeChain(chain, name, enabled);
    free(chain);
    config_PutPsz("audio-filter", qtu(result));

    vlc_player_aout_EnableFilter(m_intf->p_player, module.constData(), enabled);
}

double EffectsController::audioOption(const QString& option) const
{
    const QByteArray name = option.toUtf8();

    SharedAOut aout = m_intf->p_mainPlayerController->getAout();
    if (aout && (var_Type(aout.get(), name.constData()) & VLC_VAR_CLASS) == VLC_VAR_FLOAT)
        return var_GetFloat(aout.get(), name.constData());

    if (config_FindConfig(name.constData()))
        return config_GetFloat(name.constData());
    return 0.0;
}

void EffectsController::setAudioOption(const QString& option, double value)
{
    const QByteArray name = option.toUtf8();

    SharedAOut aout = m_intf->p_mainPlayerController->getAout();
    if (aout && var_Type(aout.get(), name.constData()) != 0)
        var_SetFloat(aout.get(), name.constData(), value);

    if (config_FindConfig(name.constData()))
        config_PutFloat(name.constData(), value);
}

QStringList EffectsController::bands() const
{
    QStringList result;

    SharedAOut aout = m_intf->p_mainPlayerController->getAout();
    if (aout && (var_Type(aout.get(), "equalizer-bands") & VLC_VAR_CLASS) == VLC_VAR_STRING)
    {
        char* value = var_GetString(aout.get(), "equalizer-bands");
        result = QString::fromUtf8(value ? value : "").split(QLatin1Char(' '), Qt::SkipEmptyParts);
        free(value);
    }

    if (result.isEmpty() && config_FindConfig("equalizer-bands"))
    {
        char* value = config_GetPsz("equalizer-bands");
        result = QString::fromUtf8(value ? value : "").split(QLatin1Char(' '), Qt::SkipEmptyParts);
        free(value);
    }

    while (result.size() < EQUALIZER_BANDS)
        result << QStringLiteral("0");
    return result;
}

void EffectsController::storeBands(const QStringList& bands)
{
    const QByteArray value = bands.join(QLatin1Char(' ')).toUtf8();

    SharedAOut aout = m_intf->p_mainPlayerController->getAout();
    if (aout && var_Type(aout.get(), "equalizer-bands") != 0)
        var_SetString(aout.get(), "equalizer-bands", value.constData());

    if (config_FindConfig("equalizer-bands"))
        config_PutPsz("equalizer-bands", value.constData());
}

QVariantList EffectsController::equalizerBands() const
{
    const QLocale c(QLocale::C);
    QVariantList result;
    const QStringList values = bands();
    for (int i = 0; i < EQUALIZER_BANDS; ++i)
        result << c.toDouble(values[i]);
    return result;
}

void EffectsController::setEqualizerBand(int index, double value)
{
    if (index < 0 || index >= EQUALIZER_BANDS)
        return;

    QStringList values = bands();
    values[index] = QLocale(QLocale::C).toString(value, 'f', 1);
    storeBands(values);
}

int EffectsController::equalizerPreset() const
{
    const QVariantList current = equalizerBands();
    const double preamp = audioOption(QStringLiteral("equalizer-preamp"));

    for (int i = 0; i < NB_PRESETS; ++i)
    {
        const eqz_preset_t& preset = eqz_preset_10b[i];
        if (std::abs(preset.f_preamp - preamp) > 0.05)
            continue;

        bool same = true;
        for (int band = 0; band < std::min(preset.i_band, EQUALIZER_BANDS); ++band)
        {
            if (std::abs(preset.f_amp[band] - current[band].toDouble()) > 0.05)
            {
                same = false;
                break;
            }
        }
        if (same)
            return i;
    }
    return -1;
}

void EffectsController::applyEqualizerPreset(int index)
{
    if (index < 0 || index >= NB_PRESETS)
        return;

    const eqz_preset_t& preset = eqz_preset_10b[index];

    // the preset first: the equalizer applies it to the bands and the preamp,
    // which are then set again with the same values
    SharedAOut aout = m_intf->p_mainPlayerController->getAout();
    if (aout && var_Type(aout.get(), "equalizer-preset") != 0)
        var_SetString(aout.get(), "equalizer-preset", preset_list[index]);
    if (config_FindConfig("equalizer-preset"))
        config_PutPsz("equalizer-preset", preset_list[index]);

    setAudioOption(QStringLiteral("equalizer-preamp"), preset.f_preamp);

    const QLocale c(QLocale::C);
    QStringList values;
    for (int band = 0; band < EQUALIZER_BANDS; ++band)
        values << c.toString(band < preset.i_band ? preset.f_amp[band] : 0.f, 'f', 1);
    storeBands(values);

    emit equalizerChanged();
}

QStringList EffectsController::equalizerPresets() const
{
    QStringList result;
    for (int i = 0; i < NB_PRESETS; ++i)
        result << qfut(preset_list_text[i]);
    return result;
}

QStringList EffectsController::equalizerBandNames() const
{
    const bool vlcFrequencies = var_InheritBool(m_intf, "equalizer-vlcfreqs");
    const float* frequencies = vlcFrequencies ? f_vlc_frequency_table_10b : f_iso_frequency_table_10b;

    QStringList result;
    for (int i = 0; i < EQUALIZER_BANDS; ++i)
        result << bandName(frequencies[i]);
    return result;
}

bool EffectsController::isVideoFilterEnabled(const QString& name) const
{
    const QByteArray module = name.toUtf8();
    const char* type = videoFilterType(module.constData());
    if (!type)
        return false;

    SharedVOutThread vout = m_intf->p_mainPlayerController->getVout();
    char* chain = vout ? var_GetString(vout.get(), type) : var_InheritString(m_intf, type);
    const bool enabled = chainItems(chain).contains(name);
    free(chain);
    return enabled;
}

void EffectsController::setVideoFilterEnabled(const QString& name, bool enabled)
{
    const QByteArray module = name.toUtf8();
    const char* type = videoFilterType(module.constData());
    if (!type)
    {
        msg_Err(m_intf, "Unable to find video filter module \"%s\".", module.constData());
        return;
    }

    SharedVOutThread vout = m_intf->p_mainPlayerController->getVout();
    char* chain = vout ? var_GetString(vout.get(), type) : config_GetPsz(type);
    const QByteArray result = changeChain(chain, name, enabled).toUtf8();
    free(chain);

    config_PutPsz(type, result.constData());

    // the splitters can not be changed on the fly
    if (vout && strcmp(type, "video-splitter") != 0)
        var_SetString(vout.get(), type, result.constData());
}

QVariant EffectsController::videoOption(const QString& option) const
{
    const QByteArray name = option.toUtf8();

    SharedVOutThread vout = m_intf->p_mainPlayerController->getVout();
    if (vout)
    {
        switch (var_Type(vout.get(), name.constData()) & VLC_VAR_CLASS)
        {
        case VLC_VAR_FLOAT:
            return static_cast<double>(var_GetFloat(vout.get(), name.constData()));
        case VLC_VAR_INTEGER:
            return static_cast<qlonglong>(var_GetInteger(vout.get(), name.constData()));
        case VLC_VAR_BOOL:
            return var_GetBool(vout.get(), name.constData());
        case VLC_VAR_STRING:
        {
            char* value = var_GetString(vout.get(), name.constData());
            const QString result = QString::fromUtf8(value ? value : "");
            free(value);
            return result;
        }
        default:
            break;
        }
    }

    switch (config_GetType(name.constData()))
    {
    case VLC_VAR_FLOAT:
        return static_cast<double>(config_GetFloat(name.constData()));
    case VLC_VAR_INTEGER:
        return static_cast<qlonglong>(config_GetInt(name.constData()));
    case VLC_VAR_BOOL:
        return config_GetInt(name.constData()) != 0;
    case VLC_VAR_STRING:
    {
        char* value = config_GetPsz(name.constData());
        const QString result = QString::fromUtf8(value ? value : "");
        free(value);
        return result;
    }
    default:
        return {};
    }
}

void EffectsController::setVideoOption(const QString& option, const QVariant& value)
{
    const QByteArray name = option.toUtf8();

    SharedVOutThread vout = m_intf->p_mainPlayerController->getVout();
    int type = vout ? (var_Type(vout.get(), name.constData()) & VLC_VAR_CLASS) : 0;
    const int configType = config_GetType(name.constData());
    if (type == 0)
        type = configType;

    vlc_value_t val;
    QByteArray string;
    switch (type)
    {
    case VLC_VAR_FLOAT:
        val.f_float = value.toFloat();
        if (configType == VLC_VAR_FLOAT)
            config_PutFloat(name.constData(), val.f_float);
        break;
    case VLC_VAR_INTEGER:
        val.i_int = value.toLongLong();
        if (configType == VLC_VAR_INTEGER)
            config_PutInt(name.constData(), val.i_int);
        break;
    case VLC_VAR_BOOL:
        val.b_bool = value.toBool();
        if (configType == VLC_VAR_BOOL)
            config_PutInt(name.constData(), val.b_bool);
        break;
    case VLC_VAR_STRING:
        string = value.toString().toUtf8();
        val.psz_string = string.data();
        if (configType == VLC_VAR_STRING)
            config_PutPsz(name.constData(), string.constData());
        break;
    default:
        msg_Err(m_intf, "Unsupported type for the video option \"%s\".", name.constData());
        return;
    }

    if (vout && var_Type(vout.get(), name.constData()) != 0)
        var_SetChecked(vout.get(), name.constData(), type, val);
}

void EffectsController::setPanelVisible(bool visible)
{
    if (m_panelVisible == visible)
        return;
    m_panelVisible = visible;
    emit panelVisibleChanged();
}

QVariantMap EffectsController::optionInfo(const QString& option) const
{
    const module_config_t* item = config_FindConfig(qtu(option));
    if (!item)
        return {};

    QVariantMap info;
    if (item->psz_text)
        info.insert(QStringLiteral("text"), qtr(item->psz_text));

    if (IsConfigFloatType(item->i_type))
    {
        info.insert(QStringLiteral("min"), item->min.f);
        info.insert(QStringLiteral("max"), item->max.f);
        info.insert(QStringLiteral("default"), item->orig.f);
    }
    else if (IsConfigIntegerType(item->i_type))
    {
        info.insert(QStringLiteral("min"), static_cast<qlonglong>(item->min.i));
        info.insert(QStringLiteral("max"), static_cast<qlonglong>(item->max.i));
        info.insert(QStringLiteral("default"), static_cast<qlonglong>(item->orig.i));
    }
    else if (IsConfigStringType(item->i_type))
    {
        info.insert(QStringLiteral("default"), qfu(item->orig.psz));
    }

    // choices of the option: "values" and their "texts"
    if (item->list_count > 0)
    {
        QVariantList values;
        QStringList texts;
        for (int i = 0; i < item->list_count; ++i)
        {
            if (IsConfigStringType(item->i_type))
                values << qfu(item->list.psz[i]);
            else if (IsConfigIntegerType(item->i_type))
                values << static_cast<qlonglong>(item->list.i[i]);
            else
                break;
            texts << ((item->list_text && item->list_text[i]) ? qtr(item->list_text[i]) : values.last().toString());
        }
        info.insert(QStringLiteral("values"), values);
        info.insert(QStringLiteral("texts"), texts);
    }
    return info;
}
