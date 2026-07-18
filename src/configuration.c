#if !defined(NCMPCPP_CONFIGURATION_C)
#define NCMPCPP_CONFIGURATION_C

#include "configuration.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_base.h"
#include "c/ncm_conversion.h"
#include "c/ncm_format.h"
#include "c/ncm_fs.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_path.h"
#include "c/ncm_song.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "config.h"
#include "global.h"
#include "bindings.h"
#include "settings.h"
#include "lyrics_fetcher.h"
#include "screens/screen_type.h"

#if !defined(VERSION)
#define VERSION "unknown"
#endif

static char current_song_default_format[] = "{{{(%l) }{{%a - }%t}}|{%f}}";
static bool configuration_quiet;

static void configuration_print_error(char *context, NcmError *error);

void
ncm_configuration_options_init(NcmConfigurationOptions *options) {
    ncm_buffer_init(&options->host);
    ncm_buffer_init(&options->current_song_format);
    ncm_buffer_init(&options->screen_name);
    ncm_buffer_init(&options->slave_screen_name);
    ncm_buffer_array_init(&options->config_paths);
    ncm_buffer_array_init(&options->bindings_paths);

    ncm_buffer_append(&options->host, STRLIT_ARGS("localhost"));
    ncm_buffer_append(&options->current_song_format,
                      current_song_default_format,
                      strlen32(current_song_default_format));
    options->port = 6600;

    options->host_provided = false;
    options->port_provided = false;
    options->current_song = false;
    options->ignore_config_errors = false;
    options->test_lyrics_fetchers = false;
    options->screen = false;
    options->slave_screen = false;
    options->help = false;
    options->version = false;
    options->quiet = false;
    return;
}

void
ncm_configuration_options_destroy(NcmConfigurationOptions *options) {
    ncm_buffer_destroy(&options->host);
    ncm_buffer_destroy(&options->current_song_format);
    ncm_buffer_destroy(&options->screen_name);
    ncm_buffer_destroy(&options->slave_screen_name);
    ncm_buffer_array_destroy(&options->config_paths);
    ncm_buffer_array_destroy(&options->bindings_paths);
    return;
}

static bool
command_line_options_append_path(NcmBufferArray *paths, char *path,
                                 int32 path_len) {
    NcmBuffer *slot;

    slot = ncm_buffer_array_append(paths);
    if (slot == NULL) {
        return false;
    }
    ncm_buffer_append(slot, path, path_len);
    return true;
}

static bool
configuration_append_buffer_path(NcmBufferArray *paths, NcmBuffer *path) {
    return command_line_options_append_path(paths, path->data, path->len);
}

static bool
configuration_append_default_file(NcmBufferArray *paths, char *filename,
                                  int32 filename_len) {
    char *xdg_config_home;
    NcmBuffer directory;
    NcmBuffer path;
    bool result;

    ncm_buffer_init(&directory);
    ncm_buffer_init(&path);

    xdg_config_home = getenv("XDG_CONFIG_HOME");
    if ((xdg_config_home != NULL) && (xdg_config_home[0] != '\0')) {
        ncm_buffer_append(&directory, xdg_config_home,
                          strlen32(xdg_config_home));
    } else {
        ncm_buffer_append(&directory, STRLIT_ARGS("~/.config"));
    }
    result = ncm_fs_join(&directory, directory.data, directory.len,
                         STRLIT_ARGS("ncmpcpp"));
    if (result) {
        result = ncm_fs_join(&path, directory.data, directory.len, filename,
                             filename_len);
    }
    if (result) {
        result = configuration_append_buffer_path(paths, &path);
    }

    ncm_buffer_destroy(&path);
    ncm_buffer_destroy(&directory);
    return result;
}

static bool
configuration_append_legacy_file(NcmBufferArray *paths, char *filename,
                                 int32 filename_len) {
    NcmBuffer directory;
    NcmBuffer path;
    bool result;

    ncm_buffer_init(&directory);
    ncm_buffer_init(&path);

    ncm_buffer_append(&directory, STRLIT_ARGS("~/.ncmpcpp"));
    result = ncm_fs_join(&path, directory.data, directory.len, filename,
                         filename_len);
    if (result) {
        result = configuration_append_buffer_path(paths, &path);
    }

    ncm_buffer_destroy(&path);
    ncm_buffer_destroy(&directory);
    return result;
}

