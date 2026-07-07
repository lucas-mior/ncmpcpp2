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

#include <cassert>
#include <string>

#include "c/ncm_mpd_client.h"

#include "charset.h"
#include "curses/formatted_color.h"
#include "app_controller.h"
#include "global.h"
#include "ui_state_legacy.h"
#include "screens/outputs.h"
#include "screens/screen_switcher.h"
#include "screens/screen_legacy.h"
#include "settings_legacy.h"
#include "statusbar_legacy.h"
#include "title_legacy.h"


namespace {

NcScroll toNcScroll(enum NcScroll where)
{
    switch (where)
    {
        case NC_SCROLL_UP:
            return NC_SCROLL_UP;
        case NC_SCROLL_DOWN:
            return NC_SCROLL_DOWN;
        case NC_SCROLL_PAGE_UP:
            return NC_SCROLL_PAGE_UP;
        case NC_SCROLL_PAGE_DOWN:
            return NC_SCROLL_PAGE_DOWN;
        case NC_SCROLL_HOME:
            return NC_SCROLL_HOME;
        case NC_SCROLL_END:
            return NC_SCROLL_END;
    }
    return NC_SCROLL_UP;
}

NcBorder toNcBorder(NC::Border border)
{
    NcBorder result = {};

    if (border)
    {
        result.enabled = true;
        result.color = NC::toNcColor(*border);
    }
    return result;
}

}

Outputs *myOutputs;

