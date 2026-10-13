/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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
#include "systempalette.hpp"
#include <QGuiApplication>
#include <QPalette>
#include <QSettings>
#include <QFile>
#include <maininterface/mainctx.hpp>
#include <vlc_modules.h>
#include <vlc_picture.h>

namespace {

QColor setColorAlpha(const QColor& c1, float alpha)
{
    QColor c(c1);
    c.setAlphaF(alpha);
    return c;
}

#ifndef _WIN32
/**
 * function taken from QtBase gui/platform/unix/qgenericunixservices.cpp
 */
static inline QByteArray detectLinuxDesktopEnvironment()
{
    const QByteArray xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
    if (!xdgCurrentDesktop.isEmpty())
        return xdgCurrentDesktop.toUpper(); // KDE, GNOME, UNITY, LXDE, MATE, XFCE...

    // Classic fallbacks
    if (!qEnvironmentVariableIsEmpty("KDE_FULL_SESSION"))
        return QByteArrayLiteral("KDE");
    if (!qEnvironmentVariableIsEmpty("GNOME_DESKTOP_SESSION_ID"))
        return QByteArrayLiteral("GNOME");

    // Fallback to checking $DESKTOP_SESSION (unreliable)
    QByteArray desktopSession = qgetenv("DESKTOP_SESSION");

    // This can be a path in /usr/share/xsessions
    int slash = desktopSession.lastIndexOf('/');
    if (slash != -1) {
        QSettings desktopFile(QFile::decodeName(desktopSession + ".desktop"), QSettings::IniFormat);
        desktopFile.beginGroup(QStringLiteral("Desktop Entry"));
        QByteArray desktopName = desktopFile.value(QStringLiteral("DesktopNames")).toByteArray();
        if (!desktopName.isEmpty())
            return desktopName;

        // try decoding just the basename
        desktopSession = desktopSession.mid(slash + 1);
    }

    if (desktopSession == "gnome")
        return QByteArrayLiteral("GNOME");
    else if (desktopSession == "xfce")
        return QByteArrayLiteral("XFCE");
    else if (desktopSession == "kde")
        return QByteArrayLiteral("KDE");

    return QByteArrayLiteral("UNKNOWN");
}

bool isGTKBasedEnvironment()
{
    QList<QByteArray> gtkBasedEnvironments{
        "GNOME",
        "X-CINNAMON",
        "UNITY",
        "MATE",
        "XFCE",
        "LXDE"
    };
    const QList<QByteArray> desktopNames = detectLinuxDesktopEnvironment().split(':');
    for (const QByteArray& desktopName: desktopNames)
    {
        if (gtkBasedEnvironments.contains(desktopName))
            return true;
    }
    return false;
}
#endif


static void PaletteChangedCallback(vlc_qt_theme_provider_t*, void* data)
{
    auto priv = static_cast<ExternalPaletteImpl*>(data);
    emit priv->paletteChanged();
}

static void MetricsChangedCallback(vlc_qt_theme_provider_t*, vlc_qt_theme_image_type type, void* data)
{
    auto priv = static_cast<ExternalPaletteImpl*>(data);
    assert(priv);
    priv->updateMetrics(type);
}


static void ReleaseVLCPictureCb(void* data)
{
    auto pic = static_cast<picture_t*>(data);
    if (pic)
        picture_Release(pic);
}


static void setColorRBGAInt(
    vlc_qt_theme_provider_t* obj,
    vlc_qt_theme_color_set set, vlc_qt_theme_color_section section,
    vlc_qt_theme_color_name name, vlc_qt_theme_color_state state,
    int r, int g, int b, int a)
{
    auto palette = static_cast<SystemPalette*>(obj->setColorData);
    QColor color(r,g,b,a);
    palette->setColor(
        static_cast<ColorContext::ColorSet>(set), static_cast<ColorContext::ColorSection>(section),
        static_cast<ColorContext::ColorName>(name), static_cast<ColorContext::ColorState>(state),
        color);
}

static void setColorRBGAFloat(
    vlc_qt_theme_provider_t* obj,
    vlc_qt_theme_color_set set, vlc_qt_theme_color_section section,
    vlc_qt_theme_color_name name, vlc_qt_theme_color_state state,
    double r, double g, double b, double a)
{
    auto palette = static_cast<SystemPalette*>(obj->setColorData);
    QColor color;
    color.setRgbF(r,g,b,a);
    palette->setColor(
        static_cast<ColorContext::ColorSet>(set), static_cast<ColorContext::ColorSection>(section),
        static_cast<ColorContext::ColorName>(name), static_cast<ColorContext::ColorState>(state),
        color);
}

}

//ExternalPaletteImpl

ExternalPaletteImpl::ExternalPaletteImpl(MainCtx* ctx, SystemPalette& palette, QObject* parent)
    : QObject(parent)
    , m_palette(palette)
    , m_ctx(ctx)
{
}