bool
configuration_discover_default_paths(NcmBufferArray *config_paths,
                                     NcmBufferArray *bindings_paths,
                                     NcmError *error) {
    bool result;

    if ((config_paths == NULL) || (bindings_paths == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing default path output"));
        return false;
    }

    result = configuration_append_default_file(config_paths,
                                               STRLIT_ARGS("config"));
    result = result
             && configuration_append_legacy_file(config_paths,
                                                 STRLIT_ARGS("config"));
    result = result
             && configuration_append_default_file(bindings_paths,
                                                  STRLIT_ARGS("bindings"));
    result = result
             && configuration_append_legacy_file(bindings_paths,
                                                 STRLIT_ARGS("bindings"));
    if (!result) {
        ncm_error_set(error, ENOMEM,
                      STRLIT_ARGS("failed to build default paths"));
    }
    return result;
}

static bool
configuration_copy_string(NcmBuffer *buffer, char *string, int32 string_len) {
    ncm_buffer_clear(buffer);
    ncm_buffer_append(buffer, string, string_len);
    return true;
}

static bool
configuration_require_value(int32 argc, char **argv, int32 *i, char *option,
                            int32 option_len, char **value, int32 *value_len,
                            NcmError *error) {
    if (*i + 1 >= argc) {
        char message[128];
        int32 len;

        len = SNPRINTF(message, "option '%.*s' requires an argument",
                       option_len, option);
        ncm_error_set(error, EINVAL, message, len);
        return false;
    }

    *i += 1;
    *value = argv[*i];
    *value_len = strlen32(*value);
    return true;
}

static bool
configuration_parse_port(char *value, int32 value_len, char *option,
                         int32 option_len, uint32 *port, NcmError *error) {
    uint32 parsed;

    if (!ncm_parse_uint32(value, value_len, &parsed, error)) {
        char message[192];
        int32 len;

        len = SNPRINTF(message,
                       "the argument ('%.*s') for option '%.*s' is invalid",
                       value_len, value, option_len, option);
        ncm_error_set(error, EINVAL, message, len);
        return false;
    }
    if (parsed > 65535) {
        ncm_error_set(error, ERANGE,
                      STRLIT_ARGS("port must be between 0 and 65535"));
        return false;
    }

    *port = parsed;
    return true;
}

static bool
configuration_is_flag_short_option(char c) {
    return (c == '?') || (c == 'v') || (c == 'q');
}

static bool
configuration_is_value_short_option(char c) {
    return (c == 'h') || (c == 'p') || (c == 'c') || (c == 'b') || (c == 's')
           || (c == 'S');
}

static bool
configuration_set_short_flag(NcmConfigurationOptions *options, char c) {
    switch (c) {
    case '?':
        options->help = true;
        return true;
    case 'v':
        options->version = true;
        return true;
    case 'q':
        options->quiet = true;
        return true;
    default:
        break;
    }

    return false;
}

static bool
configuration_looks_like_option(char *arg, int32 arg_len) {
    return (arg_len > 1) && (arg[0] == '-');
}

static bool
configuration_parse_short_option(NcmConfigurationOptions *options, int32 argc,
                                 char **argv, int32 *i, NcmError *error) {
    char *arg;
    int32 arg_len;
    bool all_flags;
    char option[3];
    char c;
    char *value;
    int32 value_len;

    arg = argv[*i];
    arg_len = strlen32(arg);
    all_flags = true;
    for (int32 j = 1; j < arg_len; j += 1) {
        if (!configuration_is_flag_short_option(arg[j])) {
            all_flags = false;
            break;
        }
    }
    if (all_flags) {
        for (int32 j = 1; j < arg_len; j += 1) {
            configuration_set_short_flag(options, arg[j]);
        }
        return true;
    }

    c = arg[1];
    if (!configuration_is_value_short_option(c)) {
        char message[128];
        int32 len;

        len = SNPRINTF(message, "unrecognized option '-%c'", c);
        ncm_error_set(error, EINVAL, message, len);
        return false;
    }

    option[0] = '-';
    option[1] = c;
    option[2] = '\0';
    if (arg_len > 2) {
        value = arg + 2;
        value_len = arg_len - 2;
    } else if (!configuration_require_value(argc, argv, i, option, 2, &value,
                                            &value_len, error)) {
        return false;
    }

    switch (c) {
    case 'h':
        configuration_copy_string(&options->host, value, value_len);
        options->host_provided = true;
        break;
    case 'p':
        if (!configuration_parse_port(value, value_len, option, 2,
                                      &options->port, error)) {
            return false;
        }
        options->port_provided = true;
        break;
    case 'c':
        command_line_options_append_path(&options->config_paths, value,
                                         value_len);
        break;
    case 'b':
        command_line_options_append_path(&options->bindings_paths, value,
                                         value_len);
        break;
    case 's':
        options->screen = true;
        configuration_copy_string(&options->screen_name, value, value_len);
        break;
    case 'S':
        options->slave_screen = true;
        configuration_copy_string(&options->slave_screen_name, value,
                                  value_len);
        break;
    default:
        break;
    }
    return true;
}

static bool
configuration_parse_long_option(NcmConfigurationOptions *options, int32 argc,
                                char **argv, int32 *i, NcmError *error) {
    char *arg;
    char *name;
    char *value;
    int32 arg_len;
    int32 name_len;
    int32 value_len;
    int32 equals;

    arg = argv[*i];
    arg_len = strlen32(arg);
    equals = ncm_string_find_char(arg, arg_len, '=');
    name = arg + 2;
    if (equals >= 0) {
        name_len = equals - 2;
        value = arg + equals + 1;
        value_len = arg_len - equals - 1;
    } else {
        name_len = arg_len - 2;
        value = NULL;
        value_len = 0;
    }

#define REQUIRE_LONG_VALUE() \
    do { \
        if (value == NULL) { \
            if (!configuration_require_value(argc, argv, i, arg, \
                                             name_len + 2, &value, \
                                             &value_len, error)) { \
                return false; \
            } \
        } \
    } while (0)

#define REJECT_LONG_VALUE() \
    do { \
        if (value != NULL) { \
            char message[128]; \
            int32 len; \
            len = SNPRINTF(message, \
                           "option '--%.*s' does not take an argument", \
                           name_len, name); \
            ncm_error_set(error, EINVAL, message, len); \
            return false; \
        } \
    } while (0)

    if (STREQUAL(name, name_len, STRLIT_ARGS("host"))) {
        REQUIRE_LONG_VALUE();
        configuration_copy_string(&options->host, value, value_len);
        options->host_provided = true;
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("port"))) {
        REQUIRE_LONG_VALUE();
        if (!configuration_parse_port(value, value_len, arg, name_len + 2,
                                      &options->port, error)) {
            return false;
        }
        options->port_provided = true;
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("current-song"))) {
        options->current_song = true;
        if (value != NULL) {
            configuration_copy_string(&options->current_song_format, value,
                                      value_len);
        } else if ((*i + 1 < argc)
                   && !configuration_looks_like_option(
                       argv[*i + 1], strlen32(argv[*i + 1]))) {
            *i += 1;
            configuration_copy_string(&options->current_song_format, argv[*i],
                                      strlen32(argv[*i]));
        }
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("config"))) {
        REQUIRE_LONG_VALUE();
        command_line_options_append_path(&options->config_paths, value,
                                         value_len);
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("ignore-config-errors"))) {
        REJECT_LONG_VALUE();
        options->ignore_config_errors = true;
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("test-lyrics-fetchers"))) {
        REJECT_LONG_VALUE();
        options->test_lyrics_fetchers = true;
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("bindings"))) {
        REQUIRE_LONG_VALUE();
        command_line_options_append_path(&options->bindings_paths, value,
                                         value_len);
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("screen"))) {
        REQUIRE_LONG_VALUE();
        options->screen = true;
        configuration_copy_string(&options->screen_name, value, value_len);
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("slave-screen"))) {
        REQUIRE_LONG_VALUE();
        options->slave_screen = true;
        configuration_copy_string(&options->slave_screen_name, value,
                                  value_len);
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("help"))) {
        REJECT_LONG_VALUE();
        options->help = true;
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("version"))) {
        REJECT_LONG_VALUE();
        options->version = true;
    } else if (STREQUAL(name, name_len, STRLIT_ARGS("quiet"))) {
        REJECT_LONG_VALUE();
        options->quiet = true;
    } else {
        char message[128];
        int32 len;

        len = SNPRINTF(message, "unrecognized option '--%.*s'", name_len, name);
        ncm_error_set(error, EINVAL, message, len);
        return false;
    }

