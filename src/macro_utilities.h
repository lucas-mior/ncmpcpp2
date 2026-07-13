#ifndef NCMPCPP_MACRO_UTILITIES_H
#define NCMPCPP_MACRO_UTILITIES_H

#include <string>

#include "c/ncm_error.h"
#include "c/ncm_macro_utilities.h"
#include "curses/window.h"

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


#endif // NCMPCPP_MACRO_UTILITIES_H
