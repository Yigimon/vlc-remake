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
#include "widgetstyle.hpp"

#include <optional>

#include <memory>

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QMenu>
#include <QPalette>
#include <QPointer>
#include <QTemporaryDir>

#include "systempalette.hpp"
#include "colorcontext.hpp"
#include "util/color_scheme_model.hpp"

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

// Fills the "%{name}" placeholders of a style sheet template
class StyleSheetBuilder
{
public:
    explicit StyleSheetBuilder(QString styleSheet)
        : m_styleSheet(std::move(styleSheet))
    {
    }

    StyleSheetBuilder& set(const char* name, const QColor& color)
    {
        return set(name, cssColor(color));
    }

    StyleSheetBuilder& set(const char* name, int value)
    {
        return set(name, QString::number(value));
    }

    StyleSheetBuilder& set(const char* name, const QString& value)
    {
        m_styleSheet.replace(QStringLiteral("%{%1}").arg(QLatin1String(name)), value);
        return *this;
    }

    const QString& result() const { return m_styleSheet; }

private:
    QString m_styleSheet;
};

QString svgColor(const QColor& color)
{
    return color.name(QColor::HexRgb);
}

QString svgOpacity(const QColor& color)
{
    return QString::number(color.alphaF(), 'f', 3);
}

// The style sheets can only use image files, the images of the indicators are
// generated with the colors of the palette in a temporary directory (the name
// of a file depends on its content, as Qt caches the images by file name).
QString styleImage(const char* name, const QString& svg)
{
    static std::unique_ptr<QTemporaryDir> directory;
    if (!directory)
    {
        directory = std::make_unique<QTemporaryDir>();
        if (!directory->isValid())
            return {};
    }

    const QByteArray data = svg.toUtf8();
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex().left(12));
    const QString path = directory->filePath(QStringLiteral("%1-%2.svg").arg(QLatin1String(name), hash));

    if (!QFile::exists(path))
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        file.write(data);
    }

    // the style sheet syntax wants forward slashes:
    return QStringLiteral("url(\"%1\")").arg(QDir::fromNativeSeparators(path));
}

QString chevronImage(const char* name, const QColor& color, bool up)
{
    return styleImage(name, QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"12\" height=\"12\" viewBox=\"0 0 12 12\">"
        "<path d=\"%1\" fill=\"none\" stroke=\"%2\" stroke-opacity=\"%3\" stroke-width=\"1.5\""
        " stroke-linecap=\"round\" stroke-linejoin=\"round\"/></svg>")
        .arg(up ? QStringLiteral("M2.5 7.75 6 4.25l3.5 3.5") : QStringLiteral("M2.5 4.25 6 7.75l3.5-3.5"),
             svgColor(color), svgOpacity(color)));
}

QString checkBoxImage(const char* name, const QColor& background, const QColor& border, const QColor& mark)
{
    QString svg = QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"18\" height=\"18\" viewBox=\"0 0 18 18\">"
        "<rect x=\"1.75\" y=\"1.75\" width=\"14.5\" height=\"14.5\" rx=\"4\" fill=\"%1\" fill-opacity=\"%2\""
        " stroke=\"%3\" stroke-opacity=\"%4\" stroke-width=\"1.5\"/>")
        .arg(svgColor(background), svgOpacity(background), svgColor(border), svgOpacity(border));
    if (mark.isValid())
        svg += QStringLiteral("<path d=\"M5.25 9.25 7.75 11.75 12.75 6.25\" fill=\"none\" stroke=\"%1\""
                              " stroke-width=\"1.75\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>")
                   .arg(svgColor(mark));
    svg += QStringLiteral("</svg>");
    return styleImage(name, svg);
}

QString radioButtonImage(const char* name, const QColor& background, const QColor& border, const QColor& dot)
{
    QString svg = QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"18\" height=\"18\" viewBox=\"0 0 18 18\">"
        "<circle cx=\"9\" cy=\"9\" r=\"7.25\" fill=\"%1\" fill-opacity=\"%2\""
        " stroke=\"%3\" stroke-opacity=\"%4\" stroke-width=\"1.5\"/>")
        .arg(svgColor(background), svgOpacity(background), svgColor(border), svgOpacity(border));
    if (dot.isValid())
        svg += QStringLiteral("<circle cx=\"9\" cy=\"9\" r=\"3.5\" fill=\"%1\"/>").arg(svgColor(dot));
    svg += QStringLiteral("</svg>");
    return styleImage(name, svg);
}

