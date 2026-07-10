#include <exception>
#include <iostream>

#include "configuration.h"
#include "configuration_legacy.h"
#include "settings.h"
#include "settings_legacy.h"

#undef Config

extern "C" bool configuration_legacy_sync(void)
{
	try
	{
		if (configuration_is_quiet())
			std::clog.rdbuf(nullptr);
		settings_legacy_sync_from_c(&::Config);
	}
	catch (const std::exception &error)
	{
		std::cerr << "Error while synchronizing legacy configuration: "
		          << error.what() << "\n";
		return false;
	}
	catch (...)
	{
		std::cerr << "Error while synchronizing legacy configuration\n";
		return false;
	}
	return true;
}
