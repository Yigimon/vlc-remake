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
#include "widgetmenustyle.hpp"

#include <QApplication>
#include <QEvent>
#include <QMenu>
#include <QPalette>
#include <QPointer>

#include "systempalette.hpp"
#include "colorcontext.hpp"

namespace {

// Dynamic property storing which style generation was applied to a menu
constexpr const char* STYLE_GENERATION_PROPERTY = "_vlcMenuStyleGeneration";

QString cssColor(const QColor& color)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

// Rounded corners need a translucent popup window, which only renders properly
// when the windowing system composites windows. Without a compositor (possible
// with X11) the corners would be black, so keep square menus there.
bool useTranslucentMenus()
{
    return QGuiApplication::platformName() != QLatin1String("xcb");
}

class MenuStyler : public QObject
{
public:
    explicit MenuStyler(QObject* parent)
        : QObject(parent)
    {
    }

    void setStyle(const QString& styleSheet, const QPalette& palette)
    {
        m_styleSheet = styleSheet;
        m_palette = palette;
        ++m_generation;

        // Update the menus that already exist:
        const auto widgets = QApplication::allWidgets();
        for (QWidget* widget : widgets)
        {
            if (auto menu = qobject_cast<QMenu*>(widget))
                apply(menu);
        }
    }

protected:
    bool eventFilter(QObject* object, QEvent* event) override
    {
        // Polish happens before the menu window is created, which is required
        // for the translucent background attribute.
        if (event->type() == QEvent::Polish && m_generation > 0)
        {
            if (auto menu = qobject_cast<QMenu*>(object))
                apply(menu);
        }
        return QObject::eventFilter(object, event);
    }

private:
    void apply(QMenu* menu)
    {
        // Setting the style sheet polishes the menu again, do not recurse:
        if (menu->property(STYLE_GENERATION_PROPERTY).toInt() == m_generation)
            return;
        menu->setProperty(STYLE_GENERATION_PROPERTY, m_generation);

        if (useTranslucentMenus() && !menu->testAttribute(Qt::WA_WState_Created))
            menu->setAttribute(Qt::WA_TranslucentBackground, true);

        menu->setPalette(m_palette);
        menu->setStyleSheet(m_styleSheet);
    }

    QString m_styleSheet;
    QPalette m_palette;
    int m_generation = 0;
};

MenuStyler* menuStyler()
{
    static QPointer<MenuStyler> styler;
    if (!styler && qApp)
    {
        styler = new MenuStyler(qApp);
        qApp->installEventFilter(styler);
    }
    return styler;
}

} // anonymous namespace

void applyWidgetMenuStyle(const SystemPalette& palette)
{
    MenuStyler* styler = menuStyler();
    if (!styler)
        return;

    typedef ColorContext C;

    // Design tokens (dark / light): BG/A3, Divider, FG/A1, BG/A1, FG/2, FG/A4
    const QColor background = palette.getColor(C::Window, C::Bg, C::Primary, C::Normal);
    const QColor border = palette.getColor(C::View, C::Decoration, C::Border, C::Normal);
    const QColor text = palette.getColor(C::View, C::Fg, C::Primary, C::Normal);
    const QColor hoverBackground = palette.getColor(C::Item, C::Bg, C::Primary, C::Hovered);
    const QColor hoverText = palette.getColor(C::TabButton, C::Fg, C::Primary, C::Hovered);
    const QColor disabledText = palette.getColor(C::View, C::Fg, C::Primary, C::Disabled);
    const QColor separator = palette.getColor(C::View, C::Decoration, C::Separator, C::Normal);

    const int radius = useTranslucentMenus() ? 10 : 0;

    const QString styleSheet = QStringLiteral(
        "QMenu {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: %8px;"
        "  padding: 4px;"
        "  color: %3;"
        "  font-size: 14px;"
        "}"
        "QMenu::item {"
        "  padding: 8px 28px 8px 32px;"
        "  border-radius: 6px;"
        "  background-color: transparent;"
        "}"
        "QMenu::item:selected {"
        "  background-color: %4;"
        "  color: %5;"
        "}"
        "QMenu::item:disabled {"
        "  color: %6;"
        "}"
        "QMenu::separator {"
        "  height: 1px;"
        "  background-color: %7;"
        "  margin: 4px 10px;"
        "}"
        "QMenu::icon, QMenu::indicator {"
        "  left: 8px;"
        "}")
        .arg(cssColor(background),
             cssColor(border),
             cssColor(text),
             cssColor(hoverBackground),
             cssColor(hoverText),
             cssColor(disabledText),
             cssColor(separator))
        .arg(radius);

    QPalette menuPalette = QApplication::palette();
    for (QPalette::ColorRole role : { QPalette::Window, QPalette::Base, QPalette::Button, QPalette::AlternateBase })
        menuPalette.setColor(role, background);
    for (QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
    {
        menuPalette.setColor(QPalette::Active, role, text);
        menuPalette.setColor(QPalette::Inactive, role, text);
        menuPalette.setColor(QPalette::Disabled, role, disabledText);
    }
    menuPalette.setColor(QPalette::Highlight, hoverBackground);
    menuPalette.setColor(QPalette::HighlightedText, hoverText);

    styler->setStyle(styleSheet, menuPalette);
}
