#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "bindings.h"
#include "configuration_legacy.h"
#include "config.h"
#include "settings.h"
#include "settings_legacy.h"

#undef Config
#include "utility/string.h"

using std::cerr;
using std::cout;

namespace {

const char *env_home;

const char *current_song_default_format = "{{{(%l) }{{%a - }%t}}|{%f}}";

struct CommandLineOptions
{
	std::string host = "localhost";
	int port = 6600;
	bool host_provided = false;
	bool port_provided = false;
	bool current_song = false;
	std::string current_song_format = current_song_default_format;
	std::vector<std::string> config_paths;
	bool ignore_config_errors = false;
	bool test_lyrics_fetchers = false;
	std::vector<std::string> bindings_paths;
	bool screen = false;
	std::string screen_name;
	bool slave_screen = false;
	std::string slave_screen_name;
	bool help = false;
	bool version = false;
	bool quiet = false;
};


void readConfigurationPaths(const std::vector<std::string> &paths,
                            bool ignore_errors, bool quiet)
{
	NcmStringViewArray c_paths;
	NcmError error;

	ncm_string_view_array_init(&c_paths);
	for (const auto &path : paths)
	{
		auto view = ncm_string_view_array_append(&c_paths);
		if (view == nullptr)
		{
			ncm_string_view_array_destroy(&c_paths);
			throw std::bad_alloc();
		}
		view->data = const_cast<char *>(path.data());
		view->len = static_cast<int32>(path.size());
	}

	ncm_error_clear(&error);
	bool result = configuration_read(
		&::Config, &c_paths, ignore_errors, quiet, &error);
	ncm_string_view_array_destroy(&c_paths);
	if (!result)
		throw std::runtime_error(error.message);

	settings_legacy_sync_from_c(&::Config);
}

bool readBindingPaths(const std::vector<std::string> &paths)
{
	std::vector<char *> c_paths;
	std::vector<int32> c_path_lens;
	NcmError error;

	c_paths.reserve(paths.size());
	c_path_lens.reserve(paths.size());
	for (const auto &path : paths)
	{
		c_paths.push_back(const_cast<char *>(path.data()));
		c_path_lens.push_back(static_cast<int32>(path.size()));
	}

	ncm_error_clear(&error);
	return ncm_bindings_configuration_read_paths(
		&Bindings,
		c_paths.empty() ? nullptr : c_paths.data(),
		c_path_lens.empty() ? nullptr : c_path_lens.data(),
		static_cast<int32>(c_paths.size()),
		&error);
}

int parseInt(const std::string &s)
{
	std::istringstream is(s);
	int result;
	if (!(is >> result))
		throw std::invalid_argument(s);
	is >> std::ws;
	if (!is.eof())
		throw std::invalid_argument(s);
	return result;
}

int parseOptionInt(const std::string &s, const std::string &option)
{
	try
	{
		return parseInt(s);
	}
	catch (std::invalid_argument &)
	{
		throw std::runtime_error("the argument ('" + s + "') for option '"
		                         + option + "' is invalid");
	}
}

std::string requireValue(int argc, char **argv, int &i, const std::string &option)
{
	if (i+1 == argc)
		throw std::runtime_error("option '" + option + "' requires an argument");
	return argv[++i];
}

void rejectValue(const std::string &option)
{
	throw std::runtime_error("option '" + option + "' does not take an argument");
}

bool isFlagShortOption(char c)
{
	return c == '?' || c == 'v' || c == 'q';
}

bool isValueShortOption(char c)
{
	return c == 'h' || c == 'p' || c == 'c'
	    || c == 'b' || c == 's' || c == 'S';
}

void setFlagShortOption(CommandLineOptions &options, char c)
{
	switch (c)
	{
		case '?': options.help = true; break;
		case 'v': options.version = true; break;
		case 'q': options.quiet = true; break;
		default: break;
	}
}

bool looksLikeOption(const std::string &s)
{
	return s.length() > 1 && s[0] == '-';
}

void parseShortOption(
	CommandLineOptions &options, int argc, char **argv, int &i)
{
	std::string arg = argv[i];
	bool all_flags = true;
	for (size_t j = 1; j < arg.length(); ++j)
	{
		if (!isFlagShortOption(arg[j]))
		{
			all_flags = false;
			break;
		}
	}
	if (all_flags)
	{
		for (size_t j = 1; j < arg.length(); ++j)
			setFlagShortOption(options, arg[j]);
		return;
	}

	char c = arg[1];
	if (!isValueShortOption(c))
		throw std::runtime_error("unrecognized option '-" + std::string(1, c) + "'");

	std::string option = "-" + std::string(1, c);
	std::string value = arg.length() > 2
	                  ? arg.substr(2)
	                  : requireValue(argc, argv, i, option);

	switch (c)
	{
		case 'h':
			options.host = value;
			options.host_provided = true;
			break;
		case 'p':
			options.port = parseOptionInt(value, option);
			options.port_provided = true;
			break;
		case 'c':
			options.config_paths.push_back(value);
			break;
		case 'b':
			options.bindings_paths.push_back(value);
			break;
		case 's':
			options.screen = true;
			options.screen_name = value;
			break;
		case 'S':
			options.slave_screen = true;
			options.slave_screen_name = value;
			break;
		default:
			break;
	}
}

std::string longOptionName(const std::string &arg, size_t equals)
{
	if (equals == std::string::npos)
		return arg.substr(2);
	else
		return arg.substr(2, equals-2);
}

std::string longOptionValue(
	int argc, char **argv, int &i, const std::string &arg,
	size_t equals, const std::string &option)
{
	if (equals != std::string::npos)
		return arg.substr(equals+1);
	return requireValue(argc, argv, i, option);
}

void parseLongOption(
	CommandLineOptions &options, int argc, char **argv, int &i)
{
	std::string arg = argv[i];
	size_t equals = arg.find('=');
	std::string name = longOptionName(arg, equals);
	std::string option = "--" + name;

	if (name == "host")
	{
		options.host = longOptionValue(argc, argv, i, arg, equals, option);
		options.host_provided = true;
	}
	else if (name == "port")
	{
		auto value = longOptionValue(argc, argv, i, arg, equals, option);
		options.port = parseOptionInt(value, option);
		options.port_provided = true;
	}
	else if (name == "current-song")
	{
		options.current_song = true;
		if (equals != std::string::npos)
			options.current_song_format = arg.substr(equals+1);
		else if (i+1 < argc && !looksLikeOption(argv[i+1]))
			options.current_song_format = argv[++i];
		else
			options.current_song_format = current_song_default_format;
	}
	else if (name == "config")
	{
		options.config_paths.push_back(
			longOptionValue(argc, argv, i, arg, equals, option));
	}
	else if (name == "ignore-config-errors")
	{
		if (equals != std::string::npos)
			rejectValue(option);
		options.ignore_config_errors = true;
	}
	else if (name == "test-lyrics-fetchers")
	{
		if (equals != std::string::npos)
			rejectValue(option);
		options.test_lyrics_fetchers = true;
	}
	else if (name == "bindings")
	{
		options.bindings_paths.push_back(
			longOptionValue(argc, argv, i, arg, equals, option));
	}
	else if (name == "screen")
	{
		options.screen = true;
		options.screen_name = longOptionValue(argc, argv, i, arg, equals, option);
	}
	else if (name == "slave-screen")
	{
		options.slave_screen = true;
		options.slave_screen_name = longOptionValue(argc, argv, i, arg, equals, option);
	}
	else if (name == "help")
	{
		if (equals != std::string::npos)
			rejectValue(option);
		options.help = true;
	}
	else if (name == "version")
	{
		if (equals != std::string::npos)
			rejectValue(option);
		options.version = true;
	}
	else if (name == "quiet")
	{
		if (equals != std::string::npos)
			rejectValue(option);
		options.quiet = true;
	}
	else
	{
		throw std::runtime_error("unrecognized option '" + option + "'");
	}
}

CommandLineOptions parseCommandLine(
	int argc, char **argv,
	const std::vector<std::string> &default_config_paths,
	const std::vector<std::string> &default_bindings_paths)
{
	CommandLineOptions options;
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "--")
		{
			if (i+1 < argc)
				throw std::runtime_error(
					"unexpected positional argument '" + std::string(argv[i+1]) + "'");
			break;
		}
		else if (arg.rfind("--", 0) == 0)
		{
			parseLongOption(options, argc, argv, i);
		}
		else if (arg.length() > 1 && arg[0] == '-')
		{
			parseShortOption(options, argc, argv, i);
		}
		else
		{
			throw std::runtime_error("unexpected positional argument '" + arg + "'");
		}
	}

	if (options.config_paths.empty())
		options.config_paths = default_config_paths;
	if (options.bindings_paths.empty())
		options.bindings_paths = default_bindings_paths;

	return options;
}