ExternalPaletteImpl::~ExternalPaletteImpl()
{
    if (m_provider)
    {
        if (m_provider->close)
            m_provider->close(m_provider);
        if (m_module)
            module_unneed(m_provider, m_module);
        vlc_object_delete(m_provider);
    }
}

bool ExternalPaletteImpl::init()
{
    QString preferedProvider;
#ifndef _WIN32
    if (isGTKBasedEnvironment())
        preferedProvider = "qt-themeprovider-gtk";
#endif

    m_provider = vlc_object_create<vlc_qt_theme_provider_t>(m_ctx->getIntf());
    if (!m_provider)
        return false;


    m_provider->paletteUpdated = PaletteChangedCallback;
    m_provider->paletteUpdatedData = this;

    m_provider->metricsUpdated = MetricsChangedCallback;
    m_provider->metricsUpdatedData = this;

    m_provider->setColorF = setColorRBGAFloat;
    m_provider->setColorInt = setColorRBGAInt;
    m_provider->setColorData = &m_palette;

    m_module = module_need(m_provider, "qt theme provider",
                           preferedProvider.isNull() ? nullptr : qtu(preferedProvider),
                           true);
    if (!m_module)
        return false;
    return true;
}

bool ExternalPaletteImpl::isThemeDark() const
{
    if (!m_provider->isThemeDark)
        return false;
    return m_provider->isThemeDark(m_provider);
}

bool ExternalPaletteImpl::hasCSDImages() const
{
    if (!m_provider->supportThemeImage)
        return false;
    return m_provider->supportThemeImage(m_provider, VLC_QT_THEME_IMAGE_TYPE_CSD_BUTTON);
}


QImage ExternalPaletteImpl::getCSDImage(vlc_qt_theme_csd_button_type type, vlc_qt_theme_csd_button_state state, bool maximized, bool active, int bannerHeight)
{
    if (!m_provider->getThemeImage)
        return {};
    vlc_qt_theme_image_setting imageSettings;
    imageSettings.windowScaleFactor = m_ctx->intfMainWindow()->devicePixelRatio();
    imageSettings.userScaleFacor = m_ctx->getIntfUserScaleFactor();
    imageSettings.u.csdButton.buttonType = type;
    imageSettings.u.csdButton.state = state;
    imageSettings.u.csdButton.maximized = maximized;
    imageSettings.u.csdButton.active = active;
    imageSettings.u.csdButton.bannerHeight = bannerHeight;
    picture_t* pic = m_provider->getThemeImage(m_provider, VLC_QT_THEME_IMAGE_TYPE_CSD_BUTTON, &imageSettings);
    if (!pic)
        return {};

    QImage::Format format = QImage::Format_Invalid;
    switch (pic->format.i_chroma)
    {
    case VLC_CODEC_ARGB:
        format = QImage::Format_ARGB32_Premultiplied;
        break;
    case VLC_CODEC_RGBA:
        format = QImage::Format_RGBA8888_Premultiplied;
        break;
    case VLC_CODEC_RGB24:
        format = QImage::Format_RGB888;
        break;
    default:
        msg_Err(m_ctx->getIntf(), "unexpected image format from theme provider");
        break;
    }

    return QImage(pic->p[0].p_pixels,
            pic->format.i_visible_width, pic->format.i_visible_height, pic->p[0].i_pitch,
            format,
            ReleaseVLCPictureCb, pic
            );
}

CSDMetrics* ExternalPaletteImpl::getCSDMetrics() const
{
    return m_csdMetrics.get();
}

int ExternalPaletteImpl::update()
{
    if (m_provider->updatePalette)
        return m_provider->updatePalette(m_provider);
    return VLC_EGENERIC;
}

void ExternalPaletteImpl::updateMetrics(vlc_qt_theme_image_type type)
{
    if (m_provider->getThemeMetrics)
    {
        if (type == VLC_QT_THEME_IMAGE_TYPE_CSD_BUTTON)
        {
            m_csdMetrics.reset();
            [&](){
                if (!m_provider->getThemeMetrics)
                    return;
                vlc_qt_theme_metrics metrics;
                memset(&metrics, 0, sizeof(metrics));
                bool ret = m_provider->getThemeMetrics(m_provider, VLC_QT_THEME_IMAGE_TYPE_CSD_BUTTON, &metrics);
                if (!ret)
                    return;
                m_csdMetrics = std::make_unique<CSDMetrics>();
                m_csdMetrics->interNavButtonSpacing = metrics.u.csd.interNavButtonSpacing;

                m_csdMetrics->csdFrameMarginLeft = metrics.u.csd.csdFrameMarginLeft;
                m_csdMetrics->csdFrameMarginRight = metrics.u.csd.csdFrameMarginRight;
                m_csdMetrics->csdFrameMarginTop = metrics.u.csd.csdFrameMarginTop;
                m_csdMetrics->csdFrameMarginBottom = metrics.u.csd.csdFrameMarginBottom;
            }();
            emit CSDMetricsChanged();
        }
    }
}

