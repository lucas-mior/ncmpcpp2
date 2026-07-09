/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef NCMPCPP_TITLE_LEGACY_H
#define NCMPCPP_TITLE_LEGACY_H

#include <cstddef>
#include <cstring>
#include <limits>
#include <string>

#include "app_controller.h"
#include "curses/window.h"
#include "settings_legacy.h"
#include "title.h"

static inline int32
title_legacy_size_to_int32(std::size_t size) {
    std::size_t max;

    max = static_cast<std::size_t>(std::numeric_limits<int32>::max());
    if (size > max) {
        return std::numeric_limits<int32>::max();
    }
    return static_cast<int32>(size);
}

static inline int32
title_legacy_cstring_len(char *string) {
    return title_legacy_size_to_int32(std::strlen(string));
}

static inline char *
title_legacy_current_screen_title() {
    NcScreen *screen;
    char *title;

    screen = app_controller_current_screen();
    if (screen == nullptr) {
        return const_cast<char *>("");
    }

    title = nc_screen_title(screen);
    if (title == nullptr) {
        return const_cast<char *>("");
    }

    return title;
}

inline void
windowTitle(const std::string &title) {
    if (!Config.set_window_title) {
        return;
    }

    ncm_window_title_write(const_cast<char *>(title.data()),
                           title_legacy_size_to_int32(title.size()));
    return;
}

inline void
drawHeader() {
    char *title;

    title = title_legacy_current_screen_title();
    ncm_title_draw_header_with_config(
        title, title_legacy_cstring_len(title), Config.header_visibility,
        Config.design, Config.volume_color.cFormattedColor(),
        Config.alternative_ui_separator_color.cFormattedColor());
    return;
}

#endif // NCMPCPP_TITLE_LEGACY_H