#undef REQUIRE_LONG_VALUE
#undef REJECT_LONG_VALUE

    return true;
}

bool
ncm_configuration_options_parse(NcmConfigurationOptions *options, int32 argc,
                                char **argv, NcmError *error) {
    for (int32 i = 1; i < argc; i += 1) {
        char *arg;
        int32 arg_len;

        arg = argv[i];
        arg_len = strlen32(arg);
        if (STREQUAL(arg, arg_len, STRLIT_ARGS("--"))) {
            if (i + 1 < argc) {
                char message[192];
                int32 len;

                len = SNPRINTF(message, "unexpected positional argument '%s'",
                               argv[i + 1]);
                ncm_error_set(error, EINVAL, message, len);
                return false;
            }
            break;
        }
        if (ncm_string_starts_with(arg, arg_len, STRLIT_ARGS("--"))) {
            if (!configuration_parse_long_option(options, argc, argv, &i,
                                                 error)) {
                return false;
            }
        } else if ((arg_len > 1) && (arg[0] == '-')) {
            if (!configuration_parse_short_option(options, argc, argv, &i,
                                                  error)) {
                return false;
            }
        } else {
            char message[192];
            int32 len;

            len = SNPRINTF(message, "unexpected positional argument '%.*s'",
                           arg_len, arg);
            ncm_error_set(error, EINVAL, message, len);
            return false;
        }
    }

    if ((options->config_paths.len == 0)
        || (options->bindings_paths.len == 0)) {
        NcmBufferArray default_config_paths;
        NcmBufferArray default_bindings_paths;

        ncm_buffer_array_init(&default_config_paths);
        ncm_buffer_array_init(&default_bindings_paths);
        if (!configuration_discover_default_paths(
                &default_config_paths, &default_bindings_paths, error)) {
            ncm_buffer_array_destroy(&default_config_paths);
            ncm_buffer_array_destroy(&default_bindings_paths);
            return false;
        }
        if (options->config_paths.len == 0) {
            for (int32 j = 0; j < default_config_paths.len; j += 1) {
                NcmBuffer *path;

                path = &default_config_paths.items[j];
                command_line_options_append_path(&options->config_paths,
                                                 path->data, path->len);
            }
        }
        if (options->bindings_paths.len == 0) {
            for (int32 j = 0; j < default_bindings_paths.len; j += 1) {
                NcmBuffer *path;

                path = &default_bindings_paths.items[j];
                command_line_options_append_path(&options->bindings_paths,
                                                 path->data, path->len);
            }
        }
        ncm_buffer_array_destroy(&default_config_paths);
        ncm_buffer_array_destroy(&default_bindings_paths);
    }
    return true;
}