SystemPalette::SystemPalette(QObject* parent)
    : QObject(parent)
{

}

void SystemPalette::componentComplete()
{
    {
        assert(m_ctx);
        char* accentColor = var_InheritString(m_ctx->getIntf(), "qt-accent-color");
        assert(accentColor);

        if (!strcmp(accentColor, "default"))
        {
            // Use default
        }
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        else if (!strcmp(accentColor, "system"))
        {
            assert(qApp);
            const auto palette = qApp->palette();

            orange500 = palette.color(QPalette::Normal, QPalette::Accent);
            orange800 = orange500.lighter(150);
        }
#endif
        else
        {
            orange500 = QColor(accentColor);
            orange800 = orange500.lighter(150);
        }

        free(accentColor);
    }

    updatePalette();
}

ColorSchemeModel::ColorScheme SystemPalette::source() const
{
    return m_source;
}

QImage SystemPalette::getCSDImage(vlc_qt_theme_csd_button_type type, vlc_qt_theme_csd_button_state state, bool maximized, bool active, int bannerHeight)
{
    if (!hasCSDImage())
        return QImage();
    assert(m_palettePriv);
    return m_palettePriv->getCSDImage(type, state, maximized, active, bannerHeight);

}

CSDMetrics* SystemPalette::getCSDMetrics() const
{
    if (!m_palettePriv)
        return nullptr;
    return m_palettePriv->getCSDMetrics();
}

bool SystemPalette::hasCSDImage() const
{
    if (!m_palettePriv)
        return false;
    return m_palettePriv->hasCSDImages();
}

void SystemPalette::setSource(ColorSchemeModel::ColorScheme source)
{
    if (m_source == source)
        return;
    m_source = source;

    updatePalette();
    emit sourceChanged();
}

void SystemPalette::setCtx(MainCtx* ctx)
{
    if (ctx == m_ctx)
        return;
    m_ctx = ctx;
    emit ctxChanged();
    updatePalette();
}

void SystemPalette::updatePalette()
{
    m_palettePriv.reset();
    switch(m_source)
    {
    case ColorSchemeModel::System:
        makeSystemPalette();
        break;
    case ColorSchemeModel::Day:
        makeLightPalette();
        break;
    case ColorSchemeModel::Night:
        makeDarkPalette();
        break;
    default:
        break;
    }

    if (m_palettePriv)
    {
        connect(
            m_palettePriv.get(), &ExternalPaletteImpl::paletteChanged,
            this, &SystemPalette::updatePalette);
        connect(
            m_palettePriv.get(), &ExternalPaletteImpl::CSDMetricsChanged,
            this, &SystemPalette::CSDMetricsChanged);
    }
    emit paletteChanged();

    bool hasCSDImage = m_palettePriv && m_palettePriv->hasCSDImages();
    if (m_hasCSDImage != hasCSDImage)
    {
        m_hasCSDImage = hasCSDImage;
        emit hasCSDImageChanged();
    }
}

static quint64 makeKey(ColorContext::ColorSet colorSet, ColorContext::ColorSection section,
                       ColorContext::ColorName name, ColorContext::ColorState state)
{
    static_assert(VQTC_STATE_COUNT < (1<<4), "");
    static_assert(VQTC_SECTION_COUNT < (1<<4), "");
    static_assert(VQTC_NAME_COUNT < (1<<8), "");
    static_assert(VQTC_SET_COUNT < (1<<16), "");
    return  (colorSet << 16)
        + (name << 8)
        + (section << 4)
        + state;
}

void SystemPalette::setColor(ColorContext::ColorSet colorSet,  ColorContext::ColorSection section,
                             ColorContext::ColorName name, ColorContext::ColorState state, QColor color)
{

    quint64 key = makeKey(colorSet, section, name, state);
    m_colorMap[key] = color;
}


QColor SystemPalette::getColor(ColorContext::ColorSet colorSet, ColorContext::ColorSection section,
                               ColorContext::ColorName name, ColorContext::ColorState state) const
{
    typedef ColorContext C;

    quint64 key = makeKey(colorSet, section, name, state);
    auto it = m_colorMap.find(key);
    if (it != m_colorMap.cend())
        return *it;
    //we don't have the role explicitly set, fallback to the normal state
    key = makeKey(colorSet, section, name, C::Normal);
    it = m_colorMap.find(key);
    if (it != m_colorMap.cend())
        return *it;
    //we don't have the role explicitly set, fallback to the colorSet View
    //TODO do we want finer hierarchy?
    if (colorSet != C::View)
    {
        return getColor(C::View, section, name, state);
    }
    else
    {
        //nothing matches, that's probably an issue, return an ugly color
        return Qt::magenta;
    }
}