void printUsage(
	const char *program,
	const std::vector<std::string> &default_config_paths,
	const std::vector<std::string> &default_bindings_paths)
{
	cout << "Usage: " << program << " [options]...\n"
	     << "Options:\n"
	     << "  -h, --host HOST              connect to server at host\n"
	     << "  -p, --port PORT              connect to server at port\n"
	     << "      --current-song[=FORMAT]  print current song using given format and exit\n"
	     << "  -c, --config PATH            specify configuration file(s)\n"
	     << "                               default: "
	     << join<std::string>(default_config_paths, " AND ") << "\n"
	     << "      --ignore-config-errors   ignore unknown and invalid options in configuration files\n"
	     << "      --test-lyrics-fetchers   check if lyrics fetchers work\n"
	     << "  -b, --bindings PATH          specify bindings file(s)\n"
	     << "                               default: "
	     << join<std::string>(default_bindings_paths, " AND ") << "\n"
	     << "  -s, --screen SCREEN          specify the startup screen\n"
	     << "  -S, --slave-screen SCREEN    specify the startup slave screen\n"
	     << "  -?, --help                   show help message\n"
	     << "  -v, --version                display version information\n"
	     << "  -q, --quiet                  suppress logs and excess output\n"
	     << "\n";
}

std::string xdg_config_home()
{
	std::string result;
	const char *env_xdg_config_home = getenv("XDG_CONFIG_HOME");
	if (env_xdg_config_home == nullptr)
		result = "~/.config/";
	else
	{
		result = env_xdg_config_home;
		if (!result.empty() && result.back() != '/')
			result += "/";
	}
	return result;
}

}