static void
configuration_print_usage(char *program) {
    NcmBufferArray config_paths;
    NcmBufferArray bindings_paths;
    NcmError error;

    ncm_buffer_array_init(&config_paths);
    ncm_buffer_array_init(&bindings_paths);
    ncm_error_clear(&error);
    if (!configuration_discover_default_paths(&config_paths, &bindings_paths,
                                              &error)) {
        configuration_print_error("Failed to build default paths", &error);
        ncm_buffer_array_destroy(&bindings_paths);
        ncm_buffer_array_destroy(&config_paths);
        return;
    }

    printf("Usage: %s [options]...\n", program);
    printf("Options:\n");
    printf("  -h, --host HOST              connect to server at host\n");
    printf("  -p, --port PORT              connect to server at port\n");
    printf("      --current-song[=FORMAT]  print current song using given ");
    printf("format and exit\n");
    printf("  -c, --config PATH            specify configuration file(s)\n");
    printf("                               default: ");
    for (int32 i = 0; i < config_paths.len; i += 1) {
        NcmBuffer *path;

        path = &config_paths.items[i];
        if (i > 0) {
            printf(" AND ");
        }
        printf("%.*s", path->len, path->data);
    }
    printf("\n");
    printf("      --ignore-config-errors   ignore unknown and invalid ");
    printf("options in configuration files\n");
    printf("      --test-lyrics-fetchers   check if lyrics fetchers work\n");
    printf("  -b, --bindings PATH          specify bindings file(s)\n");
    printf("                               default: ");
    for (int32 i = 0; i < bindings_paths.len; i += 1) {
        NcmBuffer *path;

        path = &bindings_paths.items[i];
        if (i > 0) {
            printf(" AND ");
        }
        printf("%.*s", path->len, path->data);
    }
    printf("\n");
    printf("  -s, --screen SCREEN          specify the startup screen\n");
    printf("  -S, --slave-screen SCREEN    specify startup slave screen\n");
    printf("  -?, --help                   show help message\n");
    printf("  -v, --version                display version information\n");
    printf("  -q, --quiet                  suppress logs and excess output\n");
    printf("\n");

    ncm_buffer_array_destroy(&bindings_paths);
    ncm_buffer_array_destroy(&config_paths);
    return;
}