/*
 * Redesign palettes, based on the "VLC Redesign" Figma community file.
 * Token names in the comments refer to the color styles of that file
 * (e.g. "Dark/BG/B1"). See design/DESIGN.md for the full token table.
 */
namespace redesign {
namespace dark {
    // foreground
    const QColor fg1        {"#EAEAEA"}; // Dark/FG/1
    const QColor fg2        {"#C6C6C6"}; // Dark/FG/2   (active / highlighted text)
    const QColor fgA1       {"#969696"}; // Dark/FG/A1  (primary text)
    const QColor fgA2       {"#8F8F8F"}; // Dark/FG/A2  (icons, nav text)
    const QColor fgA3       {"#6B6B6B"}; // Dark/FG/A3  (secondary text)
    const QColor fgA4       {"#404040"}; // Dark/FG/A4  (disabled, placeholder, tracks)
    const QColor fgB1       {"#161616"}; // Dark/FG/B1  (text on accent)
    // accent
    const QColor accent     {"#D37918"}; // Dark/Accent/Main
    const QColor accentBright{"#FC9729"}; // Dark/Accent/Bright
    const QColor accentDim  {"#AB6314"}; // Dark/Accent/DIm
    // background
    const QColor bgAccentBright{"#371F06"}; // Dark/BG/Accent Bright
    const QColor bgAccentDark {"#1A1611"};  // Dark/BG/Accent Dark
    const QColor bgA1       {"#222222"}; // Dark/BG/A1
    const QColor bgA2       {"#1F1F1F"}; // Dark/BG/A2
    const QColor bgA3       {"#1B1B1B"}; // Dark/BG/A3
    const QColor bgA4       {"#191919"}; // Dark/BG/A4  (side panel)
    const QColor bgA5       {"#151515"}; // Dark/BG/A5  (input fields)
    const QColor bgB1       {"#0E0E0E"}; // Dark/BG/B1  (main content)
    const QColor bgB2       {"#090909"}; // Dark/BG/B2  (player)
    const QColor card       {"#171717"}; // card / thumbnail placeholder
    // dividers
    const QColor dividerBright{"#373737"}; // Dark/Divider/Bright
    const QColor dividerDim {"#222222"};   // Dark/Divider/Dim
}

namespace light {
    const QColor fgA1       {"#3C3C3C"}; // Light/FG/A1
    const QColor fgA2       {"#2E2E2E"}; // Light/FG/A2  (primary text)
    const QColor fgA3       {"#6B6B6B"}; // Light/FG/A3  (secondary text)
    const QColor fgA4       {"#3B3B3B"}; // Light/FG/A4
    const QColor fgB1       {"#161616"}; // Light/FG/B1  (strongest text / text on accent)
    // NOTE: Light/FG/1 and Light/FG/2 are identical to the dark values in the
    // design file (near white) and unreadable on light backgrounds, they are
    // intentionally not used here.
    const QColor accent     {"#E8851A"}; // Light/Accent/Main
    const QColor accentBright{"#FFA13B"}; // Light/Accent/Bright
    const QColor accentLogo {"#C2711A"}; // Light/Accent/Logo
    const QColor accentDim  {"#965916"}; // Light/Accent/DIm
    const QColor bgAccentBright{"#D8BA9A"}; // Light/BG/Accent Bright
    const QColor bgAccentDark {"#C6AB89"};  // Light/BG/Accent Dark
    const QColor bgA1       {"#EDEDED"}; // Light/BG/A1  (side panel)
    const QColor bgA2       {"#DDDDDD"}; // Light/BG/A2
    const QColor bgA3       {"#D5D5D5"}; // Light/BG/A3
    const QColor bgA4       {"#B3B3B3"}; // Light/BG/A4
    const QColor bgA5       {"#888888"}; // Light/BG/A5
    const QColor bgB1       {"#E7E7E7"}; // Light/BG/B1  (main content)
    const QColor bgB2       {"#DEDEDE"}; // Light/BG/B2
    const QColor card       {"#DDDDDD"}; // card / thumbnail placeholder (BG/A2)
    const QColor dividerBright{"#9B9B9B"}; // Light/Divider/Bright
    const QColor dividerDim {"#ACACAC"};   // Light/Divider/Dim
}
}

