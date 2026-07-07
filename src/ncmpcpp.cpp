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

#include <cerrno>
#include <clocale>
#include <csignal>
#include <cstring>

#include <locale>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include "mpdpp.h"

#include "actions_legacy.h"
#include "bindings_legacy.h"
#include "screens/browser.h"
#include "charset.h"
#include "configuration_legacy.h"
#include "global.h"
#include "ui_state.h"
#include "screens/screen_legacy.h"
#include "helpers_legacy.h"
#include "screens/lyrics.h"
#include "screens/playlist.h"
#include "settings_legacy.h"
#include "status_legacy.h"
#include "statusbar_legacy.h"
#include "screens/visualizer.h"
#include "title_legacy.h"
#include "utility/conversion.h"

namespace ph = std::placeholders;

namespace {

std::ofstream errorlog;
std::streambuf *cerr_buffer;
std::streambuf *clog_buffer;

volatile bool run_resize_screen = false;

void sighandler(int sig)
{
	if (sig == SIGWINCH)
		run_resize_screen = true;
#if defined(__sun) && defined(__SVR4)
	// in solaris it is needed to reinstall the handler each time it's executed
	signal(sig, sighandler);
#endif // __sun && __SVR4
}

void do_at_exit()
{
	// restore old cerr & clog buffers
	std::cerr.rdbuf(cerr_buffer);
	std::clog.rdbuf(clog_buffer);
	errorlog.close();
	Mpd.Disconnect();
	global_state_destroy();
	NC::destroyScreen();
	windowTitle("");
}

}

int main(int argc, char **argv)
{
	
	global_state_init();

	std::setlocale(LC_ALL, "");
	std::locale::global(Charset::internalLocale());

	// clog might be overriden in configure, so preserve the original buffer.
	clog_buffer = std::clog.rdbuf();

	if (!configure(argc, argv))
		return 0;

	// always execute these commands, even if ncmpcpp use exit function
	atexit(do_at_exit);

	// redirect std::cerr output to the error.log file
	errorlog.open((Config.ncmpcpp_directory + "error.log").c_str(), std::ios::app);
	cerr_buffer = std::cerr.rdbuf();
	std::cerr.rdbuf(errorlog.rdbuf());

	signal(SIGPIPE, SIG_IGN);
	signal(SIGWINCH, sighandler);

	Mpd.setNoidleCallback(Status::update);

	NC::initScreen(Config.colors_enabled, Config.mouse_support);

	Actions::OriginalStatusbarVisibility = Config.statusbar_visibility;

	if (Config.design == NCM_DESIGN_ALTERNATIVE)
		Config.statusbar_visibility = 0;

	Actions::setWindowsDimensions();
	Actions::initializeScreens();

	auto header_window = new NC::Window(0, 0, COLS, Actions::HeaderHeight, "", Config.header_color, NC::Border());
	ui_state_set_header_window(header_window);
	if (Config.header_visibility || Config.design == NCM_DESIGN_ALTERNATIVE)
		header_window->display();

	auto footer_window = new NC::Window(0, Actions::FooterStartY, COLS, Actions::FooterHeight, "", Config.statusbar_color, NC::Border());
	ui_state_set_footer_window(footer_window);
	footer_window->setPromptHook(Statusbar::Helpers::mainHook);

	// initialize global timer
	global_timer_update(nullptr);

	// initialize global random number generator
	ncm_random_seed_from_time(&global_random, nullptr);

	// initialize playlist
	myPlaylist->switchTo();

	// go to startup screen
	if (Config.startup_screen_type != screenLegacyCurrent()->type())
	{
		auto startup_screen = toScreen(Config.startup_screen_type);
		assert(startup_screen != nullptr);
		startup_screen->switchTo();
	}

	// lock current screen and go to the slave one if applicable
	if (Config.startup_slave_screen_type)
	{
		auto slave_screen_type = *Config.startup_slave_screen_type;
		bool screen_locked = screenLegacyCurrent()->lock();
		if (screen_locked && slave_screen_type != screenLegacyCurrent()->type())
		{
			auto slave_screen = toScreen(slave_screen_type);
			assert(slave_screen != nullptr);
			slave_screen->switchTo();
			if (!Config.startup_slave_screen_focus)
				Actions::execute(NCM_ACTION_MASTER_SCREEN);
		}
	}

	// local variables
	bool key_pressed = false;
	auto input = NC_KEY_NONE;
	NcmTimePoint connect_attempt = {};
	auto update_environment_action =
		Actions::runtimeAction(NCM_ACTION_UPDATE_ENVIRONMENT);
	assert(update_environment_action != nullptr);
	auto update_environment = static_cast<Actions::UpdateEnvironment &>(
		*update_environment_action);

	while (!Actions::ExitMainLoop)
	{
		try
		{
			if (!Mpd.Connected() && global_timer_elapsed_ms(connect_attempt) > 1000)
			{
				connect_attempt = global_timer;
				// reset local status info
				Status::clear();
				// clear mpd callback
				static_cast<NC::Window *>(ui_state_footer_window())->clearFDCallbacksList();
				try
				{
					Mpd.Connect();
					if (Mpd.Version() < 16)
					{
							Mpd.Disconnect();
							throw MPD::ClientError(MPD_ERROR_STATE, "MPD < 0.16.0 is not supported", false);
					}
				}
				catch (MPD::ClientError &e)
				{
					Status::handleClientError(e);
				}
			}

			if (run_resize_screen)
			{
				Actions::resizeScreen(true);
				run_resize_screen = false;
			}

			update_environment.run(!key_pressed, key_pressed, false);

			input = ncm_read_key(static_cast<NC::Window *>(ui_state_footer_window())->nativeWindow());
			key_pressed = input != NC_KEY_NONE;
			if (!key_pressed)
				continue;

			// The reason we want to update timer here is that if the timer is updated
			// in Status::trace, then Key::read usually blocks for 500ms and if key is
			// pressed 400ms after Key::read was called, we end up with global_timer that is
			// ~400ms inaccurate. On the other hand, if keys are being pressed, we don't
			// want to update timer in both Status::trace and here. Therefore we update
			// timer in Status::trace only if there was no recent input.
			global_timer_update(nullptr);

			try
			{
				auto k = bindings_legacy_get(input);
				bool executed = false;
				for (int32 i = 0; i < k.len; i += 1)
				{
					if (bindings_legacy_execute(k.data + i))
					{
						executed = true;
						break;
					}
				}
				(void)executed;
			}
			catch (ConversionError &e)
			{
				Statusbar::printf("Invalid value: %1%", e.value());
			}
			catch (OutOfBounds &e)
			{
				Statusbar::printf("Error: %1%", e.errorMessage());
			}
			catch (NC::PromptAborted &)
			{
				Statusbar::printf("Action aborted");
			}

			if (screenLegacyCurrent() == myPlaylist)
				myPlaylist->enableHighlighting();
		}
		catch (MPD::ClientError &e)
		{
			Status::handleClientError(e);
		}
		catch (MPD::ServerError &e)
		{
			Status::handleServerError(e);
		}
		catch (std::exception &e)
		{
			Statusbar::printf("Unexpected error: %1%", e.what());
		}
	}
	return 0;
}