static void
configuration_print_version(void) {
    printf("ncmpcpp %s\n\n", VERSION);
    printf("optional screens compiled-in:\n");
#if defined(HAVE_TAGLIB_H)
    printf(" - tag editor\n");
    printf(" - tiny tag editor\n");
#endif
#if defined(ENABLE_OUTPUTS)
    printf(" - outputs\n");
#endif
#if defined(ENABLE_VISUALIZER)
    printf(" - visualizer\n");
#endif
    printf("\nencoding: UTF-8\n");
    printf("built with support for:");
#if defined(HAVE_FFTW3_H)
    printf(" fftw");
#endif
    printf(" ncurses");
#if defined(HAVE_TAGLIB_H)
    printf(" taglib");
#endif
    printf("\n");
    return;
}

static bool
configuration_make_string_views(NcmStringViewArray *views,
                                NcmBufferArray *buffers) {
    ncm_string_view_array_init(views);
    for (int32 i = 0; i < buffers->len; i += 1) {
        NcmStringView *view;
        NcmBuffer *buffer;

        buffer = &buffers->items[i];
        view = ncm_string_view_array_append(views);
        if (view == NULL) {
            ncm_string_view_array_destroy(views);
            return false;
        }
        view->data = buffer->data;
        view->len = buffer->len;
    }
    return true;
}

static bool
configuration_read_settings(NcmConfigurationOptions *options, NcmError *error) {
    NcmStringViewArray config_views;
    bool result;

    for (int32 i = 0; i < options->config_paths.len; i += 1) {
        if (!ncm_path_expand_home(&options->config_paths.items[i], error)) {
            return false;
        }
    }
    if (!configuration_make_string_views(&config_views,
                                         &options->config_paths)) {
        ncm_error_set(error, ENOMEM,
                      STRLIT_ARGS("failed to build config path views"));
        return false;
    }

    configuration_clear(&Config);
    result = configuration_read(&Config, &config_views,
                                options->ignore_config_errors, options->quiet,
                                error);
    ncm_string_view_array_destroy(&config_views);
    if (!result) {
        if (!ncm_error_is_set(error)) {
            ncm_error_set(error, EINVAL,
                          STRLIT_ARGS("failed to read configuration"));
        }
        return false;
    }
    return true;
}

static bool
configuration_read_bindings(NcmConfigurationOptions *options, NcmError *error) {
    ncm_bindings_configuration_clear(&Bindings);
    for (int32 i = 0; i < options->bindings_paths.len; i += 1) {
        NcmBuffer *path;

        path = &options->bindings_paths.items[i];
        if (!ncm_path_expand_home(path, error)) {
            return false;
        }
        if (!ncm_bindings_configuration_read(&Bindings, path->data, path->len,
                                             error)) {
            return false;
        }
    }
    ncm_bindings_configuration_generate_defaults(&Bindings);
    return true;
}

static bool
configuration_create_directories(NcmError *error) {
    if (!ncm_fs_exists(Config.ncmpcpp_directory, Config.ncmpcpp_directory_len)
        && !ncm_fs_mkdir_all(Config.ncmpcpp_directory,
                             Config.ncmpcpp_directory_len, error)) {
        return false;
    }
    if (!ncm_fs_exists(Config.lyrics_directory, Config.lyrics_directory_len)
        && !ncm_fs_mkdir_all(Config.lyrics_directory,
                             Config.lyrics_directory_len, error)) {
        return false;
    }
    return true;
}

