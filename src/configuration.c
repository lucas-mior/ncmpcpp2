#if !defined(NCMPCPP_CONFIGURATION_C)
#define NCMPCPP_CONFIGURATION_C

#include "cbase.h"

#include "bindings.h"
#include "c/ncm_c.h"
#include "config.h"
#include "configuration.h"
#include "global.h"
#include "lyrics_fetcher.h"
#include "screens/nc_screens.h"
#include "settings.h"

#if !defined(VERSION)
#define VERSION "unknown"
#endif

static bool configuration_quiet;

static void configuration_print_error(char *context, NcmError *ncm_error);

void
ncm_configuration_options_init(NcmConfigurationOptions *options) {
    options->host = (StrBuilder){0};
    options->current_song_format = (StrBuilder){0};
    options->screen_name = (StrBuilder){0};
    options->slave_screen_name = (StrBuilder){0};

    str_builder_array_init(&options->config_paths);
    str_builder_array_init(&options->bindings_paths);

    SB_APPEND(&options->host, "localhost");
    SB_APPEND(&options->current_song_format, "{{{(%l) }{{%a - }%t}}|{%f}}");
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
    sb_free(&options->host);
    sb_free(&options->current_song_format);
    sb_free(&options->screen_name);
    sb_free(&options->slave_screen_name);
    str_builder_array_destroy(&options->config_paths);
    str_builder_array_destroy(&options->bindings_paths);
    return;
}

static int32
command_line_options_append_path(StrBuilderArray *paths, char *path,
                                 int32 path_len) {
    StrBuilder *slot;

    if ((slot = str_builder_array_append(paths)) == NULL) {
        return -ENOMEM;
    }
    SB_APPEND(slot, path, path_len);
    return 0;
}

static int32
configuration_append_buffer_path(StrBuilderArray *paths, StrBuilder *path) {
    return command_line_options_append_path(paths, path->data, path->len);
}

static int32
configuration_append_default_file(StrBuilderArray *paths, char *filename,
                                  int32 filename_len) {
    char *xdg_config_home;
    StrBuilder directory = {0};
    StrBuilder path = {0};
    int32 status;

    status = 0;
    if ((xdg_config_home = getenv("XDG_CONFIG_HOME"))
        && (xdg_config_home[0] != '\0')) {
        SB_APPEND(&directory, xdg_config_home, strlen32(xdg_config_home));
    } else {
        SB_APPEND(&directory, "~/.config");
    }
    if ((status = ncm_fs_join(&directory, directory.data, directory.len,
                              STRLIT("ncmpcpp"))) < 0) {
        goto cleanup;
    }
    if ((status = ncm_fs_join(&path, directory.data, directory.len,
                              filename, filename_len)) < 0) {
        goto cleanup;
    }
    status = configuration_append_buffer_path(paths, &path);

cleanup:
    sb_free(&path);
    sb_free(&directory);
    return status;
}

static int32
configuration_append_legacy_file(StrBuilderArray *paths, char *filename,
                                 int32 filename_len) {
    StrBuilder directory = {0};
    StrBuilder path = {0};
    int32 status;

    status = 0;
    SB_APPEND(&directory, "~/.ncmpcpp");
    if ((status = ncm_fs_join(&path, directory.data, directory.len,
                              filename, filename_len)) < 0) {
        goto cleanup;
    }
    status = configuration_append_buffer_path(paths, &path);

cleanup:
    sb_free(&path);
    sb_free(&directory);
    return status;
}