// Toggle switch, see the "Toggle" component of the design
QString toggleImage(const char* name, const QColor& frame, const QColor& track, const QColor& knob, bool on)
{
    const QString knobRect = on ? QStringLiteral("x=\"23\" y=\"2\" width=\"15\" height=\"18\"")
                                : QStringLiteral("x=\"2\" y=\"2\" width=\"15\" height=\"18\"");
    const QString trackRect = on ? QStringLiteral("x=\"2\" y=\"2\" width=\"20\" height=\"18\"")
                                 : QStringLiteral("x=\"18\" y=\"2\" width=\"20\" height=\"18\"");
    return styleImage(name, QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"22\" viewBox=\"0 0 40 22\">"
        "<rect width=\"40\" height=\"22\" rx=\"5\" fill=\"%1\" fill-opacity=\"%2\"/>"
        "<rect %3 rx=\"3\" fill=\"%4\" fill-opacity=\"%5\"/>"
        "<rect %6 rx=\"3\" fill=\"%7\" fill-opacity=\"%8\"/>"
        "</svg>")
        .arg(svgColor(frame), svgOpacity(frame),
             trackRect, svgColor(track), svgOpacity(track),
             knobRect, svgColor(knob), svgOpacity(knob)));
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

// The application palette before it was replaced, it is restored when the
// system color scheme is selected, as it is the source of the system palette.
std::optional<QPalette>& originalApplicationPalette()
{
    static std::optional<QPalette> palette;
    return palette;
}

typedef ColorContext C;

// Colors of the Qt Widgets, taken from the interface palette (see the
// "Open network stream" and "Settings" frames of the design).
struct WidgetColors
{
    explicit WidgetColors(const SystemPalette& palette)
    {
        dark = palette.isDark();

        // Dark: BG/A4, light: BG/A1
        window = palette.getColor(C::Window, C::Bg, C::Secondary, C::Normal);
        // Dark: BG/A3, light: BG/A3
        button = palette.getColor(C::Window, C::Bg, C::Primary, C::Normal);
        buttonHovered = palette.getColor(C::Item, C::Bg, C::Primary, C::Hovered);
        // Dark: BG/B1, light: BG/B1
        field = palette.getColor(C::View, C::Bg, C::Primary, C::Normal);
        fieldBorder = palette.getColor(C::View, C::Decoration, C::Border, C::Normal);
        separator = palette.getColor(C::View, C::Decoration, C::Separator, C::Normal);

        text = palette.getColor(C::View, C::Fg, C::Primary, C::Normal);
        strongText = palette.getColor(C::TabButton, C::Fg, C::Primary, C::Hovered);
        secondaryText = palette.getColor(C::View, C::Fg, C::Secondary, C::Normal);
        disabledText = palette.getColor(C::View, C::Fg, C::Primary, C::Disabled);
        buttonText = palette.getColor(C::ButtonStandard, C::Fg, C::Primary, C::Normal);
        link = palette.getColor(C::View, C::Fg, C::Link, C::Normal);

        accent = palette.getColor(C::ButtonAccent, C::Bg, C::Primary, C::Normal);
        accentPressed = palette.getColor(C::ButtonAccent, C::Bg, C::Primary, C::Pressed);
        textOnAccent = palette.getColor(C::ButtonAccent, C::Fg, C::Primary, C::Normal);

        // Accent backgrounds: Dark/BG/Accent Bright, Light/BG/Accent Bright
        accentBackground = palette.getColor(C::Item, C::Bg, C::Highlight, C::Hovered);
        accentBackgroundHovered = dark ? accentBackground.lighter(130) : accentBackground.darker(108);
        // the accent is not readable enough on the light accent background:
        accentText = dark ? accent : accentPressed;

        // Dark: FG/A4
        track = palette.getColor(C::Slider, C::Bg, C::Primary, C::Normal);
        scrollHandle = dark ? QColor(0x40, 0x40, 0x40) : QColor(0xAC, 0xAC, 0xAC);

        menu = palette.getColor(C::Window, C::Bg, C::Primary, C::Normal);
        menuBorder = palette.getColor(C::View, C::Decoration, C::Border, C::Normal);
        itemHovered = palette.getColor(C::Item, C::Bg, C::Primary, C::Hovered);
        itemHoveredText = palette.getColor(C::TabButton, C::Fg, C::Primary, C::Hovered);

        tooltip = palette.getColor(C::Tooltip, C::Bg, C::Primary, C::Normal);
        tooltipText = palette.getColor(C::Tooltip, C::Fg, C::Primary, C::Normal);

        placeholder = disabledText;

        // Dark: FG/A4, light: Divider/Bright
        indicatorBorder = dark ? track : palette.getColor(C::View, C::Decoration, C::Border, C::Hovered);

        // Dark: BG/A5 (fields of the preferences), BG/B2 (frame of the toggles)
        raisedField = palette.getColor(C::TextField, C::Bg, C::Primary, C::Normal);
        deepField = palette.getColor(C::Slider, C::Bg, C::Secondary, C::Normal);
    }

