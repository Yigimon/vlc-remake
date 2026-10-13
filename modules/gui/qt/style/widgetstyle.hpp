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
#ifndef VLC_QT_WIDGETSTYLE_HPP
#define VLC_QT_WIDGETSTYLE_HPP

class SystemPalette;

/**
 * Applies the look of the redesign to the Qt Widgets parts of the interface
 * (menus, dialogs, preferences...), so that they follow the color scheme of
 * the interface instead of the platform style:
 *
 * - the menus (QMenu) get a dedicated style sheet and palette
 * - the application palette is replaced by one built from the interface
 *   palette, unless the "system" color scheme is used
 * - an application style sheet gives the common widgets (buttons, text
 *   fields, combo boxes, tabs, group boxes, scroll bars, sliders...) the
 *   shapes of the design
 *
 * It is meant to be called with the main palette of the interface whenever
 * that palette changes.
 */
void applyWidgetStyle(const SystemPalette& palette);

/**
 * Restores the application palette that was in use before
 * applyWidgetStyle() replaced it, so that it can be read again as the
 * system palette.
 */
void restoreSystemWidgetPalette();

#endif // VLC_QT_WIDGETSTYLE_HPP