void expand_home(std::string &path)
{
	assert(env_home != nullptr);
	if (!path.empty())
	{
		size_t i = path.find("~");
		if (i != std::string::npos && (i == 0 || path[i - 1] == '@'))
			path.replace(i, 1, env_home);
	}
}

bool configure(int argc, char **argv)
{
	const std::vector<std::string> default_config_paths = {
		xdg_config_home() + "ncmpcpp/config",
		"~/.ncmpcpp/config"
	};

	const std::vector<std::string> default_bindings_paths = {
		xdg_config_home() + "ncmpcpp/bindings",
		"~/.ncmpcpp/bindings"
	};

	try
	{
		auto options = parseCommandLine(
			argc, argv, default_config_paths, default_bindings_paths);

		// suppress messages from std::clog
		if (options.quiet)
			std::clog.rdbuf(nullptr);

		if (options.help)
		{
			printUsage(argv[0], default_config_paths, default_bindings_paths);
			return false;
		}
		if (options.version)
		{
			std::cout << "ncmpcpp " << VERSION << "\n\n"
			<< "optional screens compiled-in:\n"
	#		ifdef HAVE_TAGLIB_H
			<< " - tag editor\n"
			<< " - tiny tag editor\n"
	#		endif
	#		ifdef ENABLE_OUTPUTS
			<< " - outputs\n"
	#		endif
	#		ifdef ENABLE_VISUALIZER
			<< " - visualizer\n"
	#		endif
			<< "\nencoding: UTF-8"
			<< "\nbuilt with support for:"
	#		ifdef HAVE_FFTW3_H
			<< " fftw"
	#		endif
			<< " ncurses"
	#		ifdef HAVE_TAGLIB_H
			<< " taglib"
	#		endif
			<< "\n";
			return false;
		}

		if (options.test_lyrics_fetchers)
		{
			std::vector<std::tuple<std::string, std::string, std::string>> fetcher_data = {
				std::make_tuple("justsomelyrics", "rihanna", "umbrella"),
				std::make_tuple("jahlyrics", "sean kingston", "dry your eyes"),
				std::make_tuple("plyrics", "rihanna", "umbrella"),
				std::make_tuple("tekstowo", "rihanna", "umbrella"),
				std::make_tuple("zeneszoveg", "rihanna", "umbrella"),
			};
			for (auto &data : fetcher_data)
			{
				NcmLyricsFetcherDef fetcher;
				NcmLyricsResult result;
				std::string name = std::get<0>(data);

				ncm_lyrics_fetcher_def_init(&fetcher);
				ncm_lyrics_result_init(&result);
				if (!ncm_lyrics_fetcher_def_set_name(
				        &fetcher, name.data(), static_cast<int32>(name.size())))
					throw std::runtime_error("Unknown lyrics fetcher: " + name);
				std::cout << std::setw(20)
				          << std::left
				          << ncm_lyrics_fetcher_name(&fetcher)
				          << " : "
				          << std::flush;
				ncm_lyrics_fetcher_fetch(
					&fetcher, &result,
					std::get<1>(data).data(),
					static_cast<int32>(std::get<1>(data).size()),
					std::get<2>(data).data(),
					static_cast<int32>(std::get<2>(data).size()),
					nullptr);
				std::cout << (result.success ? "ok" : "failed")
				          << "\n";
				ncm_lyrics_result_destroy(&result);
				ncm_lyrics_fetcher_def_destroy(&fetcher);
			}
			exit(0);
		}

		// get home directory
		env_home = getenv("HOME");
		if (env_home == nullptr)
		{
			cerr << "Fatal error: HOME environment variable is not defined\n";
			return false;
		}

		// read configuration
		std::for_each(options.config_paths.begin(), options.config_paths.end(), expand_home);
		readConfigurationPaths(
			options.config_paths, options.ignore_config_errors, options.quiet);

		// read bindings
		std::for_each(options.bindings_paths.begin(), options.bindings_paths.end(), expand_home);
		if (readBindingPaths(options.bindings_paths) == false)
			exit(1);
		ncm_bindings_configuration_generate_defaults(&Bindings);

		// create directories
		std::filesystem::create_directories(ConfigLegacy.ncmpcpp_directory);
		std::filesystem::create_directory(ConfigLegacy.lyrics_directory);

		// MPD connection options and --current-song are applied by the C
		// configuration path after this legacy pass completes.

		// custom startup screen
		if (options.screen)
		{
			ConfigLegacy.startup_screen_type = stringtoStartupScreenType(options.screen_name);
			if (ConfigLegacy.startup_screen_type == NCM_SCREEN_TYPE_UNKNOWN)
			{
				std::cerr << "Unknown screen: " << options.screen_name << "\n";
				exit(1);
			}
		}

		// custom startup slave screen
		if (options.slave_screen)
		{
			ConfigLegacy.startup_slave_screen_type = stringtoStartupScreenType(options.slave_screen_name);
			if (ConfigLegacy.startup_slave_screen_type == NCM_SCREEN_TYPE_UNKNOWN)
			{
				std::cerr << "Unknown slave screen: " << options.slave_screen_name << "\n";
				exit(1);
			}
		}
	}
	catch (std::exception &e)
	{
		cerr << "Error while processing configuration: " << e.what() << "\n";
		exit(1);
	}
	return true;
}