Outputs::Outputs()
{
    nc_outputs_screen_init(&m_screen,
                           makeHooks(),
                           0,
                           COLS,
                           ui_state_legacy_main_start_y(),
                           ui_state_legacy_main_height(),
                           NC::toNcColor(Config.main_color),
                           toNcBorder(NC::Border()),
                           static_cast<int64>(Config.lines_scrolled),
                           Config.mouse_list_scroll_whole_page);
    nc_menu_set_cyclic_scrolling(&m_screen.menu,
                                 Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(&m_screen.menu, Config.centered_cursor);
    nc_outputs_screen_set_highlight_prefix(
        &m_screen, Config.current_item_prefix.cBuffer());
    nc_outputs_screen_set_highlight_suffix(
        &m_screen, Config.current_item_suffix.cBuffer());

    bool register_success = app_controller_register_screen(nativeScreen());
    assert(register_success);
    (void)register_success;
}

Outputs::~Outputs()
{
    if (app_controller_is_screen_registered(nativeScreen()))
    {
        app_controller_unregister_screen(nativeScreen());
    }
    nc_outputs_screen_destroy(&m_screen);
}

bool Outputs::isActiveWindow(const NC::Window &w_) const
{
    (void)w_;
    return false;
}

NC::Window *Outputs::activeWindow()
{
    return nullptr;
}

const NC::Window *Outputs::activeWindow() const
{
    return nullptr;
}

void Outputs::refresh()
{
    nc_screen_refresh(nativeScreen());
}

void Outputs::refreshWindow()
{
    nc_screen_refresh_window(nativeScreen());
}

void Outputs::scroll(enum NcScroll where)
{
    nc_screen_scroll(nativeScreen(), toNcScroll(where));
}

void Outputs::switchTo()
{
    nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
    app_controller_switch_to_screen(nativeScreen());
}

void Outputs::resize()
{
    nc_screen_resize(nativeScreen());
}

int Outputs::windowTimeout()
{
    return nc_screen_window_timeout(nativeScreen());
}

std::string Outputs::title()
{
    char *result = nc_screen_title(nativeScreen());

    if (result == nullptr)
        return "";
    return result;
}

void Outputs::update()
{
    nc_screen_update(nativeScreen());
}

void Outputs::mouseButtonPressed(MEVENT me)
{
    nc_screen_mouse_button_pressed(nativeScreen(), me);
}

bool Outputs::isLockable()
{
    return nc_screen_is_lockable(nativeScreen());
}

bool Outputs::isMergable()
{
    return nc_screen_is_mergable(nativeScreen());
}

NcScreen *Outputs::nativeScreen()
{
    return nc_outputs_screen_base(&m_screen);
}

const NcScreen *Outputs::nativeScreen() const
{
    return nc_outputs_screen_base(const_cast<NcOutputsScreen *>(&m_screen));
}

void Outputs::fetchList()
{
    nc_outputs_screen_fetch_list(&m_screen);
}

void Outputs::toggleOutput()
{
    nc_outputs_screen_toggle_current(&m_screen);
}

void Outputs::loadOutputs(NcOutputsScreen *screen)
{
    NcmError error = {};
    NcmMpdOutputList outputs;

    ncm_mpd_output_list_init(&outputs);
    if (!ncm_mpd_client_get_outputs(&global_mpd, &outputs, &error))
    {
        Statusbar::printf("Could not fetch outputs: %s", error.message);
        ncm_mpd_output_list_destroy(&outputs);
        return;
    }

    for (int32 i = 0; i < outputs.count; i += 1)
    {
        NcmMpdOutput *output = &outputs.items[i];
        std::string name = Charset::utf8ToLocale(output->name);

        nc_outputs_screen_add_output(screen,
                                     output->id,
                                     const_cast<char *>(name.data()),
                                     static_cast<int32>(name.size()),
                                     output->enabled);
    }
    ncm_mpd_output_list_destroy(&outputs);
}

bool Outputs::toggleOutput(uint32 id, bool enabled,
                           char *name, int32 name_len)
{
    NcmError error = {};
    bool ok;
    std::string name_string(name, static_cast<size_t>(name_len));

    if (enabled)
    {
        ok = ncm_mpd_client_disable_output(&global_mpd, id, &error);
        if (!ok)
        {
            Statusbar::printf("Could not disable output \"%s\": %s",
                              name_string.c_str(), error.message);
            return false;
        }
        Statusbar::printf("Output \"%s\" disabled",
                          name_string.c_str());
    }
    else
    {
        ok = ncm_mpd_client_enable_output(&global_mpd, id, &error);
        if (!ok)
        {
            Statusbar::printf("Could not enable output \"%s\": %s",
                              name_string.c_str(), error.message);
            return false;
        }
        Statusbar::printf("Output \"%s\" enabled",
                          name_string.c_str());
    }
    return true;
}

NcOutputsHooks Outputs::makeHooks()
{
    NcOutputsHooks hooks = {};

    hooks.fetch_outputs = fetchOutputsHook;
    hooks.toggle_output = toggleOutputHook;
    hooks.switch_to = switchToHook;
    hooks.resize_layout = resizeLayoutHook;
    hooks.resize_background = resizeBackgroundHook;
    hooks.destroy = destroyHook;
    hooks.user = this;
    return hooks;
}

void Outputs::fetchOutputsHook(void *user, NcOutputsScreen *screen)
{
    static_cast<Outputs *>(user)->loadOutputs(screen);
}

bool Outputs::toggleOutputHook(void *user, uint32 id, bool enabled,
                               char *name, int32 name_len)
{
    return static_cast<Outputs *>(user)->toggleOutput(id, enabled,
                                                      name, name_len);
}

void Outputs::switchToHook(void *user)
{
    Outputs *outputs = static_cast<Outputs *>(user);

    if (screenLegacySwitchChanged())
        SwitchTo::finishNativeSwitch(outputs);
    drawHeader();
}

void Outputs::resizeLayoutHook(void *user, NcOutputsScreen *screen)
{
    Outputs *outputs = static_cast<Outputs *>(user);
    size_t x_offset;
    size_t width;

    outputs->getWindowResizeParams(x_offset, width);
    nc_outputs_screen_set_geometry(screen,
                                   static_cast<int64>(x_offset),
                                   static_cast<int64>(width),
                                   ui_state_legacy_main_start_y(),
                                   ui_state_legacy_main_height());
    outputs->hasToBeResized = false;
}

void Outputs::resizeBackgroundHook(void *user)
{
    (void)user;
}

void Outputs::destroyHook(void *user)
{
    Outputs *outputs = static_cast<Outputs *>(user);

    if (myOutputs == outputs)
        myOutputs = nullptr;
    delete outputs;
}