void SystemPalette::makeLightPalette()
{
    m_isDark = false;

    typedef ColorContext C;
    using namespace redesign::light;

    m_colorMap.clear();

    //base set
    {
        C::ColorSet CS = C::View;
        setColor(CS, C::Bg, C::Primary, C::Normal, bgB1);
        setColor(CS, C::Bg, C::Secondary, C::Normal, card);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA2);
        setColor(CS, C::Fg, C::Primary, C::Disabled, setColorAlpha(fgA2, 0.35));

        setColor(CS, C::Fg, C::Secondary, C::Normal, fgA3);

        setColor(CS, C::Bg, C::Negative, C::Normal, QColor("#fde7e9"));
        setColor(CS, C::Fg, C::Negative, C::Normal, QColor("#C42B1C"));

        setColor(CS, C::Bg, C::Neutral, C::Normal, bgAccentBright);
        setColor(CS, C::Fg, C::Neutral, C::Normal, accentDim);

        setColor(CS, C::Bg, C::Positive, C::Normal, QColor("#dff6dd"));
        setColor(CS, C::Fg, C::Positive, C::Normal, QColor("#0F7B0F"));

        setColor(CS, C::Decoration, C::VisualFocus, C::Normal, setColorAlpha(accent, 0.0) );
        setColor(CS, C::Decoration, C::VisualFocus, C::Focused, accent );

        setColor(CS, C::Decoration, C::Border, C::Normal, dividerDim );
        setColor(CS, C::Decoration, C::Border, C::Focused, dividerBright );
        setColor(CS, C::Decoration, C::Border, C::Hovered, dividerBright );
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent );

        setColor(CS, C::Decoration, C::Separator, C::Normal, dividerDim);

        setColor(CS, C::Decoration, C::Shadow, C::Normal, setColorAlpha(Qt::black, 0.18));

        setColor(CS, C::Decoration, C::Accent, C::Normal, accent);
        setColor(CS, C::Fg, C::Link, C::Normal, accentDim);
    }

    //window banner & miniplayer
    {
        C::ColorSet CS = C::Window;
        setColor(CS, C::Bg, C::Primary, C::Normal, bgA3);
        setColor(CS, C::Bg, C::Secondary, C::Normal, bgA1);
        setColor(CS, C::Decoration, C::Border, C::Normal, dividerDim);
    }

    //badges
    {
        C::ColorSet CS = C::Badge;
        setColor(CS, C::Bg, C::Primary, C::Normal, setColorAlpha(fgB1, 0.7));
        setColor(CS, C::Fg, C::Primary, C::Normal, bgB1);
    }

    //tab button
    {
        C::ColorSet CS = C::TabButton;
        setColor(CS, C::Bg, C::Primary, C::Normal, setColorAlpha(bgA2, 0.0));
        setColor(CS, C::Bg, C::Primary, C::Focused, bgA2);
        setColor(CS, C::Bg, C::Primary, C::Hovered, bgA2);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA3);
        setColor(CS, C::Fg, C::Primary, C::Focused, fgB1);
        setColor(CS, C::Fg, C::Primary, C::Hovered, fgB1);
        setColor(CS, C::Fg, C::Primary, C::Disabled, setColorAlpha(fgA3, 0.4));
        setColor(CS, C::Fg, C::Secondary, C::Normal, fgB1);
    }

    //tool button
    {
        C::ColorSet CS = C::ToolButton;
        setColor(CS, C::Bg, C::Primary, C::Normal, Qt::transparent);
        setColor(CS, C::Bg, C::Secondary, C::Normal, bgA2);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA3);
        setColor(CS, C::Fg, C::Primary, C::Focused, fgB1);
        setColor(CS, C::Fg, C::Primary, C::Hovered, fgB1);
        setColor(CS, C::Fg, C::Primary, C::Disabled, setColorAlpha(fgA3, 0.4));
        setColor(CS, C::Fg, C::Secondary, C::Normal, fgB1);

        setColor(CS, C::Decoration, C::Border, C::Normal, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Focused, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Hovered, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent);
    }

    //menubar
    {
        C::ColorSet CS = C::MenuBar;
        setColor(CS, C::Bg, C::Primary, C::Normal, setColorAlpha(bgA2, 0.0));
        setColor(CS, C::Bg, C::Primary, C::Focused, bgA2);
        setColor(CS, C::Bg, C::Primary, C::Hovered, bgA2);
        setColor(CS, C::Fg, C::Primary, C::Normal, fgA2);
        setColor(CS, C::Fg, C::Primary, C::Disabled, setColorAlpha(fgA2, 0.35));
    }

    //Item
    {
        C::ColorSet CS = C::Item;
        setColor(CS, C::Bg, C::Primary, C::Normal, setColorAlpha(bgA2, 0.0));
        setColor(CS, C::Bg, C::Primary, C::Focused, bgA2);
        setColor(CS, C::Bg, C::Primary, C::Hovered, bgA2);

        setColor(CS, C::Bg, C::Highlight, C::Normal, bgAccentBright);
        setColor(CS, C::Bg, C::Highlight, C::Focused, bgAccentDark);
        setColor(CS, C::Bg, C::Highlight, C::Hovered, bgAccentDark);
        setColor(CS, C::Fg, C::Highlight, C::Normal, fgB1);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA2);
        setColor(CS, C::Fg, C::Secondary, C::Normal, fgA3);

        setColor(CS, C::Decoration, C::Indicator, C::Normal, accent);
    }

    //Accent Buttons
    {
        C::ColorSet CS = C::ButtonAccent;
        setColor(CS, C::Bg, C::Primary, C::Normal, accent);
        setColor(CS, C::Bg, C::Primary, C::Hovered, accentBright);
        setColor(CS, C::Bg, C::Primary, C::Focused, accentBright);
        setColor(CS, C::Bg, C::Primary, C::Pressed, accentDim);
        setColor(CS, C::Bg, C::Primary, C::Disabled, bgA3);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgB1);
        setColor(CS, C::Fg, C::Primary, C::Disabled, setColorAlpha(fgA3, 0.6));

        setColor(CS, C::Decoration, C::Border, C::Normal, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Focused, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Hovered, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent);
    }

    //Standard Buttons
    {
        C::ColorSet CS = C::ButtonStandard;
        setColor(CS, C::Bg, C::Primary, C::Normal, Qt::transparent);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA3);
        setColor(CS, C::Fg, C::Primary, C::Focused, fgB1);
        setColor(CS, C::Fg, C::Primary, C::Hovered, fgB1);
        setColor(CS, C::Fg, C::Primary, C::Disabled, setColorAlpha(fgA3, 0.4));

        setColor(CS, C::Decoration, C::Border, C::Normal, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Focused, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Hovered, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent);
    }

    //tooltip
    {
        C::ColorSet CS = C::Tooltip;
        setColor(CS, C::Bg, C::Primary, C::Normal, bgA1);
        setColor(CS, C::Fg, C::Primary, C::Normal, fgB1);
    }

    //slider
    {
        C::ColorSet CS = C::Slider;
        setColor(CS, C::Bg, C::Primary, C::Normal, setColorAlpha(bgA5, 0.45));
        setColor(CS, C::Bg, C::Primary, C::Focused, setColorAlpha(bgA5, 0.65));
        setColor(CS, C::Bg, C::Primary, C::Hovered, setColorAlpha(bgA5, 0.65));
        setColor(CS, C::Bg, C::Secondary, C::Normal, setColorAlpha(bgA5, 0.2));
        setColor(CS, C::Fg, C::Primary, C::Normal, accent);
        setColor(CS, C::Fg, C::Positive, C::Normal, "#0F7B0F");
        setColor(CS, C::Fg, C::Neutral, C::Normal, "#9D5D00");
        setColor(CS, C::Fg, C::Negative, C::Normal, "#C42B1C");
    }

    //Combo box
    {
        C::ColorSet CS = C::ComboBox;
        setColor(CS, C::Fg, C::Primary, C::Normal, fgA2);
        setColor(CS, C::Bg, C::Primary, C::Normal, bgA1);
        setColor(CS, C::Bg, C::Secondary, C::Normal, bgA2);
    }

    //TextField
    {
        C::ColorSet CS = C::TextField;
        setColor(CS, C::Decoration, C::Border, C::Normal, dividerDim );
        setColor(CS, C::Decoration, C::Border, C::Focused, accent);
        setColor(CS, C::Decoration, C::Border, C::Hovered, dividerBright );
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent );

        setColor(CS, C::Bg, C::Highlight, C::Normal, bgAccentBright);
        setColor(CS, C::Fg, C::Highlight, C::Normal, fgB1);
    }

    //Switch
    {
        C::ColorSet CS = C::Switch;
        setColor(CS, C::Bg, C::Primary, C::Normal, bgA2);
        setColor(CS, C::Fg, C::Primary, C::Normal, fgA3);
        setColor(CS, C::Decoration, C::Border, C::Normal, dividerBright);

        setColor(CS, C::Bg, C::Secondary, C::Normal, accent);
        setColor(CS, C::Fg, C::Secondary, C::Normal, fgB1);
    }

    //SpinBox
    {
        C::ColorSet CS = C::SpinBox;
        setColor(CS, C::Decoration, C::Border, C::Normal, dividerDim );
        setColor(CS, C::Decoration, C::Border, C::Focused, accent);
        setColor(CS, C::Decoration, C::Border, C::Hovered, dividerBright );
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent );

        setColor(CS, C::Bg, C::Highlight, C::Normal, bgAccentBright);
        setColor(CS, C::Fg, C::Highlight, C::Normal, fgB1);
    }
}