int32
configuration_discover_default_paths(StrBuilderArray *config_paths,
                                     StrBuilderArray *bindings_paths,
                                     NcmError *ncm_error) {
    int32 status;

    if ((config_paths == NULL) || (bindings_paths == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing default path output"));
    }

    if ((status = configuration_append_default_file(config_paths,
                                                    STRLIT("config"))) < 0) {
        goto fail;
    }
    if ((status = configuration_append_legacy_file(config_paths,
                                                   STRLIT("config"))) < 0) {
        goto fail;
    }
    if ((status = configuration_append_default_file(bindings_paths,
                                                    STRLIT("bindings"))) < 0) {
        goto fail;
    }
    if ((status = configuration_append_legacy_file(bindings_paths,
                                                   STRLIT("bindings"))) < 0) {
        goto fail;
    }

    return ncm_error_ok(ncm_error);

fail:
    if (!ncm_error_is_set(ncm_error)) {
        ncm_error_set_status(ncm_error, status,
                             STRLIT("failed to build default paths"));
    }
    return status;
}

static void
configuration_copy_string(StrBuilder *buffer, char *string, int32 string_len) {
    sb_clear(buffer);
    SB_APPEND(buffer, string, string_len);
    return;
}

static int32
configuration_require_value(int32 argc, char **argv, int32 *i, char *option,
                            int32 option_len, char **value, int32 *value_len,
                            NcmError *ncm_error) {
    if (*i + 1 >= argc) {
        char message[128];
        int32 len;

        len = SNPRINTF(message, "option '%.*s' requires an argument",
                       option_len, option);
        return ncm_error_set_status(ncm_error, -EINVAL, message, len);
    }

    *i += 1;
    *value = argv[*i];
    *value_len = strlen32(*value);
    return 0;
}

static int32
configuration_parse_port(char *value, int32 value_len, char *option,
                         int32 option_len, int32 *port, NcmError *ncm_error) {
    int32 parsed;

    if (ncm_parse_int32(value, value_len, &parsed, ncm_error) < 0) {
        char message[192];
        int32 len;

        len = SNPRINTF(message,
                       "the argument ('%.*s') for option '%.*s' is invalid",
                       value_len, value, option_len, option);
        return ncm_error_set_status(ncm_error, -EINVAL, message, len);
    }
    if (parsed > 65535) {
        return ncm_error_set_status(ncm_error, -ERANGE,
                                    STRLIT("port must be between 0 and 65535"));
    }

    *port = parsed;
    return 0;
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

static void
configuration_set_short_flag(NcmConfigurationOptions *options, char c) {
    switch (c) {
    case '?':
        options->help = true;
        break;
    case 'v':
        options->version = true;
        break;
    case 'q':
        options->quiet = true;
        break;
    default:
        break;
    }

    return;
}

static bool
configuration_looks_like_option(char *arg, int32 arg_len) {
    return (arg_len > 1) && (arg[0] == '-');
}

static int32
configuration_parse_short_option(NcmConfigurationOptions *options, int32 argc,
                                 char **argv, int32 *i, NcmError *ncm_error) {
    char *arg;
    int32 arg_len;
    bool all_flags;
    char option[3];
    char c;
    char *value;
    int32 value_len;
    int32 status;

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
        return 0;
    }

    c = arg[1];
    if (!configuration_is_value_short_option(c)) {
        char message[128];
        int32 len;

        len = SNPRINTF(message, "unrecognized option '-%c'", c);
        return ncm_error_set_status(ncm_error, -EINVAL, message, len);
    }

    option[0] = '-';
    option[1] = c;
    option[2] = '\0';
    if (arg_len > 2) {
        value = arg + 2;
        value_len = arg_len - 2;
    } else if ((status = configuration_require_value(argc, argv, i, option, 2,
                                                     &value, &value_len,
                                                     ncm_error)) < 0) {
        return status;
    }

    switch (c) {
    case 'h':
        configuration_copy_string(&options->host, value, value_len);
        options->host_provided = true;
        break;
    case 'p':
        if ((status = configuration_parse_port(value, value_len, option, 2,
                                               &options->port,
                                               ncm_error)) < 0) {
            return status;
        }
        options->port_provided = true;
        break;
    case 'c':
        if ((status = command_line_options_append_path(&options->config_paths,
                                                       value, value_len)) < 0) {
            return status;
        }
        break;
    case 'b':
        if ((status = command_line_options_append_path(&options->bindings_paths,
                                                       value, value_len)) < 0) {
            return status;
        }
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
    return 0;
}

static int32
configuration_parse_long_option(NcmConfigurationOptions *options, int32 argc,
                                char **argv, int32 *i, NcmError *ncm_error) {
    char *arg;
    char *name;
    char *value;
    int32 arg_len;
    int32 name_len;
    int32 value_len;
    int32 equals;
    int32 status;

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
            status = configuration_require_value(argc, argv, i, arg, \
                                                 name_len + 2, &value, \
                                                 &value_len, ncm_error); \
            if (status < 0) { \
                return status; \
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
            return ncm_error_set_status(ncm_error, -EINVAL, message, len); \
        } \
    } while (0)

    if (STREQUAL(name, name_len, "host")) {
        REQUIRE_LONG_VALUE();
        configuration_copy_string(&options->host, value, value_len);
        options->host_provided = true;
    } else if (STREQUAL(name, name_len, "port")) {
        REQUIRE_LONG_VALUE();
        if ((status = configuration_parse_port(value, value_len, arg,
                                               name_len + 2, &options->port,
                                               ncm_error)) < 0) {
            return status;
        }
        options->port_provided = true;
    } else if (STREQUAL(name, name_len, "current-song")) {
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
    } else if (STREQUAL(name, name_len, "config")) {
        REQUIRE_LONG_VALUE();
        if ((status = command_line_options_append_path(&options->config_paths,
                                                       value, value_len)) < 0) {
            return status;
        }
    } else if (STREQUAL(name, name_len, "ignore-config-errors")) {
        REJECT_LONG_VALUE();
        options->ignore_config_errors = true;
    } else if (STREQUAL(name, name_len, "test-lyrics-fetchers")) {
        REJECT_LONG_VALUE();
        options->test_lyrics_fetchers = true;
    } else if (STREQUAL(name, name_len, "bindings")) {
        REQUIRE_LONG_VALUE();
        if ((status = command_line_options_append_path(&options->bindings_paths,
                                                       value, value_len)) < 0) {
            return status;
        }
    } else if (STREQUAL(name, name_len, "screen")) {
        REQUIRE_LONG_VALUE();
        options->screen = true;
        configuration_copy_string(&options->screen_name, value, value_len);
    } else if (STREQUAL(name, name_len, "slave-screen")) {
        REQUIRE_LONG_VALUE();
        options->slave_screen = true;
        configuration_copy_string(&options->slave_screen_name, value,
                                  value_len);
    } else if (STREQUAL(name, name_len, "help")) {
        REJECT_LONG_VALUE();
        options->help = true;
    } else if (STREQUAL(name, name_len, "version")) {
        REJECT_LONG_VALUE();
        options->version = true;
    } else if (STREQUAL(name, name_len, "quiet")) {
        REJECT_LONG_VALUE();
        options->quiet = true;
    } else {
        char message[128];
        int32 len;

        len = SNPRINTF(message, "unrecognized option '--%.*s'", name_len, name);
        return ncm_error_set_status(ncm_error, -EINVAL, message, len);
    }

#undef REQUIRE_LONG_VALUE
#undef REJECT_LONG_VALUE

    return 0;
}

int32
ncm_configuration_options_parse(NcmConfigurationOptions *options, int32 argc,
                                char **argv, NcmError *ncm_error) {
    int32 status;

    for (int32 i = 1; i < argc; i += 1) {
        char *arg;
        int32 arg_len;

        arg = argv[i];
        arg_len = strlen32(arg);
        if (STREQUAL(arg, arg_len, "--")) {
            if (i + 1 < argc) {
                char message[192];
                int32 len;

                len = SNPRINTF(message,
                               "unexpected positional argument '%s'",
                               argv[i + 1]);
                return ncm_error_set_status(ncm_error, -EINVAL, message, len);
            }
            break;
        }
        if (BEGINS_WITH(arg, arg_len, "--")) {
            if ((status = configuration_parse_long_option(options, argc, argv,
                                                          &i,
                                                          ncm_error)) < 0) {
                return status;
            }
        } else if ((arg_len > 1) && (arg[0] == '-')) {
            if ((status = configuration_parse_short_option(options, argc, argv,
                                                           &i,
                                                           ncm_error)) < 0) {
                return status;
            }
        } else {
            char message[192];
            int32 len;

            len = SNPRINTF(message, "unexpected positional argument '%.*s'",
                           arg_len, arg);
            return ncm_error_set_status(ncm_error, -EINVAL, message, len);
        }
    }

    if ((options->config_paths.len == 0)
        || (options->bindings_paths.len == 0)) {
        StrBuilderArray default_config_paths;
        StrBuilderArray default_bindings_paths;

        str_builder_array_init(&default_config_paths);
        str_builder_array_init(&default_bindings_paths);
        if ((status = configuration_discover_default_paths(
            &default_config_paths, &default_bindings_paths, ncm_error)) < 0) {
            str_builder_array_destroy(&default_config_paths);
            str_builder_array_destroy(&default_bindings_paths);
            return status;
        }
        if (options->config_paths.len == 0) {
            for (int32 j = 0; j < default_config_paths.len; j += 1) {
                StrBuilder *path;

                path = &default_config_paths.items[j];
                if ((status = command_line_options_append_path(
                    &options->config_paths, path->data, path->len)) < 0) {
                    str_builder_array_destroy(&default_config_paths);
                    str_builder_array_destroy(&default_bindings_paths);
                    return status;
                }
            }
        }
        if (options->bindings_paths.len == 0) {
            for (int32 j = 0; j < default_bindings_paths.len; j += 1) {
                StrBuilder *path;

                path = &default_bindings_paths.items[j];
                if ((status = command_line_options_append_path(
                    &options->bindings_paths, path->data, path->len)) < 0) {
                    str_builder_array_destroy(&default_config_paths);
                    str_builder_array_destroy(&default_bindings_paths);
                    return status;
                }
            }
        }
        str_builder_array_destroy(&default_config_paths);
        str_builder_array_destroy(&default_bindings_paths);
    }
    return ncm_error_ok(ncm_error);
}

static void
configuration_print_usage(char *program_name) {
    StrBuilderArray config_paths;
    StrBuilderArray bindings_paths;
    NcmError ncm_error;

    str_builder_array_init(&config_paths);
    str_builder_array_init(&bindings_paths);
    ncm_error_clear(&ncm_error);
    if (configuration_discover_default_paths(&config_paths, &bindings_paths,
                                             &ncm_error) < 0) {
        configuration_print_error("Failed to build default paths", &ncm_error);
        str_builder_array_destroy(&bindings_paths);
        str_builder_array_destroy(&config_paths);
        return;
    }

    printf("Usage: %s [options]...\n", program_name);
    printf("Options:\n");
    printf("  -h, --host HOST              connect to server at host\n");
    printf("  -p, --port PORT              connect to server at port\n");
    printf("      --current-song[=FORMAT]  print current song using given ");
    printf("format and exit\n");
    printf("  -c, --config PATH            specify configuration file(s)\n");
    printf("                               default: ");
    for (int32 i = 0; i < config_paths.len; i += 1) {
        StrBuilder *path;

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
        StrBuilder *path;

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

    str_builder_array_destroy(&bindings_paths);
    str_builder_array_destroy(&config_paths);
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

static int32
configuration_make_string_views(NcmStringViewArray *views,
                                StrBuilderArray *buffers) {
    *views = (NcmStringViewArray){0};
    for (int32 i = 0; i < buffers->len; i += 1) {
        NcmStringView *view;
        StrBuilder *buffer;

        buffer = &buffers->items[i];
        if ((view = ncm_string_view_array_append(views)) == NULL) {
            ncm_string_view_array_destroy(views);
            return -ENOMEM;
        }
        view->data = buffer->data;
        view->len = buffer->len;
    }
    return 0;
}

static int32
configuration_read_settings(NcmConfigurationOptions *options,
                            NcmError *ncm_error) {
    NcmStringViewArray config_views;
    int32 status;

    for (int32 i = 0; i < options->config_paths.len; i += 1) {
        status = ncm_path_expand_home(&options->config_paths.items[i],
                                      ncm_error);
        if (status < 0) {
            return status;
        }
    }
    status = configuration_make_string_views(&config_views,
                                             &options->config_paths);
    if (status < 0) {
        return ncm_error_set_status(
            ncm_error, status, STRLIT("failed to build config path views"));
    }

    configuration_clear(&Config);
    status = configuration_read(&Config, &config_views,
                                options->ignore_config_errors, options->quiet,
                                ncm_error);
    if (status < 0) {
        ncm_string_view_array_destroy(&config_views);
        if (!ncm_error_is_set(ncm_error)) {
            return ncm_error_set_status(ncm_error, status,
                                        STRLIT("failed to read configuration"));
        }
        return status;
    }

    ncm_string_view_array_destroy(&config_views);
    return 0;
}

static int32
configuration_read_bindings(NcmConfigurationOptions *options,
                            NcmError *ncm_error) {
    int32 status;

    ncm_bindings_configuration_clear(&Bindings);
    for (int32 i = 0; i < options->bindings_paths.len; i += 1) {
        StrBuilder *path;

        path = &options->bindings_paths.items[i];
        if ((status = ncm_path_expand_home(path, ncm_error)) < 0) {
            return status;
        }
        if ((status = ncm_bindings_configuration_read(&Bindings, path->data,
                                                      path->len,
                                                      ncm_error)) < 0) {
            return status;
        }
    }
    ncm_bindings_configuration_generate_defaults(&Bindings);
    return 0;
}

static int32
configuration_create_directories(NcmError *ncm_error) {
    int32 status;

    if (!ncm_fs_path_is_existing(Config.ncmpcpp_directory,
                                 Config.ncmpcpp_directory_len)
        && ((status = ncm_fs_mkdir_all(Config.ncmpcpp_directory,
                                       Config.ncmpcpp_directory_len,
                                       ncm_error)) < 0)) {
        return status;
    }
    if (!ncm_fs_path_is_existing(Config.lyrics_directory,
                                 Config.lyrics_directory_len)
        && ((status = ncm_fs_mkdir_all(Config.lyrics_directory,
                                       Config.lyrics_directory_len,
                                       ncm_error)) < 0)) {
        return status;
    }
    return 0;
}

static int32
configuration_apply_mpd_environment(NcmError *ncm_error) {
    char *env_host;
    char *env_port;
    int32 port;
    int32 status;

    env_host = getenv("MPD_HOST");
    env_port = getenv("MPD_PORT");
    if (env_host != NULL) {
        if ((status = ncm_mpd_client_set_hostname(&global_mpd, env_host,
                                                  strlen32(env_host),
                                                  ncm_error)) < 0) {
            return status;
        }
    }
    if (env_port != NULL) {
        if ((status = ncm_parse_int32(env_port, strlen32(env_port),
                                      &port, ncm_error)) < 0) {
            return status;
        }
        if (port > 65535) {
            return ncm_error_set_status(ncm_error, -ERANGE,
                                        STRLIT("MPD_PORT is out of range"));
        }
        ncm_mpd_client_set_port(&global_mpd, (uint16)port);
    }
    return 0;
}

static int32
configuration_apply_mpd_command_line(NcmConfigurationOptions *options,
                                     NcmError *ncm_error) {
    int32 status;

    if (options->host_provided) {
        if ((status = ncm_mpd_client_set_hostname(&global_mpd,
                                                  options->host.data,
                                                  options->host.len,
                                                  ncm_error)) < 0) {
            return status;
        }
    }
    if (options->port_provided) {
        ncm_mpd_client_set_port(&global_mpd, (uint16)options->port);
    }
    if ((status = ncm_mpd_client_set_timeout_ms(
        &global_mpd, Config.mpd_connection_timeout*1000, ncm_error)) < 0) {
        return status;
    }
    return 0;
}

static int32
configuration_apply_screen_options(NcmConfigurationOptions *options,
                                   NcmError *ncm_error) {
    int32 status;

    if (options->screen) {
        status = screen_type_parse_startup(options->screen_name.data,
                                           options->screen_name.len,
                                           &Config.startup_screen_type);
        if (status < 0) {
            return ncm_error_set_status(ncm_error, -EINVAL,
                                        STRLIT("unknown screen"));
        }
    }
    if (options->slave_screen) {
        status = screen_type_parse_startup(options->slave_screen_name.data,
                                           options->slave_screen_name.len,
                                           &Config.startup_slave_screen_type);
        if (status < 0) {
            return ncm_error_set_status(ncm_error, -EINVAL,
                                        STRLIT("unknown slave screen"));
        }
        Config.has_startup_slave_screen_type = true;
    }
    return 0;
}

int32
ncm_configuration_options_apply(NcmConfigurationOptions *options,
                                NcmError *ncm_error) {
    int32 status;

    if ((status = configuration_read_settings(options, ncm_error)) < 0) {
        return status;
    }
    if ((status = configuration_create_directories(ncm_error)) < 0) {
        return status;
    }
    if ((status = configuration_apply_mpd_environment(ncm_error)) < 0) {
        return status;
    }
    if ((status = configuration_apply_mpd_command_line(options,
                                                       ncm_error)) < 0) {
        return status;
    }
    return configuration_apply_screen_options(options, ncm_error);
}

static int32
configuration_print_current_song(NcmConfigurationOptions *options,
                                 NcmError *ncm_error) {
    NcmSong song;
    NcmFormatAst format;
    StrBuilder output = {0};
    int32 status;

    song = (NcmSong){0};
    format = (NcmFormatAst){0};
    status = 0;

    if ((status = ncm_mpd_client_connect(&global_mpd, ncm_error)) < 0) {
        goto cleanup;
    }
    if ((status = ncm_mpd_client_get_current_song(&global_mpd, &song,
                                                  ncm_error)) < 0) {
        goto cleanup;
    }
    if (!ncm_song_is_empty(&song)) {
        status = ncm_format_parse(&format, options->current_song_format.data,
                                  options->current_song_format.len,
                                  NCM_FORMAT_FLAG_TAG, ncm_error);
        if (status < 0) {
            goto cleanup;
        }
        output = ncm_format_render_string(&format, &song);
        if (output.len > 0) {
            fwrite64(output.data, 1, output.len, stdout);
        }
    }

cleanup:
    sb_free(&output);
    ncm_format_ast_destroy(&format);
    ncm_song_destroy(&song);
    ncm_mpd_client_disconnect(&global_mpd);
    return status;
}

typedef struct ConfigurationLyricsFetcherTest {
    char *name;
    char *artist;
    char *title;
    int32 name_len;
    int32 artist_len;
    int32 title_len;
} ConfigurationLyricsFetcherTest;

static int32
configuration_test_lyrics_fetchers(NcmError *ncm_error) {
    ConfigurationLyricsFetcherTest tests[] = {
        {
            .name = "azlyrics",
            .artist = "luis fonsi",
            .title = "despacito",
            .name_len = STRLIT_LEN("azlyrics"),
            .artist_len = STRLIT_LEN("luis fonsi"),
            .title_len = STRLIT_LEN("despacito"),
        },
        {
            .name = "genius",
            .artist = "luis fonsi",
            .title = "despacito",
            .name_len = STRLIT_LEN("genius"),
            .artist_len = STRLIT_LEN("luis fonsi"),
            .title_len = STRLIT_LEN("despacito"),
        },
        {
            .name = "letras",
            .artist = "luis fonsi",
            .title = "despacito",
            .name_len = STRLIT_LEN("letras"),
            .artist_len = STRLIT_LEN("luis fonsi"),
            .title_len = STRLIT_LEN("despacito"),
        },
        {
            .name = "musixmatch",
            .artist = "luis fonsi",
            .title = "despacito",
            .name_len = STRLIT_LEN("musixmatch"),
            .artist_len = STRLIT_LEN("luis fonsi"),
            .title_len = STRLIT_LEN("despacito"),
        },
        {
            .name = "tekstowo",
            .artist = "luis fonsi",
            .title = "despacito",
            .name_len = STRLIT_LEN("tekstowo"),
            .artist_len = STRLIT_LEN("luis fonsi"),
            .title_len = STRLIT_LEN("despacito"),
        },
        {
            .name = "vagalume",
            .artist = "luis fonsi",
            .title = "despacito",
            .name_len = STRLIT_LEN("vagalume"),
            .artist_len = STRLIT_LEN("luis fonsi"),
            .title_len = STRLIT_LEN("despacito"),
        },
    };

    for (int32 i = 0; i < LENGTH(tests); i += 1) {
        NcmLyricsFetcherDef fetcher;
        NcmLyricsResult result;
        int32 status;

        fetcher = (NcmLyricsFetcherDef){0};
        result = (NcmLyricsResult){0};
        if ((status = ncm_lyrics_fetcher_def_set_name(&fetcher,
                                                      tests[i].name,
                                                      tests[i].name_len)) < 0) {
            ncm_error_set_status(ncm_error, status,
                                 STRLIT("unknown lyrics fetcher"));
            ncm_lyrics_result_destroy(&result);
            ncm_lyrics_fetcher_def_destroy(&fetcher);
            return status;
        }

        printf("%-20.*s : ", ncm_lyrics_fetcher_name_len(&fetcher),
               ncm_lyrics_fetcher_name(&fetcher));
        fflush(stdout);
        (void)ncm_lyrics_fetcher_fetch(&fetcher, &result, tests[i].artist,
                                       tests[i].artist_len, tests[i].title,
                                       tests[i].title_len);
        if (result.success) {
            printf("ok\n");
        } else {
            printf("failed\n");
        }
        ncm_lyrics_result_destroy(&result);
        ncm_lyrics_fetcher_def_destroy(&fetcher);
    }
    return 0;
}

static void
configuration_print_error(char *context, NcmError *ncm_error) {
    if (ncm_error && (ncm_error->message[0] != '\0')) {
        fprintf(stderr, "%s: %s\n", context, ncm_error->message);
    } else {
        fprintf(stderr, "%s\n", context);
    }
    return;
}

int32
configure(int32 argc, char **argv) {
    NcmConfigurationOptions options;
    NcmError ncm_error;
    int32 status;

    configuration_quiet = false;
    ncm_error_clear(&ncm_error);
    ncm_configuration_options_init(&options);
    if ((status = ncm_configuration_options_parse(&options, argc, argv,
                                                  &ncm_error)) < 0) {
        configuration_print_error("Error while processing configuration",
                                  &ncm_error);
        ncm_configuration_options_destroy(&options);
        exit(EXIT_FAILURE);
    }

    configuration_quiet = options.quiet;

    if (options.help) {
        configuration_print_usage(argv[0]);
        ncm_configuration_options_destroy(&options);
        return 0;
    }
    if (options.version) {
        configuration_print_version();
        ncm_configuration_options_destroy(&options);
        return 0;
    }
    if (options.test_lyrics_fetchers) {
        status = configuration_test_lyrics_fetchers(&ncm_error);
        ncm_configuration_options_destroy(&options);
        if (status < 0) {
            configuration_print_error("Error while testing lyrics fetchers",
                                      &ncm_error);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    status = ncm_configuration_options_apply(&options, &ncm_error);
    if ((status >= 0) && !options.current_song) {
        status = configuration_read_bindings(&options, &ncm_error);
    }
    if ((status >= 0) && options.current_song) {
        status = configuration_print_current_song(&options, &ncm_error);
        ncm_configuration_options_destroy(&options);
        if (status < 0) {
            configuration_print_error("Error while printing current song",
                                      &ncm_error);
            exit(EXIT_FAILURE);
        }
        return 0;
    }
    if (status < 0) {
        configuration_print_error("Error while processing configuration",
                                  &ncm_error);
        ncm_configuration_options_destroy(&options);
        exit(EXIT_FAILURE);
    }

    ncm_configuration_options_destroy(&options);
    return 1;
}

#endif /* NCMPCPP_CONFIGURATION_C */
