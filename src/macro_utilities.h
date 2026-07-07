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

#ifndef NCMPCPP_MACRO_UTILITIES_H
#define NCMPCPP_MACRO_UTILITIES_H

#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "actions_legacy.h"
#include "c/ncm_error.h"
#include "c/ncm_macro_utilities.h"
#include "curses/window.h"
#include "screens/screen_cpp_legacy.h"
#include "screens/screen_type.h"
#include "utility/string.h"

inline void runExternalConsoleCommand(const std::string &cmd)
{
	NcmError error;

	ncm_error_clear(&error);
	NC::pauseScreen();
	ncm_macro_run_external_console_command(
		const_cast<char *>(cmd.data()), static_cast<int32>(cmd.size()), &error);
	NC::unpauseScreen();
}

inline void runExternalCommand(const std::string &cmd, bool block)
{
	NcmError error;

	ncm_error_clear(&error);
	ncm_macro_run_external_command(
		const_cast<char *>(cmd.data()), static_cast<int32>(cmd.size()),
		block, &error);
}

namespace Actions {

struct PushCharacters: BaseAction
{
	using WindowProvider = NC::Window *(*)();

	PushCharacters(WindowProvider window, std::vector<NcKey> &&queue)
		: BaseAction(Type::MacroUtility, "push_characters")
		, m_window(window)
		, m_queue(queue)
	{
		assert(window != nullptr);
		std::vector<std::string> keys;
		for (const auto &key : queue)
		{
			char key_name[64];
			nc_key_name(key, key_name, sizeof(key_name));
			keys.push_back(key_name);
		}
		m_name += " \"";
		m_name += join<std::string>(keys, ", ");
		m_name += "\"";
	}

private:
	virtual void run() override
	{
		for (auto it = m_queue.begin(); it != m_queue.end(); ++it)
			m_window()->pushChar(*it);
	}

	WindowProvider m_window;
	std::vector<NcKey> m_queue;
};

struct RequireRunnable: BaseAction
{
	RequireRunnable(std::shared_ptr<BaseAction> action)
		: BaseAction(Type::MacroUtility, "require_runnable")
		, m_action(std::move(action))
	{
		assert(m_action != nullptr);
		m_name += " \"";
		m_name += m_action->name();
		m_name += "\"";
	}

private:
	virtual bool canBeRun() override
	{
		return m_action->canBeRun();
	}
	virtual void run() override { }

	std::shared_ptr<BaseAction> m_action;
};

struct RequireScreen: BaseAction
{
	RequireScreen(ScreenType screen_type)
		: BaseAction(Type::MacroUtility, "require_screen")
		, m_screen_type(screen_type)
	{
		m_name += " \"";
		m_name += screenTypeToString(m_screen_type);
		m_name += "\"";
	}

private:
	virtual bool canBeRun() override
	{
		return screenLegacyCurrent()->type() == m_screen_type;
	}
	virtual void run() override { }

	ScreenType m_screen_type;
};

struct RunExternalCommand: BaseAction
{
	RunExternalCommand(std::string &&command)
		: BaseAction(Type::MacroUtility, "run_external_command")
		, m_command(std::move(command))
	{
		m_name += " \"";
		m_name += m_command;
		m_name += "\"";
	}

private:
	virtual void run() override
	{
		runExternalCommand(m_command, true);
	}

	std::string m_command;
};

struct RunExternalConsoleCommand: BaseAction
{
	RunExternalConsoleCommand(std::string &&command)
		: BaseAction(Type::MacroUtility, "run_external_console_command")
		, m_command(std::move(command))
	{
		m_name += " \"";
		m_name += m_command;
		m_name += "\"";
	}

private:
	virtual void run() override
	{
		runExternalConsoleCommand(m_command);
	}

	std::string m_command;
};

}

#endif // NCMPCPP_MACRO_UTILITIES_H