static bool
configuration_apply_mpd_environment(NcmError *error) {
    char *env_host;
    char *env_port;
    uint32 port;

    env_host = getenv("MPD_HOST");
    env_port = getenv("MPD_PORT");
    if (env_host != NULL) {
        if (!ncm_mpd_client_set_hostname(&global_mpd, env_host,
                                         strlen32(env_host), error)) {
            return false;
        }
    }
    if (env_port != NULL) {
        if (!ncm_parse_uint32(env_port, strlen32(env_port), &port, error)) {
            return false;
        }
        if (port > 65535) {
            ncm_error_set(error, ERANGE,
                          STRLIT_ARGS("MPD_PORT is out of range"));
            return false;
        }
        ncm_mpd_client_set_port(&global_mpd, (uint16)port);
    }
    return true;
}

static bool
configuration_apply_mpd_command_line(NcmConfigurationOptions *options,
                                     NcmError *error) {
    if (options->host_provided) {
        if (!ncm_mpd_client_set_hostname(&global_mpd, options->host.data,
                                         options->host.len, error)) {
            return false;
        }
    }
    if (options->port_provided) {
        ncm_mpd_client_set_port(&global_mpd, (uint16)options->port);
    }
    if (!ncm_mpd_client_set_timeout_ms(
            &global_mpd, Config.mpd_connection_timeout * 1000, error)) {
        return false;
    }
    return true;
}

static bool
configuration_apply_screen_options(NcmConfigurationOptions *options,
                                   NcmError *error) {
    if (options->screen) {
        if (!screen_type_parse_startup(options->screen_name.data,
                                       options->screen_name.len,
                                       &Config.startup_screen_type)) {
            ncm_error_set(error, EINVAL, STRLIT_ARGS("unknown screen"));
            return false;
        }
    }
    if (options->slave_screen) {
        if (!screen_type_parse_startup(options->slave_screen_name.data,
                                       options->slave_screen_name.len,
                                       &Config.startup_slave_screen_type)) {
            ncm_error_set(error, EINVAL, STRLIT_ARGS("unknown slave screen"));
            return false;
        }
        Config.has_startup_slave_screen_type = true;
    }
    return true;
}

bool
ncm_configuration_options_apply(NcmConfigurationOptions *options,
                                NcmError *error) {
    return configuration_read_settings(options, error)
           && configuration_create_directories(error)
           && configuration_apply_mpd_environment(error)
           && configuration_apply_mpd_command_line(options, error)
           && configuration_apply_screen_options(options, error);
}

static bool
configuration_print_current_song(NcmConfigurationOptions *options,
                                 NcmError *error) {
    NcmSong song;
    NcmFormatAst format;
    NcmBuffer output;
    bool result;

    ncm_song_init(&song);
    ncm_format_ast_init(&format);
    ncm_buffer_init(&output);

    result = ncm_mpd_client_connect(&global_mpd, error)
             && ncm_mpd_client_get_current_song(&global_mpd, &song, error);
    if (result && !ncm_song_empty(&song)) {
        result = ncm_format_parse(&format, options->current_song_format.data,
                                  options->current_song_format.len,
                                  NCM_FORMAT_FLAG_TAG, error);
        if (result) {
            output = ncm_format_render_string(&format, &song);
            if (output.len > 0) {
                fwrite(output.data, 1, (size_t)output.len, stdout);
            }
        }
    }

    ncm_buffer_destroy(&output);
    ncm_format_ast_destroy(&format);
    ncm_song_destroy(&song);
    ncm_mpd_client_disconnect(&global_mpd);
    return result;
}

typedef struct ConfigurationLyricsFetcherTest {
    char *name;
    char *artist;
    char *title;
    int32 name_len;
    int32 artist_len;
    int32 title_len;
} ConfigurationLyricsFetcherTest;

