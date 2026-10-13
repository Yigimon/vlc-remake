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
#ifndef VLC_QT_WIDGETMENUSTYLE_HPP
#define VLC_QT_WIDGETMENUSTYLE_HPP

class SystemPalette;

/**
 * Applies the look of the redesign to the Qt Widgets menus (QMenu), such as
 * the main menu and the context menus, so that they follow the color scheme
 * of the interface instead of the platform style.
 *
 * It is meant to be called with the main palette of the interface whenever
 * that palette changes.
 */
void applyWidgetMenuStyle(const SystemPalette& palette);

#endif // VLC_QT_WIDGETMENUSTYLE_HPP