void SystemPalette::makeDarkPalette()
{
    m_isDark = true;

    m_colorMap.clear();

    typedef ColorContext C;
    using namespace redesign::dark;

    {
        C::ColorSet CS = C::View;
        setColor(CS, C::Bg, C::Primary, C::Normal, bgB1);
        setColor(CS, C::Bg, C::Secondary, C::Normal, card);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA1);
        setColor(CS, C::Fg, C::Primary, C::Disabled, fgA4);

        setColor(CS, C::Fg, C::Secondary, C::Normal, fgA3);

        setColor(CS, C::Bg, C::Negative, C::Normal, QColor("#3A1410"));
        setColor(CS, C::Fg, C::Negative, C::Normal, QColor("#E15032"));

        setColor(CS, C::Bg, C::Neutral, C::Normal, bgAccentBright);
        setColor(CS, C::Fg, C::Neutral, C::Normal, accentBright);

        setColor(CS, C::Bg, C::Positive, C::Normal, QColor("#0F2A14"));
        setColor(CS, C::Fg, C::Positive, C::Normal, QColor("#19A234"));

        setColor(CS, C::Decoration, C::VisualFocus, C::Normal, setColorAlpha(accent, 0.0) );
        setColor(CS, C::Decoration, C::VisualFocus, C::Focused, accent );

        setColor(CS, C::Decoration, C::Border, C::Normal, dividerBright );
        setColor(CS, C::Decoration, C::Border, C::Focused, fgA4 );
        setColor(CS, C::Decoration, C::Border, C::Hovered, fgA4 );
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent );

        setColor(CS, C::Decoration, C::Shadow, C::Normal, setColorAlpha(Qt::black, 0.46));

        setColor(CS, C::Decoration, C::Separator, C::Normal, dividerDim);

        setColor(CS, C::Decoration, C::Accent, C::Normal, accent);
        setColor(CS, C::Fg, C::Link, C::Normal, accentBright);
    }

    //window banner & miniplayer
    {
        C::ColorSet CS = C::Window;
        setColor(CS, C::Bg, C::Primary, C::Normal, bgA3);
        setColor(CS, C::Bg, C::Secondary, C::Normal, bgA4);
        setColor(CS, C::Decoration, C::Border, C::Normal, dividerDim);
    }

    //badges
    {
        C::ColorSet CS = C::Badge;
        setColor(CS, C::Bg, C::Primary, C::Normal, setColorAlpha(fg2, 0.85));
        setColor(CS, C::Fg, C::Primary, C::Normal, fgB1);
    }

    //tab button
    {
        C::ColorSet CS = C::TabButton;
        setColor(CS, C::Bg, C::Primary, C::Normal, setColorAlpha(bgA1, 0.0));
        setColor(CS, C::Bg, C::Primary, C::Focused, bgA1);
        setColor(CS, C::Bg, C::Primary, C::Hovered, bgA1);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA2);
        setColor(CS, C::Fg, C::Primary, C::Focused, fg2);
        setColor(CS, C::Fg, C::Primary, C::Hovered, fg2);
        setColor(CS, C::Fg, C::Primary, C::Disabled, fgA4);
        setColor(CS, C::Fg, C::Secondary, C::Normal, fg2);
    }

    //tool button
    {
        C::ColorSet CS = C::ToolButton;
        setColor(CS, C::Bg, C::Primary, C::Normal, Qt::transparent);
        setColor(CS, C::Bg, C::Secondary, C::Normal, bgA1);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA2);
        setColor(CS, C::Fg, C::Primary, C::Focused, fg2);
        setColor(CS, C::Fg, C::Primary, C::Hovered, fg2);
        setColor(CS, C::Fg, C::Primary, C::Disabled, fgA4);
        setColor(CS, C::Fg, C::Secondary, C::Normal, fg2);

        setColor(CS, C::Decoration, C::Border, C::Normal, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Focused, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Hovered, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent);
    }

    //menubar
    {
        C::ColorSet CS = C::MenuBar;
        setColor(CS, C::Bg, C::Primary, C::Normal, setColorAlpha(bgA1, 0.0));
        setColor(CS, C::Bg, C::Primary, C::Focused, bgA1);
        setColor(CS, C::Bg, C::Primary, C::Hovered, bgA1);
        setColor(CS, C::Fg, C::Primary, C::Normal, fgA1);
        setColor(CS, C::Fg, C::Primary, C::Disabled, fgA4);
    }

    //Item
    {
        C::ColorSet CS = C::Item;
        setColor(CS, C::Bg, C::Primary, C::Normal, setColorAlpha(bgA1, 0.0));
        setColor(CS, C::Bg, C::Primary, C::Focused, bgA1);
        setColor(CS, C::Bg, C::Primary, C::Hovered, bgA1);

        setColor(CS, C::Bg, C::Highlight, C::Normal, bgAccentDark);
        setColor(CS, C::Bg, C::Highlight, C::Focused, bgAccentBright);
        setColor(CS, C::Bg, C::Highlight, C::Hovered, bgAccentBright);
        setColor(CS, C::Fg, C::Highlight, C::Normal, fg2);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA1);
        setColor(CS, C::Fg, C::Secondary, C::Normal, fgA3);

        setColor(CS, C::Decoration, C::Indicator, C::Normal, accent);
    }

    //Accent Buttons
    {
        C::ColorSet CS = C::ButtonAccent;
        setColor(CS, C::Bg, C::Primary, C::Normal, accent);
        setColor(CS, C::Bg, C::Primary, C::Hovered, accentBright);
        setColor(CS, C::Bg, C::Primary, C::Focused, accentBright);
        setColor(CS, C::Bg, C::Primary, C::Pressed, accentDim);
        setColor(CS, C::Bg, C::Primary, C::Disabled, bgA1);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgB1);
        setColor(CS, C::Fg, C::Primary, C::Disabled, fgA4);

        setColor(CS, C::Decoration, C::Border, C::Normal, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Focused, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Hovered, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent);
    }

    //Standard Buttons
    {
        C::ColorSet CS = C::ButtonStandard;
        setColor(CS, C::Bg, C::Primary, C::Normal, Qt::transparent);

        setColor(CS, C::Fg, C::Primary, C::Normal, fgA2);
        setColor(CS, C::Fg, C::Primary, C::Focused, fg2);
        setColor(CS, C::Fg, C::Primary, C::Hovered, fg2);
        setColor(CS, C::Fg, C::Primary, C::Disabled, fgA4);

        setColor(CS, C::Decoration, C::Border, C::Normal, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Focused, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Hovered, Qt::transparent);
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent);
    }

    //tooltip
    {
        C::ColorSet CS = C::Tooltip;
        setColor(CS, C::Bg, C::Primary, C::Normal, bgA1);
        setColor(CS, C::Fg, C::Primary, C::Normal, fg2);
    }

    //slider
    {
        C::ColorSet CS = C::Slider;
        setColor(CS, C::Bg, C::Primary, C::Normal, fgA4);
        setColor(CS, C::Bg, C::Primary, C::Focused, fgA3);
        setColor(CS, C::Bg, C::Primary, C::Hovered, fgA3);
        setColor(CS, C::Bg, C::Secondary, C::Normal, setColorAlpha(fgA4, 0.5));

        setColor(CS, C::Fg, C::Primary, C::Normal, accent);
        setColor(CS, C::Fg, C::Positive, C::Normal, "#19A234");
        setColor(CS, C::Fg, C::Neutral, C::Normal, "#C49E23");
        setColor(CS, C::Fg, C::Negative, C::Normal, "#E15032");
    }

    //Combo box
    {
        C::ColorSet CS = C::ComboBox;
        setColor(CS, C::Fg, C::Primary, C::Normal, fgA1);
        setColor(CS, C::Bg, C::Primary, C::Normal, bgA5);
        setColor(CS, C::Bg, C::Secondary, C::Normal, bgA1);
    }

    //TextField
    {
        C::ColorSet CS = C::TextField;
        setColor(CS, C::Decoration, C::Border, C::Normal, dividerBright );
        setColor(CS, C::Decoration, C::Border, C::Focused, accent );
        setColor(CS, C::Decoration, C::Border, C::Hovered, fgA4 );
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent );

        setColor(CS, C::Bg, C::Highlight, C::Normal, bgAccentBright);
        setColor(CS, C::Fg, C::Highlight, C::Normal, fg1);
    }

    //Switch
    {
        C::ColorSet CS = C::Switch;
        setColor(CS, C::Bg, C::Primary, C::Normal, bgA1);
        setColor(CS, C::Fg, C::Primary, C::Normal, fgA2);
        setColor(CS, C::Decoration, C::Border, C::Normal, fgA4);
        setColor(CS, C::Bg, C::Secondary, C::Normal, accent);
        setColor(CS, C::Fg, C::Secondary, C::Normal, fgB1);
    }

    //Spinbox
    {
        C::ColorSet CS = C::SpinBox;
        setColor(CS, C::Decoration, C::Border, C::Normal, dividerBright );
        setColor(CS, C::Decoration, C::Border, C::Focused, accent );
        setColor(CS, C::Decoration, C::Border, C::Hovered, fgA4 );
        setColor(CS, C::Decoration, C::Border, C::Disabled, Qt::transparent );

        setColor(CS, C::Bg, C::Highlight, C::Normal, bgAccentBright);
        setColor(CS, C::Fg, C::Highlight, C::Normal, fg1);
    }
}

void SystemPalette::makeSystemPalette()
{
    if (!m_ctx)
    {
        //can't initialise system palette, fallback to default
        makeLightPalette();
        return;
    }

    auto palette = std::make_unique<ExternalPaletteImpl>(m_ctx, *this);
    if (!palette->init())
    {
        //can't initialise system palette, fallback to default
        makeLightPalette();
        return;
    }

    m_colorMap.clear();
    int ret = palette->update();
    if (ret != VLC_SUCCESS)
    {
        if (palette->isThemeDark())
            makeDarkPalette();
        else
            makeLightPalette();
    }
    else
    {
        m_isDark = palette->isThemeDark();
    }

    m_palettePriv = std::move(palette);
}