    bool dark;

    QColor window;
    QColor button;
    QColor buttonHovered;
    QColor field;
    QColor fieldBorder;
    QColor separator;

    QColor text;
    QColor strongText;
    QColor secondaryText;
    QColor disabledText;
    QColor buttonText;
    QColor link;

    QColor accent;
    QColor accentPressed;
    QColor textOnAccent;
    QColor accentBackground;
    QColor accentBackgroundHovered;
    QColor accentText;

    QColor track;
    QColor scrollHandle;

    QColor menu;
    QColor menuBorder;
    QColor itemHovered;
    QColor itemHoveredText;

    QColor tooltip;
    QColor tooltipText;

    QColor placeholder;
    QColor indicatorBorder;
    QColor raisedField;
    QColor deepField;
};

void applyMenuStyle(MenuStyler* styler, const WidgetColors& colors)
{
    const int radius = useTranslucentMenus() ? 10 : 0;

    const QString styleSheet = StyleSheetBuilder(QStringLiteral(
        "QMenu {"
        "  background-color: %{background};"
        "  border: 1px solid %{border};"
        "  border-radius: %{radius}px;"
        "  padding: 4px;"
        "  color: %{text};"
        "  font-size: 14px;"
        "}"
        "QMenu::item {"
        "  padding: 8px 28px 8px 32px;"
        "  border-radius: 6px;"
        "  background-color: transparent;"
        "}"
        "QMenu::item:selected {"
        "  background-color: %{hover};"
        "  color: %{hoverText};"
        "}"
        "QMenu::item:disabled {"
        "  color: %{disabledText};"
        "}"
        "QMenu::separator {"
        "  height: 1px;"
        "  background-color: %{separator};"
        "  margin: 4px 10px;"
        "}"
        "QMenu::icon, QMenu::indicator {"
        "  left: 8px;"
        "}"))
        .set("background", colors.menu)
        .set("border", colors.menuBorder)
        .set("radius", radius)
        .set("text", colors.text)
        .set("hover", colors.itemHovered)
        .set("hoverText", colors.itemHoveredText)
        .set("disabledText", colors.disabledText)
        .set("separator", colors.separator)
        .result();

    QPalette menuPalette = QApplication::palette();
    for (QPalette::ColorRole role : { QPalette::Window, QPalette::Base, QPalette::Button, QPalette::AlternateBase })
        menuPalette.setColor(role, colors.menu);
    for (QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
    {
        menuPalette.setColor(QPalette::Active, role, colors.text);
        menuPalette.setColor(QPalette::Inactive, role, colors.text);
        menuPalette.setColor(QPalette::Disabled, role, colors.disabledText);
    }
    menuPalette.setColor(QPalette::Highlight, colors.itemHovered);
    menuPalette.setColor(QPalette::HighlightedText, colors.itemHoveredText);

    styler->setStyle(styleSheet, menuPalette);
}

QPalette makeApplicationPalette(const QPalette& base, const WidgetColors& colors)
{
    QPalette palette = base;

    const auto setAll = [&palette](QPalette::ColorRole role, const QColor& color) {
        palette.setColor(QPalette::Active, role, color);
        palette.setColor(QPalette::Inactive, role, color);
        palette.setColor(QPalette::Disabled, role, color);
    };

    const auto setEnabled = [&palette](QPalette::ColorRole role, const QColor& color, const QColor& disabledColor) {
        palette.setColor(QPalette::Active, role, color);
        palette.setColor(QPalette::Inactive, role, color);
        palette.setColor(QPalette::Disabled, role, disabledColor);
    };

    setAll(QPalette::Window, colors.window);
    setAll(QPalette::Base, colors.field);
    setAll(QPalette::AlternateBase, colors.dark ? colors.window.lighter(110) : colors.window.darker(104));
    setAll(QPalette::Button, colors.button);
    setAll(QPalette::ToolTipBase, colors.tooltip);
    setAll(QPalette::ToolTipText, colors.tooltipText);

    setEnabled(QPalette::WindowText, colors.text, colors.disabledText);
    setEnabled(QPalette::Text, colors.strongText, colors.disabledText);
    setEnabled(QPalette::ButtonText, colors.buttonText, colors.disabledText);
    setAll(QPalette::BrightText, colors.strongText);
    setAll(QPalette::PlaceholderText, colors.placeholder);

    setEnabled(QPalette::Highlight, colors.accent, colors.track);
    setAll(QPalette::HighlightedText, colors.textOnAccent);
    setAll(QPalette::Link, colors.link);
    setAll(QPalette::LinkVisited, colors.link);

    // Used by the styles to draw frames, lines and bevels:
    setAll(QPalette::Light, colors.buttonHovered);
    setAll(QPalette::Midlight, colors.button);
    setAll(QPalette::Mid, colors.separator);
    setAll(QPalette::Dark, colors.fieldBorder);
    setAll(QPalette::Shadow, colors.dark ? QColor(Qt::black) : colors.fieldBorder.darker(120));

    return palette;
}

QString makeApplicationStyleSheet(const WidgetColors& colors)
{
    return StyleSheetBuilder(QStringLiteral(
        // Tool tips
        "QToolTip {"
        "  background-color: %{tooltip};"
        "  color: %{tooltipText};"
        "  border: 1px solid %{border};"
        "  border-radius: 4px;"
        "  padding: 4px 6px;"
        "}"

        // Push buttons: secondary by default, the default button uses the accent
        "QPushButton {"
        "  background-color: %{button};"
        "  color: %{buttonText};"
        "  border: 1px solid transparent;"
        "  border-radius: 6px;"
        "  padding: 6px 16px;"
        "  min-height: 20px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "  background-color: %{buttonHovered};"
        "  color: %{strongText};"
        "}"
        "QPushButton:pressed {"
        "  background-color: %{button};"
        "}"
        "QPushButton:focus {"
        "  border-color: %{accent};"
        "}"
        "QPushButton:checked {"
        "  background-color: %{accentBackground};"
        "  color: %{accentText};"
        "}"
        "QPushButton:default {"
        "  background-color: %{accentBackground};"
        "  color: %{accentText};"
        "}"
        "QPushButton:default:hover {"
        "  background-color: %{accentBackgroundHovered};"
        "}"
        "QPushButton:disabled {"
        "  background-color: %{button};"
        "  color: %{disabledText};"
        "}"
        "QPushButton::menu-indicator {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: right center;"
        "  right: -8px;"
        "}"

        // Tool buttons
        "QToolButton {"
        "  background-color: transparent;"
        "  color: %{buttonText};"
        "  border: 1px solid transparent;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "}"
        "QToolButton:hover {"
        "  background-color: %{buttonHovered};"
        "  color: %{strongText};"
        "}"
        "QToolButton:checked {"
        "  background-color: %{accentBackground};"
        "  color: %{accentText};"
        "}"
        "QToolButton:focus {"
        "  border-color: %{accent};"
        "}"
        "QToolButton:disabled {"
        "  color: %{disabledText};"
        "}"
        // a tool button with a menu next to it is the main action of a dialog (e.g. "Play")
        "QToolButton[popupMode=\"1\"] {"
        "  background-color: %{accentBackground};"
        "  color: %{accentText};"
        "  padding: 6px 30px 6px 16px;"
        "  min-height: 20px;"
        "  font-weight: 600;"
        "}"
        "QToolButton[popupMode=\"1\"]:hover {"
        "  background-color: %{accentBackgroundHovered};"
        "}"
        "QToolButton[popupMode=\"1\"]:disabled {"
        "  background-color: %{button};"
        "  color: %{disabledText};"
        "}"
        "QToolButton::menu-button {"
        "  border: none;"
        "  border-left: 1px solid %{window};"
        "  width: 22px;"
        "}"
        "QToolButton::menu-arrow, QToolButton::menu-indicator {"
        "  image: %{chevronDown};"
        "  width: 12px;"
        "  height: 12px;"
        "}"
        "QToolButton::menu-arrow:disabled {"
        "  image: none;"
        "}"
        "QPushButton::menu-indicator {"
        "  image: %{chevronDown};"
        "  width: 12px;"
        "  height: 12px;"
        "}"

        // Text fields
        "QLineEdit, QTextEdit, QPlainTextEdit, QAbstractSpinBox {"
        "  background-color: %{field};"
        "  color: %{strongText};"
        "  border: 1px solid %{fieldBorder};"
        "  border-radius: 6px;"
        "  padding: 5px 8px;"
        "  selection-background-color: %{accentBackground};"
        "  selection-color: %{strongText};"
        "}"
        "QTextEdit, QPlainTextEdit {"
        "  padding: 4px;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QAbstractSpinBox:focus {"
        "  border-color: %{accent};"
        "}"
        "QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled, QAbstractSpinBox:disabled {"
        "  color: %{disabledText};"
        "  border-color: %{separator};"
        "}"
        "QLineEdit[readOnly=\"true\"] {"
        "  background-color: %{window};"
        "}"
        "QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {"
        "  border: none;"
        "  width: 20px;"
        "}"
        "QAbstractSpinBox::up-arrow {"
        "  image: %{chevronUp};"
        "  width: 10px;"
        "  height: 10px;"
        "}"
        "QAbstractSpinBox::down-arrow {"
        "  image: %{chevronDown};"
        "  width: 10px;"
        "  height: 10px;"
        "}"
        "QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::up-arrow:off,"
        "QAbstractSpinBox::down-arrow:disabled, QAbstractSpinBox::down-arrow:off {"
        "  image: none;"
        "}"

        // Combo boxes
        "QComboBox {"
        "  background-color: %{field};"
        "  color: %{strongText};"
        "  border: 1px solid %{fieldBorder};"
        "  border-radius: 6px;"
        "  padding: 5px 8px;"
        "  min-height: 20px;"
        "  selection-background-color: %{accentBackground};"
        "  selection-color: %{strongText};"
        "}"
        "QComboBox:focus, QComboBox:on {"
        "  border-color: %{accent};"
        "}"
        "QComboBox:disabled {"
        "  color: %{disabledText};"
        "  border-color: %{separator};"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 24px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: %{chevronDown};"
        "  width: 12px;"
        "  height: 12px;"
        "}"
        "QComboBox::down-arrow:disabled {"
        "  image: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %{menu};"
        "  color: %{text};"
        "  border: 1px solid %{border};"
        "  padding: 4px;"
        "  outline: 0;"
        "  selection-background-color: %{buttonHovered};"
        "  selection-color: %{strongText};"
        "}"

        // Tabs
        "QTabWidget::pane {"
        "  border: 1px solid %{separator};"
        "  border-radius: 8px;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background-color: transparent;"
        "  color: %{text};"
        "  border: none;"
        "  border-bottom: 2px solid transparent;"
        "  padding: 8px 14px;"
        "  margin-right: 4px;"
        "}"
        "QTabBar::tab:hover {"
        "  color: %{strongText};"
        "}"
        "QTabBar::tab:selected {"
        "  color: %{strongText};"
        "  border-bottom-color: %{accent};"
        "}"
        "QTabBar::tab:disabled {"
        "  color: %{disabledText};"
        "}"

        // Group boxes
        "QGroupBox {"
        "  border: 1px solid %{separator};"
        "  border-radius: 8px;"
        "  margin-top: 22px;"
        "  padding: 12px 8px 8px 8px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 8px;"
        "  padding: 0 4px;"
        "  color: %{strongText};"
        "}"

        // Check boxes and radio buttons, the indicators are drawn by the style
        "QCheckBox, QRadioButton {"
        "  spacing: 8px;"
        "}"
        "QCheckBox:disabled, QRadioButton:disabled {"
        "  color: %{disabledText};"
        "}"
        "QCheckBox::indicator, QRadioButton::indicator, QGroupBox::indicator,"
        "QAbstractItemView::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
        "QCheckBox::indicator:unchecked, QGroupBox::indicator:unchecked, QAbstractItemView::indicator:unchecked {"
        "  image: %{checkBoxOff};"
        "}"
        "QCheckBox::indicator:unchecked:hover, QGroupBox::indicator:unchecked:hover {"
        "  image: %{checkBoxOffHover};"
        "}"
        "QCheckBox::indicator:checked, QGroupBox::indicator:checked, QAbstractItemView::indicator:checked {"
        "  image: %{checkBoxOn};"
        "}"
        "QCheckBox::indicator:indeterminate, QAbstractItemView::indicator:indeterminate {"
        "  image: %{checkBoxPartial};"
        "}"
        "QCheckBox::indicator:unchecked:disabled, QGroupBox::indicator:unchecked:disabled,"
        "QAbstractItemView::indicator:unchecked:disabled {"
        "  image: %{checkBoxOffDisabled};"
        "}"
        "QCheckBox::indicator:checked:disabled, QGroupBox::indicator:checked:disabled,"
        "QAbstractItemView::indicator:checked:disabled {"
        "  image: %{checkBoxOnDisabled};"
        "}"
        "QRadioButton::indicator:unchecked {"
        "  image: %{radioOff};"
        "}"
        "QRadioButton::indicator:unchecked:hover {"
        "  image: %{radioOffHover};"
        "}"
        "QRadioButton::indicator:checked {"
        "  image: %{radioOn};"
        "}"
        "QRadioButton::indicator:unchecked:disabled {"
        "  image: %{radioOffDisabled};"
        "}"
        "QRadioButton::indicator:checked:disabled {"
        "  image: %{radioOnDisabled};"
        "}"

        // Item views
        "QAbstractItemView {"
        "  background-color: %{field};"
        "  alternate-background-color: %{window};"
        "  color: %{text};"
        "  border: 1px solid %{fieldBorder};"
        "  border-radius: 6px;"
        "  selection-background-color: %{accentBackground};"
        "  selection-color: %{strongText};"
        "  outline: 0;"
        "}"
        "QHeaderView {"
        "  background-color: transparent;"
        "  border: none;"
        "}"
        "QHeaderView::section {"
        "  background-color: %{window};"
        "  color: %{secondaryText};"
        "  border: none;"
        "  border-bottom: 1px solid %{separator};"
        "  padding: 6px 8px;"
        "}"

        // Scroll bars
        "QScrollBar:vertical {"
        "  background: transparent;"
        "  width: 10px;"
        "  margin: 0;"
        "}"
        "QScrollBar:horizontal {"
        "  background: transparent;"
        "  height: 10px;"
        "  margin: 0;"
        "}"
        "QScrollBar::handle {"
        "  background-color: %{scrollHandle};"
        "  border-radius: 3px;"
        "  margin: 2px;"
        "}"
        "QScrollBar::handle:hover {"
        "  background-color: %{secondaryText};"
        "}"
        "QScrollBar::handle:vertical {"
        "  min-height: 24px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  min-width: 24px;"
        "}"
        "QScrollBar::add-line, QScrollBar::sub-line {"
        "  width: 0;"
        "  height: 0;"
        "}"
        "QScrollBar::add-page, QScrollBar::sub-page {"
        "  background: none;"
        "}"

        // Sliders
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background-color: %{track};"
        "  border-radius: 2px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background-color: %{accent};"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  width: 12px;"
        "  height: 12px;"
        "  margin: -4px 0;"
        "  border-radius: 6px;"
        "  background-color: %{accent};"
        "}"
        "QSlider::groove:vertical {"
        "  width: 4px;"
        "  background-color: %{track};"
        "  border-radius: 2px;"
        "}"
        "QSlider::add-page:vertical {"
        "  background-color: %{accent};"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:vertical {"
        "  width: 12px;"
        "  height: 12px;"
        "  margin: 0 -4px;"
        "  border-radius: 6px;"
        "  background-color: %{accent};"
        "}"
        "QSlider::sub-page:disabled, QSlider::add-page:vertical:disabled, QSlider::handle:disabled {"
        "  background-color: %{disabledText};"
        "}"

        // Progress bars
        "QProgressBar {"
        "  background-color: %{field};"
        "  color: %{text};"
        "  border: 1px solid %{fieldBorder};"
        "  border-radius: 4px;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: %{accent};"
        "  border-radius: 3px;"
        "}"

        // Splitters
        "QSplitter::handle {"
        "  background-color: %{separator};"
        "}"

        // Preferences, see the "Settings" frames of the design
        "PrefsDialog {"
        "  background-color: %{field};"
        "}"
        "PrefsDialog #prefsCategories {"
        "  background-color: %{window};"
        "  border-radius: 6px;"
        "}"
        "#prefsCategories QLabel#prefsCategorySection {"
        "  color: %{secondaryText};"
        "  padding: 8px 8px 2px 8px;"
        "}"
        "#prefsCategories QToolButton {"
        "  background-color: transparent;"
        "  color: %{text};"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  font-weight: normal;"
        "}"
        "#prefsCategories QToolButton:hover {"
        "  background-color: %{buttonHovered};"
        "  color: %{strongText};"
        "}"
        "#prefsCategories QToolButton:checked {"
        "  background-color: %{buttonHovered};"
        "  color: %{strongText};"
        "}"
        "PrefsDialog QLabel#prefsPanelTitle {"
        "  color: %{strongText};"
        "  font-size: 18px;"
        "  padding: 6px 0 6px 0;"
        "}"
        "QScrollArea#prefsPanelScroller, QScrollArea#prefsPanelScroller > QWidget#qt_scrollarea_viewport,"
        "QWidget#prefsPanelContent {"
        "  background-color: transparent;"
        "}"
        "PrefsDialog QGroupBox {"
        "  border: none;"
        "  border-top: 1px solid %{separator};"
        "  border-radius: 0;"
        "  margin-top: 30px;"
        "  padding: 14px 0 0 0;"
        "  font-size: 16px;"
        "}"
        "PrefsDialog QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 0;"
        "  padding: 0 0 4px 0;"
        "  color: %{strongText};"
        "}"
        "PrefsDialog QLineEdit, PrefsDialog QAbstractSpinBox, PrefsDialog QComboBox {"
        "  background-color: %{raisedField};"
        "  border-color: %{raisedField};"
        "}"
        "PrefsDialog QCheckBox::indicator {"
        "  width: 40px;"
        "  height: 22px;"
        "}"
        "PrefsDialog QCheckBox::indicator:unchecked {"
        "  image: %{toggleOff};"
        "}"
        "PrefsDialog QCheckBox::indicator:unchecked:hover {"
        "  image: %{toggleOffHover};"
        "}"
        "PrefsDialog QCheckBox::indicator:checked {"
        "  image: %{toggleOn};"
        "}"
        "PrefsDialog QCheckBox::indicator:unchecked:disabled {"
        "  image: %{toggleOffDisabled};"
        "}"
        "PrefsDialog QCheckBox::indicator:checked:disabled {"
        "  image: %{toggleOnDisabled};"
        "}"

        // First run wizard, see the "Onboarding" frames of the design
        "FirstRunWizard, FirstRunWizard QWizardPage {"
        "  background-color: %{field};"
        "}"
        "FirstRunWizard QLabel#wizardTitle, FirstRunWizard QLabel#colorLabel, FirstRunWizard QLabel#layoutLabel {"
        "  color: %{strongText};"
        "  font-size: 22px;"
        "  font-weight: 300;"
        "}"
        "FirstRunWizard QLabel#wizardDescription {"
        "  color: %{text};"
        "  font-size: 14px;"
        "}"
        "FirstRunWizard QLabel#explainerLabel, FirstRunWizard QLabel#layoutExplainer, FirstRunWizard QLabel#wizardHint {"
        "  color: %{secondaryText};"
        "}"
        "FirstRunWizard QFrame#horizontalFrame_policy {"
        "  background-color: %{window};"
        "  border-radius: 8px;"
        "}"
        "FirstRunWizard QLabel#privacy {"
        "  color: %{strongText};"
        "  font-size: 15px;"
        "}"
        "FirstRunWizard QTextEdit#policy {"
        "  background-color: transparent;"
        "  border: none;"
        "  color: %{text};"
        "}"
        "FirstRunWizard QCheckBox::indicator {"
        "  width: 40px;"
        "  height: 22px;"
        "}"
        "FirstRunWizard QCheckBox::indicator:unchecked {"
        "  image: %{toggleOff};"
        "}"
        "FirstRunWizard QCheckBox::indicator:unchecked:hover {"
        "  image: %{toggleOffHover};"
        "}"
        "FirstRunWizard QCheckBox::indicator:checked {"
        "  image: %{toggleOn};"
        "}"
        "FirstRunWizard QPushButton#lightImage, FirstRunWizard QPushButton#darkImage,"
        "FirstRunWizard QPushButton#daynightImage, FirstRunWizard QPushButton#modernImage,"
        "FirstRunWizard QPushButton#classicImage {"
        "  background-color: transparent;"
        "  border: 2px solid transparent;"
        "  border-radius: 14px;"
        "  padding: 6px;"
        "}"
        "FirstRunWizard QPushButton#lightImage:hover, FirstRunWizard QPushButton#darkImage:hover,"
        "FirstRunWizard QPushButton#daynightImage:hover, FirstRunWizard QPushButton#modernImage:hover,"
        "FirstRunWizard QPushButton#classicImage:hover {"
        "  border-color: %{separator};"
        "}"
        "FirstRunWizard QPushButton#lightImage:checked, FirstRunWizard QPushButton#darkImage:checked,"
        "FirstRunWizard QPushButton#daynightImage:checked, FirstRunWizard QPushButton#modernImage:checked,"
        "FirstRunWizard QPushButton#classicImage:checked {"
        "  border-color: %{accent};"
        "}"
        // the scheme and layout names are shown below their preview, without radio indicator
        "FirstRunWizard QRadioButton {"
        "  color: %{text};"
        "  spacing: 0;"
        "}"
        "FirstRunWizard QRadioButton:checked {"
        "  color: %{strongText};"
        "}"
        "FirstRunWizard QRadioButton::indicator {"
        "  width: 0;"
        "  height: 0;"
        "  image: none;"
        "}"
        "FirstRunWizard QLabel#wizardShortcutLabel {"
        "  color: %{text};"
        "}"
        "FirstRunWizard QLabel#wizardKeyCap {"
        "  background-color: %{button};"
        "  color: %{strongText};"
        "  border: 1px solid %{fieldBorder};"
        "  border-bottom-width: 2px;"
        "  border-radius: 5px;"
        "  padding: 2px 8px;"
        "  min-width: 12px;"
        "  font-weight: 600;"
        "}"))
        .set("toggleOff", toggleImage("toggle-off", colors.deepField, colors.window, colors.indicatorBorder, false))
        .set("toggleOffHover", toggleImage("toggle-off-hover", colors.deepField, colors.window, colors.secondaryText, false))
        .set("toggleOn", toggleImage("toggle-on", colors.deepField, colors.accentBackground, colors.accent, true))
        .set("toggleOffDisabled", toggleImage("toggle-off-disabled", colors.deepField, colors.deepField, colors.separator, false))
        .set("toggleOnDisabled", toggleImage("toggle-on-disabled", colors.deepField, colors.deepField, colors.disabledText, true))
        .set("raisedField", colors.raisedField)
        .set("chevronDown", chevronImage("chevron-down", colors.buttonText, false))
        .set("chevronUp", chevronImage("chevron-up", colors.buttonText, true))
        .set("checkBoxOff", checkBoxImage("checkbox-off", colors.field, colors.indicatorBorder, QColor()))
        .set("checkBoxOffHover", checkBoxImage("checkbox-off-hover", colors.field, colors.secondaryText, QColor()))
        .set("checkBoxOn", checkBoxImage("checkbox-on", colors.accent, colors.accent, colors.textOnAccent))
        .set("checkBoxPartial", checkBoxImage("checkbox-partial", colors.accentBackground, colors.accent, QColor()))
        .set("checkBoxOffDisabled", checkBoxImage("checkbox-off-disabled", colors.window, colors.separator, QColor()))
        .set("checkBoxOnDisabled", checkBoxImage("checkbox-on-disabled", colors.separator, colors.separator, colors.disabledText))
        .set("radioOff", radioButtonImage("radio-off", colors.field, colors.indicatorBorder, QColor()))
        .set("radioOffHover", radioButtonImage("radio-off-hover", colors.field, colors.secondaryText, QColor()))
        .set("radioOn", radioButtonImage("radio-on", colors.field, colors.accent, colors.accent))
        .set("radioOffDisabled", radioButtonImage("radio-off-disabled", colors.window, colors.separator, QColor()))
        .set("radioOnDisabled", radioButtonImage("radio-on-disabled", colors.window, colors.separator, colors.disabledText))
        .set("window", colors.window)
        .set("button", colors.button)
        .set("buttonHovered", colors.buttonHovered)
        .set("buttonText", colors.buttonText)
        .set("field", colors.field)
        .set("fieldBorder", colors.fieldBorder)
        .set("separator", colors.separator)
        .set("border", colors.menuBorder)
        .set("text", colors.text)
        .set("strongText", colors.strongText)
        .set("secondaryText", colors.secondaryText)
        .set("disabledText", colors.disabledText)
        .set("accent", colors.accent)
        .set("accentBackground", colors.accentBackground)
        .set("accentBackgroundHovered", colors.accentBackgroundHovered)
        .set("accentText", colors.accentText)
        .set("track", colors.track)
        .set("scrollHandle", colors.scrollHandle)
        .set("menu", colors.menu)
        .set("tooltip", colors.tooltip)
        .set("tooltipText", colors.tooltipText)
        .result();
}

} // anonymous namespace

void applyWidgetStyle(const SystemPalette& palette)
{
    if (!qApp)
        return;

    const WidgetColors colors(palette);

    // The colors of the system color scheme can be built from the application
    // palette (Qt theme provider), keep it then. Otherwise (e.g. Windows theme
    // provider) the system scheme uses the palettes of the redesign:
    if (palette.source() == ColorSchemeModel::System && palette.hasExternalColors())
    {
        restoreSystemWidgetPalette();
    }
    else
    {
        std::optional<QPalette>& original = originalApplicationPalette();
        if (!original)
            original = QApplication::palette();

        QApplication::setPalette(makeApplicationPalette(*original, colors));
    }

    qApp->setStyleSheet(makeApplicationStyleSheet(colors));

    if (MenuStyler* styler = menuStyler())
        applyMenuStyle(styler, colors);
}

void restoreSystemWidgetPalette()
{
    std::optional<QPalette>& original = originalApplicationPalette();
    if (!original || !qApp)
        return;

    QApplication::setPalette(*original);
    original.reset();
}