static bool
configuration_test_lyrics_fetchers(NcmError *error) {
    ConfigurationLyricsFetcherTest tests[] = {
        {
            .name = "justsomelyrics",
            .artist = "rihanna",
            .title = "umbrella",
            .name_len = STRLIT_LEN("justsomelyrics"),
            .artist_len = STRLIT_LEN("rihanna"),
            .title_len = STRLIT_LEN("umbrella"),
        },
        {
            .name = "jahlyrics",
            .artist = "sean kingston",
            .title = "dry your eyes",
            .name_len = STRLIT_LEN("jahlyrics"),
            .artist_len = STRLIT_LEN("sean kingston"),
            .title_len = STRLIT_LEN("dry your eyes"),
        },
        {
            .name = "plyrics",
            .artist = "rihanna",
            .title = "umbrella",
            .name_len = STRLIT_LEN("plyrics"),
            .artist_len = STRLIT_LEN("rihanna"),
            .title_len = STRLIT_LEN("umbrella"),
        },
        {
            .name = "tekstowo",
            .artist = "rihanna",
            .title = "umbrella",
            .name_len = STRLIT_LEN("tekstowo"),
            .artist_len = STRLIT_LEN("rihanna"),
            .title_len = STRLIT_LEN("umbrella"),
        },
        {
            .name = "zeneszoveg",
            .artist = "rihanna",
            .title = "umbrella",
            .name_len = STRLIT_LEN("zeneszoveg"),
            .artist_len = STRLIT_LEN("rihanna"),
            .title_len = STRLIT_LEN("umbrella"),
        },
    };
    bool ok;

    ok = true;
    for (int32 i = 0; i < NCM_ARRAY_LEN(tests); i += 1) {
        NcmLyricsFetcherDef fetcher;
        NcmLyricsResult result;

        ncm_lyrics_fetcher_def_init(&fetcher);
        ncm_lyrics_result_init(&result);
        if (!ncm_lyrics_fetcher_def_set_name(&fetcher, tests[i].name,
                                             tests[i].name_len)) {
            ncm_error_set(error, EINVAL, STRLIT_ARGS("unknown lyrics fetcher"));
            ok = false;
        }
        if (ok) {
            printf("%-20.*s : ", ncm_lyrics_fetcher_name_len(&fetcher),
                   ncm_lyrics_fetcher_name(&fetcher));
            fflush(stdout);
            (void)ncm_lyrics_fetcher_fetch(&fetcher, &result, tests[i].artist,
                                           tests[i].artist_len, tests[i].title,
                                           tests[i].title_len, NULL);
            if (result.success) {
                printf("ok\n");
            } else {
                printf("failed\n");
            }
        }
        ncm_lyrics_result_destroy(&result);
        ncm_lyrics_fetcher_def_destroy(&fetcher);
        if (!ok) {
            break;
        }
    }
    return ok;
}

static void
configuration_print_error(char *context, NcmError *error) {
    if ((error != NULL) && (error->message[0] != '\0')) {
        fprintf(stderr, "%s: %s\n", context, error->message);
    } else {
        fprintf(stderr, "%s\n", context);
    }
    return;
}

bool
configure(int32 argc, char **argv) {
    NcmConfigurationOptions options;
    NcmError error;
    bool result;

    configuration_quiet = false;
    ncm_error_clear(&error);
    ncm_configuration_options_init(&options);
    if (!ncm_configuration_options_parse(&options, argc, argv, &error)) {
        configuration_print_error("Error while processing configuration",
                                  &error);
        ncm_configuration_options_destroy(&options);
        exit(EXIT_FAILURE);
    }

    configuration_quiet = options.quiet;

    if (options.help) {
        configuration_print_usage(argv[0]);
        ncm_configuration_options_destroy(&options);
        return false;
    }
    if (options.version) {
        configuration_print_version();
        ncm_configuration_options_destroy(&options);
        return false;
    }
    if (options.test_lyrics_fetchers) {
        result = configuration_test_lyrics_fetchers(&error);
        ncm_configuration_options_destroy(&options);
        if (!result) {
            configuration_print_error("Error while testing lyrics fetchers",
                                      &error);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    result = ncm_configuration_options_apply(&options, &error);
    if (result && !options.current_song) {
        result = configuration_read_bindings(&options, &error);
    }
    if (result && options.current_song) {
        result = configuration_print_current_song(&options, &error);
        ncm_configuration_options_destroy(&options);
        if (!result) {
            configuration_print_error("Error while printing current song",
                                      &error);
            exit(EXIT_FAILURE);
        }
        return false;
    }
    if (!result) {
        configuration_print_error("Error while processing configuration",
                                  &error);
        ncm_configuration_options_destroy(&options);
        exit(EXIT_FAILURE);
    }

    ncm_configuration_options_destroy(&options);
    return true;
}

#endif /* NCMPCPP_CONFIGURATION_C */
