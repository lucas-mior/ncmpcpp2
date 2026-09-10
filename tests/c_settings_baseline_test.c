#include "cbase.h"
#include "ncmpcpp2.h"

#define main ncmpcpp2_application_main
#include "main.c"
#undef main

static int32
settings_test_apply(SettingsApplyFn apply, Configuration *config,
                    char *value) {
    NcmError ncm_error = {0};

    return apply(config, value, strlen32(value), &ncm_error);
}

static void
settings_test_int_range(SettingsApplyFn apply, Configuration *config,
                        int32 *field, int32 minimum, int32 maximum) {
    char value[64];

    SNPRINTF(value, "%d", minimum);
    ASSERT_ZERO(settings_test_apply(apply, config, value));
    ASSERT(*field == minimum);

    if (minimum != INT32_MIN) {
        SNPRINTF(value, "%d", minimum - 1);
        ASSERT(settings_test_apply(apply, config, value) < 0);
        ASSERT(*field == minimum);
    }

    SNPRINTF(value, "%d", maximum);
    ASSERT_ZERO(settings_test_apply(apply, config, value));
    ASSERT(*field == maximum);

    if (maximum != INT32_MAX) {
        SNPRINTF(value, "%d", maximum + 1);
        ASSERT(settings_test_apply(apply, config, value) < 0);
        ASSERT(*field == maximum);
    }
    return;
}

static void
settings_test_double_range(SettingsApplyFn apply, Configuration *config,
                           double *field, double minimum, double maximum) {
    char value[64];
    double outside;

    if (minimum != -HUGE_VAL) {
        SNPRINTF(value, "%.17g", minimum);
        ASSERT_ZERO(settings_test_apply(apply, config, value));
        ASSERT(*field == minimum);

        outside = nextafter(minimum, -HUGE_VAL);
        SNPRINTF(value, "%.17g", outside);
        ASSERT(settings_test_apply(apply, config, value) < 0);
        ASSERT(*field == minimum);
    }

    if (maximum != HUGE_VAL) {
        SNPRINTF(value, "%.17g", maximum);
        ASSERT_ZERO(settings_test_apply(apply, config, value));
        ASSERT(*field == maximum);

        outside = nextafter(maximum, HUGE_VAL);
        SNPRINTF(value, "%.17g", outside);
        ASSERT(settings_test_apply(apply, config, value) < 0);
        ASSERT(*field == maximum);
    }
    return;
}

static void
settings_assert_generated_empty(Configuration *config) {

#define XX_BOOL(NAME, DEFAULT)                                           \
    ASSERT(!config->NAME);
#define XX_STRING(NAME, DEFAULT)                                         \
    ASSERT(config->NAME == NULL);                                              \
    ASSERT(config->NAME##_len == 0);
#define XX_PATH(NAME, DEFAULT)                                           \
    ASSERT(config->NAME == NULL);                                              \
    ASSERT(config->NAME##_len == 0);
#define XX_DIR(NAME, DEFAULT)                                            \
    ASSERT(config->NAME == NULL);                                              \
    ASSERT(config->NAME##_len == 0);
#define XX_INTEGER(NAME, DEFAULT, MINIMUM, MAXIMUM)                      \
    ASSERT(config->NAME == 0);
#define XX_DOUBLE(NAME, DEFAULT, MINIMUM, MAXIMUM)                       \
    ASSERT(config->NAME == 0);
#define XX_ENUM(NAME, DEFAULT, ENUM_PREFIX_)                             \
    ASSERT(config->NAME == (ENUM_PREFIX_)0);
#define XX_MPD_TAG(NAME, DEFAULT)                                        \
    ASSERT(config->NAME == TAG_UNKNOWN);
#define XX_STARTUP_SCREEN(NAME, DEFAULT)                                 \
    ASSERT(config->NAME == SCREEN_TYPE_COUNT);
#define XX_OPT_STARTUP_SCREEN(NAME, DEFAULT, PRESENT_FIELD, UNSET_VALUE) \
    ASSERT(config->NAME == (SCREEN_TYPE_)(UNSET_VALUE));                       \
    ASSERT(!config->PRESENT_FIELD);
#define XX_COLOR(NAME, DEFAULT)                                          \
    ASSERT(nc_color_is_default(config->NAME));
#define XX_FORMATTED_COLOR(NAME, DEFAULT)                                \
    ASSERT(config->NAME.formats == NULL);                                      \
    ASSERT(nc_color_is_default(config->NAME.color));
#define XX_BORDER(NAME, DEFAULT)                                         \
    ASSERT(!config->NAME.enabled);                                             \
    ASSERT(nc_color_is_default(config->NAME.color));
#define XX_FORMAT(NAME, DEFAULT, FLAGS)                                  \
    ASSERT(config->NAME.root.items == NULL);                                   \
    ASSERT(config->NAME.root.len == 0);                                        \
    ASSERT(config->NAME.root.cap == 0);
#define XX_BUFFER(NAME, DEFAULT, KEEP_EXISTING)                          \
    ASSERT(config->NAME.data == NULL);                                         \
    ASSERT(config->NAME.properties == NULL);                                   \
    ASSERT(config->NAME.len == 0);                                             \
    ASSERT(config->NAME.cap == 0);
#define XX_LOOK(NAME, DEFAULT, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)         \
    ASSERT(config->NAME.data == NULL);                                         \
    ASSERT(config->NAME.len == 0);                                             \
    ASSERT(config->NAME.cap == 0);
#define XX_RATIO(NAME, DEFAULT, EXPECTED_LEN)                            \
    ASSERT(config->NAME.items == NULL);                                        \
    ASSERT(config->NAME.len == 0);                                             \
    ASSERT(config->NAME.cap == 0);
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT)                           \
    ASSERT(config->NAME.items == NULL);                                        \
    ASSERT(config->NAME.len == 0);                                             \
    ASSERT(config->NAME.cap == 0);
#define XX_LYRICS_FETCHERS(NAME, DEFAULT)                                \
    ASSERT(config->NAME.fetchers.items == NULL);                               \
    ASSERT(config->NAME.fetchers.len == 0);                                    \
    ASSERT(config->NAME.fetchers.cap == 0);
#define XX_SCREEN_LIST(NAME, DEFAULT, PREVIOUS_FIELD)                    \
    ASSERT(config->NAME.items == NULL);                                        \
    ASSERT(config->NAME.len == 0);                                             \
    ASSERT(config->NAME.cap == 0);                                             \
    ASSERT(!config->PREVIOUS_FIELD);
#define XX_UINT32_CHOICE(NAME, DEFAULT, PARSER, UNSET_VALUE)             \
    ASSERT(config->NAME == (UNSET_VALUE));
#define XX_COLUMNS(NAME, DEFAULT, FORMAT_FIELD)                          \
    ASSERT(config->FORMAT_FIELD.root.items == NULL);                           \
    ASSERT(config->FORMAT_FIELD.root.len == 0);                                \
    ASSERT(config->FORMAT_FIELD.root.cap == 0);                                \
    ASSERT(config->NAME.items == NULL);                                        \
    ASSERT(config->NAME.len == 0);                                             \
    ASSERT(config->NAME.cap == 0);

#include "config_options_pass.h"

    return;
}

static void
test_generated_option_identity(void) {
#define XX_OPTION(NAME, DEFAULT, ...) \
    ASSERT_EQUAL( \
        ncmpcpp_options[SETTINGS_OPTION_##NAME].name, \
        ncmpcpp_options[SETTINGS_OPTION_##NAME].name_len, #NAME); \
    ASSERT_EQUAL( \
        ncmpcpp_options[SETTINGS_OPTION_##NAME].default_value, \
        ncmpcpp_options[SETTINGS_OPTION_##NAME].default_value_len, \
        DEFAULT); \
    ASSERT(ncmpcpp_options[SETTINGS_OPTION_##NAME].apply == apply_##NAME);
#include "config_options_pass.h"

    return;
}

static void
test_each_declared_default(void) {
    for (uint32 i = 0; i < SETTINGS_OPTION_COUNT; i += 1) {
        Configuration config = {0};
        SettingsOption option = ncmpcpp_options[i];
        NcmError ncm_error = {0};
        int32 status;

        config_init(&config);
        status = option.apply(&config, option.default_value,
                              option.default_value_len, &ncm_error);
        ASSERT_ZERO(status);
        config_destroy(&config);
        settings_assert_generated_empty(&config);
    }
    return;
}

static void
test_option_table_shape(void) {
    for (uint32 i = 0; i < SETTINGS_OPTION_COUNT; i += 1) {
        SettingsOption left = ncmpcpp_options[i];

        ASSERT(left.name != NULL);
        ASSERT(left.default_value != NULL);
        ASSERT(left.apply != NULL);
        ASSERT(left.name_len == strlen32(left.name));
        ASSERT(left.default_value_len == strlen32(left.default_value));
        for (uint32 j = i + 1; j < SETTINGS_OPTION_COUNT; j += 1) {
            SettingsOption right = ncmpcpp_options[j];

            ASSERT(!STREQUAL(left.name, left.name_len,
                             right.name, right.name_len));
        }
    }
    return;
}

static void
test_declared_defaults_and_cleanup(void) {
    Configuration config = {0};
    StringViewArray paths = {0};
    NcmError ncm_error = {0};

    config_init(&config);
    settings_assert_generated_empty(&config);
    ASSERT_ZERO(config_read(&config, &paths, false, true, &ncm_error));

    ASSERT(config.mpd_port == 6600);
    ASSERT(config.visualizer_fps == 60);
    ASSERT(config.visualizer_spectrum_gain == 10.0);
    ASSERT(config.visualizer_spectrum_hz_min == 20.0);
    ASSERT(config.visualizer_spectrum_hz_max == 20000.0);
    ASSERT(config.locked_screen_width_part == 50.0);
    ASSERT(config.search_engine_default_search_mode == 1);
    ASSERT(config.screen_switcher_mode.len == 2);
    ASSERT(!config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == SCREEN_TYPE_COUNT);
    ASSERT(config.regular_expressions
           == NCM_REGEX_EXTENDED_CASE_INSENSITIVE);
#if defined(HAVE_FFTW3_H)
    ASSERT(config.visualizer_type == NCM_VISUALIZER_TYPE_SPECTRUM);
#else
    ASSERT(config.visualizer_type == NCM_VISUALIZER_TYPE_ELLIPSE);
#endif
    ASSERT(config.browser_sort_mode == NCM_SORT_MODE_TYPE);
    ASSERT(config.playlist_display_mode == NCM_DISPLAY_MODE_COLUMNS);
    ASSERT(config.browser_display_mode == NCM_DISPLAY_MODE_CLASSIC);
    ASSERT(config.search_engine_display_mode == NCM_DISPLAY_MODE_CLASSIC);
    ASSERT(config.playlist_edit_display_mode == NCM_DISPLAY_MODE_CLASSIC);
    ASSERT(config.user_interface == NCM_DESIGN_CLASSIC);
    ASSERT(config.media_library_primary_tag == TAG_ARTIST);
    ASSERT(config.space_add_mode == NCM_SPACE_ADD_MODE_ADD_REMOVE);
    ASSERT(config.startup_screen == SCREEN_TYPE_PLAYLIST);

    config_destroy(&config);
    settings_assert_generated_empty(&config);
    ASSERT(config.ncmpcpp_directory == NULL);
    ASSERT(config.ncmpcpp_directory_len == 0);
    ASSERT(config.visualizer_color.items == NULL);
    ASSERT(config.screen_switcher_mode.items == NULL);
    ASSERT(config.lyrics_fetchers.fetchers.items == NULL);

    config_destroy(&config);
    return;
}

static void
test_runtime_application_is_separate(void) {
    Configuration config = {0};
    StringViewArray paths = {0};
    NcmError ncm_error = {0};
    StrBuilder previous_term = {0};
    char *term;
    bool had_term;

    term = getenv("TERM");
    had_term = term != NULL;
    if (had_term) {
        SB_APPEND(&previous_term, term, strlen32(term));
    }
    ASSERT_ZERO(setenv("TERM", "linux", 1));

    config_init(&config);

    ASSERT_ZERO(ncm_mpd_client_set_hostname(
        &global_mpd, STRLIT("before-host"), &ncm_error));
    ncm_mpd_client_set_port(&global_mpd, 1234);
    ASSERT_ZERO(ncm_mpd_client_set_password(
        &global_mpd, STRLIT("before-password"), &ncm_error));
    ASSERT_ZERO(ncm_mpd_client_set_timeout_ms(
        &global_mpd, 4321, &ncm_error));

    ASSERT_ZERO(config_read(&config, &paths, false, true, &ncm_error));
    ASSERT(config.enable_window_title);
    ASSERT_EQUAL(global_mpd.host.data, global_mpd.host.len,
                 "before-host");
    ASSERT(global_mpd.port == 1234);
    ASSERT_EQUAL(global_mpd.password.data, global_mpd.password.len,
                 "before-password");
    ASSERT(global_mpd.timeout_ms == 4321);

    ASSERT_ZERO(settings_test_apply(apply_mpd_host, &config, "after-host"));
    ASSERT_ZERO(settings_test_apply(apply_mpd_port, &config, "4567"));
    ASSERT_ZERO(settings_test_apply(apply_mpd_password, &config,
                                    "after-password"));
    ASSERT_ZERO(settings_test_apply(apply_mpd_connection_timeout,
                                    &config, "9"));
    ASSERT_ZERO(settings_test_apply(apply_enable_window_title, &config, "yes"));

    ASSERT(config.enable_window_title);
    ASSERT_EQUAL(global_mpd.host.data, global_mpd.host.len, "before-host");
    ASSERT(global_mpd.port == 1234);
    ASSERT_EQUAL(global_mpd.password.data, global_mpd.password.len,
                 "before-password");
    ASSERT(global_mpd.timeout_ms == 4321);

    ASSERT_ZERO(config_apply_runtime(&config, &global_mpd, true,
                                            &ncm_error));
    ASSERT(config.enable_window_title);
    ASSERT(!window_title_enabled);
    ASSERT_EQUAL(global_mpd.host.data, global_mpd.host.len, "after-host");
    ASSERT(global_mpd.port == 4567);
    ASSERT_EQUAL(global_mpd.password.data, global_mpd.password.len,
                 "after-password");
    ASSERT(global_mpd.timeout_ms == 9000);

    if (had_term) {
        ASSERT_ZERO(setenv("TERM", sb_opt_cstr(&previous_term), 1));
    } else {
        ASSERT_ZERO(unsetenv("TERM"));
    }
    sb_free(&previous_term);
    config_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    return;
}

static void
test_config_options_apply_runtime_precedence(void) {
    static char contents[] =
        "ncmpcpp_directory = /tmp/\n"
        "lyrics_directory = /tmp/\n"
        "mpd_host = config-host\n"
        "mpd_port = 1111\n"
        "mpd_password = config-password\n"
        "mpd_connection_timeout = 7\n"
        "enable_window_title = no\n";
    NcmConfigurationOptions options = {0};
    NcmError ncm_error = {0};
    StrBuilder previous_host = {0};
    StrBuilder previous_port = {0};
    StrBuilder *config_path;
    char *env_host;
    char *env_port;
    char path[PATH_MAX];
    int32 contents_len = SIZEOF(contents) - 1;
    int32 fd;
    bool had_host;
    bool had_port;

    env_host = getenv("MPD_HOST");
    env_port = getenv("MPD_PORT");
    had_host = env_host != NULL;
    had_port = env_port != NULL;
    if (had_host) {
        SB_APPEND(&previous_host, env_host, strlen32(env_host));
    }
    if (had_port) {
        SB_APPEND(&previous_port, env_port, strlen32(env_port));
    }
    ASSERT_ZERO(unsetenv("MPD_HOST"));
    ASSERT_ZERO(unsetenv("MPD_PORT"));

    fd = cbase_make_temp_file(path, SIZEOF(path),
                              "ncmpcpp2-settings-runtime", ".conf");
    ASSERT_NON_NEGATIVE(fd);
    ASSERT_ZERO(XCLOSE(&fd, path));
    ASSERT(write_entire_file(path, contents, contents_len) == contents_len);

    ncm_config_options_init(&options);
    options.quiet = true;
    config_path = str_builder_array_append(&options.config_paths);
    SB_APPEND(config_path, path, strlen32(path));

    ASSERT_ZERO(ncm_config_options_apply(&options, &ncm_error));
    ASSERT_EQUAL(global_mpd.host.data, global_mpd.host.len, "config-host");
    ASSERT(global_mpd.port == 1111);
    ASSERT_EQUAL(global_mpd.password.data, global_mpd.password.len,
                 "config-password");
    ASSERT(global_mpd.timeout_ms == 7000);
    ASSERT_EQUAL(Config.mpd_host, Config.mpd_host_len, "config-host");
    ASSERT(Config.mpd_port == 1111);
    ASSERT_EQUAL(Config.mpd_password, Config.mpd_password_len,
                 "config-password");
    ASSERT(Config.mpd_connection_timeout == 7);
    ASSERT(!window_title_enabled);

    ASSERT_ZERO(setenv("MPD_HOST", "env-host", 1));
    ASSERT_ZERO(setenv("MPD_PORT", "2222", 1));
    ncm_error_clear(&ncm_error);
    ASSERT_ZERO(ncm_config_options_apply(&options, &ncm_error));
    ASSERT_EQUAL(global_mpd.host.data, global_mpd.host.len, "env-host");
    ASSERT(global_mpd.port == 2222);
    ASSERT_EQUAL(global_mpd.password.data, global_mpd.password.len,
                 "config-password");
    ASSERT(global_mpd.timeout_ms == 7000);
    ASSERT_EQUAL(Config.mpd_host, Config.mpd_host_len, "config-host");
    ASSERT(Config.mpd_port == 1111);
    ASSERT_EQUAL(Config.mpd_password, Config.mpd_password_len,
                 "config-password");
    ASSERT(Config.mpd_connection_timeout == 7);

    sb_clear(&options.host);
    SB_APPEND(&options.host, "cli-host");
    options.host_provided = true;
    options.port = 3333;
    options.port_provided = true;
    ncm_error_clear(&ncm_error);
    ASSERT_ZERO(ncm_config_options_apply(&options, &ncm_error));
    ASSERT_EQUAL(global_mpd.host.data, global_mpd.host.len, "cli-host");
    ASSERT(global_mpd.port == 3333);
    ASSERT_EQUAL(global_mpd.password.data, global_mpd.password.len,
                 "config-password");
    ASSERT(global_mpd.timeout_ms == 7000);
    ASSERT_EQUAL(Config.mpd_host, Config.mpd_host_len, "config-host");
    ASSERT(Config.mpd_port == 1111);
    ASSERT_EQUAL(Config.mpd_password, Config.mpd_password_len,
                 "config-password");
    ASSERT(Config.mpd_connection_timeout == 7);

    if (had_host) {
        ASSERT_ZERO(setenv("MPD_HOST", sb_opt_cstr(&previous_host), 1));
    } else {
        ASSERT_ZERO(unsetenv("MPD_HOST"));
    }
    if (had_port) {
        ASSERT_ZERO(setenv("MPD_PORT", sb_opt_cstr(&previous_port), 1));
    } else {
        ASSERT_ZERO(unsetenv("MPD_PORT"));
    }

    sb_free(&previous_host);
    sb_free(&previous_port);
    ncm_config_options_destroy(&options);
    ASSERT_ZERO(cbase_remove_file(path));
    return;
}

static void
test_generated_numeric_boundaries(void) {
    Configuration config = {0};

    config_init(&config);

#define XX_INTEGER(NAME, DEFAULT, MINI, MAXI) \
    settings_test_int_range(apply_##NAME, &config, &config.NAME, MINI, MAXI);
#define XX_DOUBLE(NAME, DEFAULT, MINI, MAXI) \
    settings_test_double_range(apply_##NAME, &config, &config.NAME, MINI, MAXI);
#include "config_options_pass.h"

    config_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_numeric_boundaries(void) {
    Configuration config = {0};
    NcmError ncm_error = {0};

    config_init(&config);

    ASSERT_ZERO(settings_test_apply(apply_mpd_port, &config, "-1"));
    ASSERT_EQUAL(config.mpd_port, -1);

    ASSERT_ZERO(settings_test_apply(apply_visualizer_spectrum_hz_min,
                                    &config, "20"));
    ASSERT_ZERO(settings_test_apply(apply_visualizer_spectrum_hz_max,
                                    &config, "20"));
    ASSERT(config_validate(&config, &ncm_error) < 0);
    ASSERT_ZERO(settings_test_apply(apply_visualizer_spectrum_hz_max,
                                    &config, "21"));
    ncm_error_clear(&ncm_error);
    ASSERT_ZERO(config_validate(&config, &ncm_error));

    ASSERT_ZERO(settings_test_apply(apply_locked_screen_width_part,
                                    &config, "20"));
    ASSERT_EQUAL(config.locked_screen_width_part, 20.0);
    ASSERT_EQUAL(config_locked_screen_width_fraction(&config), 0.2);
    ASSERT_ZERO(settings_test_apply(apply_locked_screen_width_part,
                                    &config, "80"));
    ASSERT(config.locked_screen_width_part == 80.0);
    ASSERT(config_locked_screen_width_fraction(&config) == 0.8);

    ASSERT_ZERO(settings_test_apply(apply_search_engine_default_search_mode,
                                    &config, "1"));
    ASSERT(config.search_engine_default_search_mode == 1);
    ASSERT(config_search_engine_default_mode(&config)
           == SEARCH_ENGINE_SEARCH_MODE_LITERAL);
    ASSERT_ZERO(settings_test_apply(apply_search_engine_default_search_mode,
                                    &config, "3"));
    ASSERT(config.search_engine_default_search_mode == 3);
    ASSERT(config_search_engine_default_mode(&config)
           == SEARCH_ENGINE_SEARCH_MODE_EXACT);
    ASSERT(settings_test_apply(apply_search_engine_default_search_mode,
                               &config, "4") < 0);
    ASSERT(config.search_engine_default_search_mode == 3);

    ASSERT_ZERO(settings_test_apply(apply_system_encoding, &config, "UTF-8"));
    ASSERT_EQUAL(config.system_encoding, config.system_encoding_len, "UTF-8");

    config_destroy(&config);
    return;
}

static void
test_enum_options(void) {
    Configuration config = {0};

    config_init(&config);

    ASSERT_ZERO(settings_test_apply(
        apply_browser_sort_mode, &config, "none"));
    ASSERT(config.browser_sort_mode == NCM_SORT_MODE_NONE);
    ASSERT(settings_test_apply(
        apply_browser_sort_mode, &config, "invalid") < 0);
    ASSERT(config.browser_sort_mode == NCM_SORT_MODE_NONE);

    ASSERT_ZERO(settings_test_apply(
        apply_playlist_display_mode, &config, "columns"));
    ASSERT(config.playlist_display_mode == NCM_DISPLAY_MODE_COLUMNS);
    ASSERT(settings_test_apply(
        apply_playlist_display_mode, &config, "invalid") < 0);
    ASSERT(config.playlist_display_mode == NCM_DISPLAY_MODE_COLUMNS);

    ASSERT_ZERO(settings_test_apply(
        apply_media_library_primary_tag, &config, "performer"));
    ASSERT(config.media_library_primary_tag == TAG_PERFORMER);
    ASSERT(settings_test_apply(
        apply_media_library_primary_tag, &config, "invalid") < 0);
    ASSERT(config.media_library_primary_tag == TAG_PERFORMER);

    ASSERT_ZERO(settings_test_apply(
        apply_startup_screen, &config, "playlist"));
    ASSERT(config.startup_screen == SCREEN_TYPE_PLAYLIST);

    config_destroy(&config);
    return;
}

static void
test_optional_enum_options(void) {
    Configuration config = {0};

    config_init(&config);

    ASSERT(!config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == SCREEN_TYPE_COUNT);
    ASSERT_ZERO(settings_test_apply(
        apply_startup_slave_screen, &config, "browser"));
    ASSERT(config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == SCREEN_TYPE_BROWSER);

    ASSERT(settings_test_apply(
        apply_startup_slave_screen, &config, "invalid") < 0);
    ASSERT(config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == SCREEN_TYPE_COUNT);

    ASSERT_ZERO(settings_test_apply(
        apply_startup_slave_screen, &config, ""));
    ASSERT(!config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == SCREEN_TYPE_COUNT);

    config_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_color_options(void) {
    Configuration config = {0};
    NcColor expected;

    config_init(&config);

    ASSERT_ZERO(settings_test_apply(
        apply_header_window_color, &config, "red_blue"));
    expected = nc_color_make(COLOR_RED, COLOR_BLUE, false, false);
    ASSERT(nc_color_is_equal(config.header_window_color, expected));
    ASSERT(settings_test_apply(
        apply_header_window_color, &config, "red_invalid") < 0);
    ASSERT(nc_color_is_equal(config.header_window_color, expected));

    ASSERT_ZERO(settings_test_apply(
        apply_state_flags_color, &config, "green:bu"));
    expected = nc_color_make(COLOR_GREEN, NC_COLOR_CURRENT, false, false);
    ASSERT(nc_color_is_equal(config.state_flags_color.color, expected));
    ASSERT(ARRAY_LEN(config.state_flags_color.formats) == 2);
    ASSERT(config.state_flags_color.formats[0] == NC_FORMAT_BOLD);
    ASSERT(config.state_flags_color.formats[1] == NC_FORMAT_UNDERLINE);
    ASSERT(settings_test_apply(
        apply_state_flags_color, &config, "green:x") < 0);
    ASSERT(ARRAY_LEN(config.state_flags_color.formats) == 2);

    ASSERT_ZERO(settings_test_apply(
        apply_window_border_color, &config, "cyan"));
    expected = nc_color_make(COLOR_CYAN, NC_COLOR_CURRENT, false, false);
    ASSERT(config.window_border_color.enabled);
    ASSERT(nc_color_is_equal(config.window_border_color.color, expected));
    ASSERT(settings_test_apply(
        apply_window_border_color, &config, "invalid") < 0);
    ASSERT(config.window_border_color.enabled);
    ASSERT(nc_color_is_equal(config.window_border_color.color, expected));

    config_destroy(&config);
    return;
}

static void
test_format_options(void) {
    Configuration config = {0};

    config_init(&config);

    ASSERT_ZERO(settings_test_apply(
        apply_song_list_format, &config, "$R"));
    ASSERT(config.song_list_format.root.len == 1);
    ASSERT(config.song_list_format.root.items[0].type
           == NCM_FORMAT_EXPR_OUTPUT_SWITCH);

    ASSERT(settings_test_apply(
        apply_song_status_format, &config, "$R") < 0);
    ASSERT(config.song_status_format.root.len == 0);
    ASSERT_ZERO(settings_test_apply(
        apply_song_status_format, &config, "%a"));
    ASSERT(config.song_status_format.root.len == 1);
    ASSERT(config.song_status_format.root.items[0].type
           == NCM_FORMAT_EXPR_SONG_TAG);

    ASSERT_ZERO(settings_test_apply(
        apply_song_window_title_format, &config, "$R"));
    ASSERT(config.song_window_title_format.root.len == 1);
    ASSERT(config.song_window_title_format.root.items[0].type
           == NCM_FORMAT_EXPR_TEXT);

    ASSERT_ZERO(settings_test_apply(
        apply_browser_sort_format, &config, "%a"));
    ASSERT(config.browser_sort_format.root.len == 1);
    ASSERT(config.browser_sort_format.root.items[0].type
           == NCM_FORMAT_EXPR_SONG_TAG);

    config_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_buffer_and_look_options(void) {
    Configuration config = {0};

    config_init(&config);

    ASSERT_ZERO(settings_test_apply(
        apply_selected_item_prefix, &config, "selected"));
    ASSERT_EQUAL(config.selected_item_prefix.data,
                 config.selected_item_prefix.len, "selected");
    ASSERT_ZERO(settings_test_apply(
        apply_current_item_prefix, &config, "first"));
    ASSERT_ZERO(settings_test_apply(
        apply_current_item_prefix, &config, "second"));
    ASSERT_EQUAL(config.current_item_prefix.data,
                 config.current_item_prefix.len, "first");
    ASSERT_ZERO(settings_test_apply(
        apply_browser_playlist_prefix, &config, "playlist "));
    ASSERT_EQUAL(config.browser_playlist_prefix.data,
                 config.browser_playlist_prefix.len, "playlist ");

    ASSERT_ZERO(settings_test_apply(apply_visualizer_look, &config, "ab"));
    ASSERT(config.visualizer_look.len == 2);
    ASSERT(config.visualizer_look.data[0] == 'a');
    ASSERT(config.visualizer_look.data[1] == 'b');
    ASSERT(settings_test_apply(apply_visualizer_look, &config, "a") < 0);
    ASSERT(config.visualizer_look.len == 2);

    ASSERT_ZERO(settings_test_apply(apply_progressbar_look, &config, "ab"));
    ASSERT(config.progressbar_look.len == 3);
    ASSERT(config.progressbar_look.data[0] == 'a');
    ASSERT(config.progressbar_look.data[1] == 'b');
    ASSERT(config.progressbar_look.data[2] == '\0');
    ASSERT_ZERO(settings_test_apply(apply_progressbar_look, &config, "abc"));
    ASSERT(config.progressbar_look.len == 3);
    ASSERT(config.progressbar_look.data[2] == 'c');
    ASSERT(settings_test_apply(apply_progressbar_look, &config, "a") < 0);
    ASSERT(config.progressbar_look.len == 3);
    ASSERT(config.progressbar_look.data[2] == 'c');

    config_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_collection_options(void) {
    Configuration config = {0};

    config_init(&config);

    ASSERT_ZERO(settings_test_apply(
        apply_media_library_column_width_ratio_two, &config, "2:3"));
    ASSERT(config.media_library_column_width_ratio_two.len == 2);
    ASSERT(config.media_library_column_width_ratio_two.items[0] == 2);
    ASSERT(config.media_library_column_width_ratio_two.items[1] == 3);
    ASSERT(settings_test_apply(
        apply_media_library_column_width_ratio_two, &config, "1") < 0);
    ASSERT(config.media_library_column_width_ratio_two.len == 1);
    ASSERT(settings_test_apply(
        apply_media_library_column_width_ratio_two, &config, "0:0") < 0);

    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_color, &config, "red, , blue"));
    ASSERT(config.visualizer_color.len == 2);
    ASSERT(settings_test_apply(
        apply_visualizer_color, &config, " , ") < 0);
    ASSERT(config.visualizer_color.len == 0);

    ASSERT_ZERO(settings_test_apply(
        apply_lyrics_fetchers, &config, "genius, internet"));
    ASSERT(config.lyrics_fetchers.fetchers.len == 2);
    ASSERT(settings_test_apply(
        apply_lyrics_fetchers, &config, "genius, invalid") < 0);
    ASSERT(config.lyrics_fetchers.fetchers.len == 1);

    ASSERT_ZERO(settings_test_apply(
        apply_screen_switcher_mode, &config, "playlist, browser"));
    ASSERT(!config.screen_switcher_previous);
    ASSERT(config.screen_switcher_mode.len == 2);
    ASSERT(config.screen_switcher_mode.items[0]
           == SCREEN_TYPE_PLAYLIST);
    ASSERT(config.screen_switcher_mode.items[1]
           == SCREEN_TYPE_BROWSER);
    ASSERT_ZERO(settings_test_apply(
        apply_screen_switcher_mode, &config, "previous"));
    ASSERT(config.screen_switcher_previous);
    ASSERT(config.screen_switcher_mode.len == 0);

    config_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_remaining_generated_options(void) {
    Configuration config = {0};

    config_init(&config);

    ASSERT_ZERO(settings_test_apply(
        apply_default_place_to_search_in, &config, "database"));
    ASSERT(config.default_place_to_search_in
           == NCM_DEFAULT_SEARCH_SOURCE_DATABASE);
    ASSERT_ZERO(settings_test_apply(
        apply_default_place_to_search_in, &config, "playlist"));
    ASSERT(config.default_place_to_search_in
           == NCM_DEFAULT_SEARCH_SOURCE_PLAYLIST);
    ASSERT(settings_test_apply(
        apply_default_place_to_search_in, &config, "invalid") < 0);
    ASSERT(config.default_place_to_search_in
           == NCM_DEFAULT_SEARCH_SOURCE_PLAYLIST);

    ASSERT_ZERO(settings_test_apply(
        apply_default_find_mode, &config, "wrapped"));
    ASSERT(config.default_find_mode == NCM_DEFAULT_FIND_MODE_WRAPPED);
    ASSERT_ZERO(settings_test_apply(
        apply_default_find_mode, &config, "normal"));
    ASSERT(config.default_find_mode == NCM_DEFAULT_FIND_MODE_NORMAL);
    ASSERT(settings_test_apply(
        apply_default_find_mode, &config, "invalid") < 0);
    ASSERT(config.default_find_mode == NCM_DEFAULT_FIND_MODE_NORMAL);

    ASSERT_ZERO(settings_test_apply(
        apply_regular_expressions, &config, "none"));
    ASSERT(config.regular_expressions
           == NCM_REGEX_LITERAL_CASE_INSENSITIVE);
    ASSERT_ZERO(settings_test_apply(
        apply_regular_expressions, &config, "basic"));
    ASSERT(config.regular_expressions
           == NCM_REGEX_BASIC_CASE_INSENSITIVE);
    ASSERT_ZERO(settings_test_apply(
        apply_regular_expressions, &config, "extended"));
    ASSERT(config.regular_expressions
           == NCM_REGEX_EXTENDED_CASE_INSENSITIVE);
    ASSERT(settings_test_apply(
        apply_regular_expressions, &config, "invalid") < 0);
    ASSERT(config.regular_expressions
           == NCM_REGEX_EXTENDED_CASE_INSENSITIVE);

    ASSERT_ZERO(settings_test_apply(
        apply_song_columns_list_format, &config,
        "(10)[red]{a:Artist} (5f)[blue]{rE|t:Title}"));
    ASSERT(config.song_columns_list_format.len == 2);
    ASSERT(config.song_columns_list_format.items[0].width == 10);
    ASSERT(config.song_columns_list_format.items[0].stretch_limit == 5);
    ASSERT_EQUAL(config.song_columns_list_format.items[0].name,
                 config.song_columns_list_format.items[0].name_len,
                 "Artist");
    ASSERT(config.song_columns_list_format.items[1].fixed);
    ASSERT(config.song_columns_list_format.items[1].right_alignment);
    ASSERT(!config.song_columns_list_format.items[1].display_empty_tag);
    ASSERT_EQUAL(config.song_columns_list_format.items[1].name,
                 config.song_columns_list_format.items[1].name_len,
                 "Title");
    ASSERT(settings_test_apply(
        apply_song_columns_list_format, &config, "invalid") < 0);
    ASSERT(config.song_columns_list_format.len == 0);

    config_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_cross_field_validation_is_post_parse(void) {
    static char contents[] =
        "visualizer_spectrum_hz_min = 200\n"
        "visualizer_spectrum_hz_max = 100\n";
    Configuration config = {0};
    StringViewArray paths = {0};
    NcmError ncm_error = {0};
    StringView *path_view;
    char path[PATH_MAX];
    int32 contents_len = SIZEOF(contents) - 1;
    int32 fd;
    int32 status;

    fd = cbase_make_temp_file(path, SIZEOF(path),
                              "ncmpcpp2-settings-validate", ".conf");
    ASSERT_NON_NEGATIVE(fd);
    ASSERT_ZERO(XCLOSE(&fd, path));
    ASSERT(write_entire_file(path, contents, contents_len) == contents_len);

    path_view = ncm_string_view_array_append(&paths);
    path_view->data = path;
    path_view->len = strlen32(path);

    config_init(&config);
    status = config_read(&config, &paths, false, true, &ncm_error);
    ASSERT(status < 0);
    ASSERT(config.visualizer_spectrum_hz_min == 200.0);
    ASSERT(config.visualizer_spectrum_hz_max == 100.0);
    ASSERT_CONTAINS(ncm_error.message, ncm_error.message_len,
                    "visualizer_spectrum_hz_max");

    config_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    ASSERT_ZERO(cbase_remove_file(path));
    return;
}

static void
test_duplicate_option_is_rejected(void) {
    static char first_contents[] = "lines_scrolled = 4\n";
    static char second_contents[] = "lines_scrolled = 6\n";
    Configuration config = {0};
    StringViewArray paths = {0};
    NcmError ncm_error = {0};
    StringView *path_view;
    char first_path[PATH_MAX];
    char second_path[PATH_MAX];
    int32 first_len = SIZEOF(first_contents) - 1;
    int32 second_len = SIZEOF(second_contents) - 1;
    int32 fd;
    int32 status;

    fd = cbase_make_temp_file(first_path, SIZEOF(first_path),
                              "ncmpcpp2-settings-a", ".conf");
    ASSERT_NON_NEGATIVE(fd);
    ASSERT_ZERO(XCLOSE(&fd, first_path));
    ASSERT(write_entire_file(first_path, first_contents, first_len)
           == first_len);

    fd = cbase_make_temp_file(second_path, SIZEOF(second_path),
                              "ncmpcpp2-settings-b", ".conf");
    ASSERT_NON_NEGATIVE(fd);
    ASSERT_ZERO(XCLOSE(&fd, second_path));
    ASSERT(write_entire_file(second_path, second_contents, second_len)
           == second_len);

    path_view = ncm_string_view_array_append(&paths);
    path_view->data = first_path;
    path_view->len = strlen32(first_path);
    path_view = ncm_string_view_array_append(&paths);
    path_view->data = second_path;
    path_view->len = strlen32(second_path);

    config_init(&config);
    status = config_read(&config, &paths, false, true, &ncm_error);
    ASSERT(status < 0);
    ASSERT(config.lines_scrolled == 4);
    ASSERT_CONTAINS(ncm_error.message, ncm_error.message_len,
                    "option already set");

    config_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    ASSERT_ZERO(cbase_remove_file(first_path));
    ASSERT_ZERO(cbase_remove_file(second_path));
    return;
}

static void
test_duplicate_state_is_per_read(void) {
    static char contents[] = "lines_scrolled = 4\n";
    Configuration config = {0};
    StringViewArray paths = {0};
    NcmError ncm_error = {0};
    StringView *path_view;
    char path[PATH_MAX];
    int32 contents_len = SIZEOF(contents) - 1;
    int32 fd;

    fd = cbase_make_temp_file(path, SIZEOF(path),
                              "ncmpcpp2-settings-repeat", ".conf");
    ASSERT_NON_NEGATIVE(fd);
    ASSERT_ZERO(XCLOSE(&fd, path));
    ASSERT(write_entire_file(path, contents, contents_len) == contents_len);

    path_view = ncm_string_view_array_append(&paths);
    path_view->data = path;
    path_view->len = strlen32(path);

    config_init(&config);
    ASSERT_ZERO(config_read(&config, &paths, false, true, &ncm_error));
    ASSERT(config.lines_scrolled == 4);
    ncm_error_clear(&ncm_error);
    ASSERT_ZERO(config_read(&config, &paths, false, true, &ncm_error));
    ASSERT(config.lines_scrolled == 4);

    config_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    ASSERT_ZERO(cbase_remove_file(path));
    return;
}


static void
settings_test_tag_name_normalization(char *name, int32 name_len,
                                     char *lower_snake,
                                     char *upper_snake,
                                     char *upper_compact,
                                     char *camel_compact) {
    char buffer[64] = {0};
    int32 len;

    len = ascii_normalize_lower_snake(buffer, name, name_len);
    ASSERT_EQUAL(buffer, len, lower_snake);

    len = ascii_normalize_upper_snake(buffer, name, name_len);
    ASSERT_EQUAL(buffer, len, upper_snake);

    len = ascii_normalize_upper_compact(buffer, name, name_len);
    ASSERT_EQUAL(buffer, len, upper_compact);

    len = ascii_normalize_camel_compact(buffer, name, name_len);
    ASSERT_EQUAL(buffer, len, camel_compact);
    return;
}

static void
test_tag_name_normalization(void) {
    settings_test_tag_name_normalization(STRLIT("Artist"),
                                         "artist", "ARTIST",
                                         "ARTIST", "Artist");
    settings_test_tag_name_normalization(STRLIT("Album Artist"),
                                         "album_artist", "ALBUM_ARTIST",
                                         "ALBUMARTIST", "AlbumArtist");
    settings_test_tag_name_normalization(STRLIT("Disc Subtitle"),
                                         "disc_subtitle", "DISC_SUBTITLE",
                                         "DISCSUBTITLE", "DiscSubtitle");
    settings_test_tag_name_normalization(
        STRLIT("Musicbrainz Release Group Id"),
        "musicbrainz_release_group_id", "MUSICBRAINZ_RELEASE_GROUP_ID",
        "MUSICBRAINZRELEASEGROUPID", "MusicbrainzReleaseGroupId");

    return;
}

static void
test_tag_type_names(void) {
    char *name;
    int32 name_len;

#define TEST_TAG_TYPE_NAME(suffix, display, tag_char, getter_char, flags)    \
    name_len = ncm_tag_type_name_len(CAT(TAG_, suffix), &name);          \
    if (((flags) & TAG_FLAG_DISPLAY) == 0) {                             \
        ASSERT_EQUAL(name, name_len, "");                                     \
        ASSERT_ZERO(name_len);                                                \
    } else {                                                                  \
        ASSERT_EQUAL(name, name_len, TAG_DISPLAY_NAME(display));          \
        ASSERT(name_len == TAG_DISPLAY_NAME_LEN(display));                \
    }                                                                         \
    name_len = ncm_tag_type_canonical_name_len(CAT(TAG_, suffix), &name);\
    ASSERT_EQUAL(name, name_len, TAG_DISPLAY_NAME(display));              \
    ASSERT(name_len == TAG_DISPLAY_NAME_LEN(display));

    TAG_DEFS(TEST_TAG_TYPE_NAME)

#undef TEST_TAG_TYPE_NAME
    return;
}

static void
test_tag_char_and_field_conversions(void) {
#define TEST_FIELD_CONVERSION(suffix, display, tag_char, getter_char,      \
                              flags)                                         \
    ASSERT(ncm_char_to_tag_type(tag_char) == CAT(TAG_, suffix));         \
    ASSERT(ncm_tags_field_from_char(tag_char)                                \
           == CAT(TAGS_FIELD_, suffix));                                 \
    ASSERT(ncm_tags_field_from_tag_type(CAT(TAG_, suffix))               \
           == CAT(TAGS_FIELD_, suffix));                                 \
    ASSERT(ncm_tags_field_to_tag_type(CAT(TAGS_FIELD_, suffix))          \
           == CAT(TAG_, suffix));                                        \
    ASSERT(ncm_tags_field_to_song_getter(CAT(TAGS_FIELD_, suffix))       \
           == CAT(SONG_GETTER_, suffix));                                    \
    ASSERT(ncm_song_getter_to_tags_field(CAT(SONG_GETTER_, suffix))          \
           == CAT(TAGS_FIELD_, suffix));                                 \
    ASSERT(ncm_tags_field_format_char(CAT(TAGS_FIELD_, suffix))          \
           == tag_char);

    TAG_FIELD_DEFS(TEST_FIELD_CONVERSION)

#undef TEST_FIELD_CONVERSION
    ASSERT(ncm_char_to_tag_type('x') == TAG_UNKNOWN);
    ASSERT(ncm_tags_field_from_char('x') == TAGS_FIELD_COUNT);
    ASSERT(ncm_tags_field_from_tag_type(TAG_NAME) == TAGS_FIELD_COUNT);
    ASSERT(ncm_tags_field_to_tag_type(TAGS_FIELD_COUNT)
           == TAG_UNKNOWN);
    ASSERT(ncm_tags_field_to_song_getter(TAGS_FIELD_COUNT)
           == SONG_GETTER_NONE);
    ASSERT(ncm_song_getter_to_tags_field(SONG_GETTER_NONE)
           == TAGS_FIELD_COUNT);
    return;
}

static void
test_song_getter_conversions(void) {
#define TEST_GETTER_CHAR(getter, alias, getter_char)                         \
    if (getter != SONG_GETTER_NONE) {                                        \
        ASSERT(ncm_song_getter_from_char(getter_char) == getter);            \
        ASSERT(ncm_song_getter_format_char(getter) == getter_char);          \
    }
#define TEST_TAG_GETTER_CHAR(suffix, display, tag_char, getter_char,        \
                             flags)                                         \
    ASSERT(ncm_song_getter_from_char(getter_char)                            \
           == CAT(SONG_GETTER_, suffix));                                    \
    ASSERT(ncm_song_getter_format_char(CAT(SONG_GETTER_, suffix))            \
           == getter_char);                                                  \
    ASSERT(ncm_song_getter_to_tag_type(CAT(SONG_GETTER_, suffix))            \
           == CAT(TAG_, suffix));

    NCM_SONG_GETTER_RECORD_LENGTH(TEST_GETTER_CHAR)
    NCM_SONG_GETTER_RECORD_DIRECTORY(TEST_GETTER_CHAR)
    NCM_SONG_GETTER_RECORD_NAME(TEST_GETTER_CHAR)
    NCM_SONG_GETTER_RECORD_URI(TEST_GETTER_CHAR)
    NCM_SONG_GETTER_TAG_HEAD_DEFS(TEST_TAG_GETTER_CHAR)
    NCM_SONG_GETTER_RECORD_TRACK_NUMBER(TEST_GETTER_CHAR)
    NCM_SONG_GETTER_TAG_TAIL_DEFS(TEST_TAG_GETTER_CHAR)
    NCM_SONG_GETTER_RECORD_PRIORITY(TEST_GETTER_CHAR)

#undef TEST_TAG_GETTER_CHAR
#undef TEST_GETTER_CHAR
    ASSERT(ncm_song_getter_from_char('x') == SONG_GETTER_NONE);
    ASSERT(ncm_song_getter_to_tag_type(SONG_GETTER_NONE) == TAG_UNKNOWN);
    ASSERT(ncm_song_getter_to_tag_type(SONG_GETTER_TRACK_NUMBER)
           == TAG_UNKNOWN);
    ASSERT(ncm_song_getter_to_tag_type(SONG_GETTER_PRIORITY)
           == TAG_UNKNOWN);
    return;
}

static void
test_primary_tag_settings_parse(void) {
    char settings_name[TAG_SETTINGS_NAME_CAP];
    enum TagType parsed;
    int32 settings_name_len;

#define TEST_PRIMARY_SETTING(suffix, display, tag_char, getter_char,      \
                             flags)                                         \
    settings_name_len = ncm_tag_type_settings_name_len(                     \
        CAT(TAG_, suffix), settings_name, LENGTH(settings_name));        \
    ASSERT(settings_name_len > 0);                                           \
    parsed = TAG_UNKNOWN;                                                \
    ASSERT(ncm_tag_type_parse_settings_name(settings_name,                   \
                                            settings_name_len, &parsed));    \
    ASSERT(parsed == CAT(TAG_, suffix));                                 \
    parsed = TAG_UNKNOWN;                                                \
    ASSERT_ZERO(settings_parse_mpd_tag(settings_name, settings_name_len,     \
                                       &parsed));                            \
    ASSERT(parsed == CAT(TAG_, suffix));

    TAG_PRIMARY_DEFS(TEST_PRIMARY_SETTING)

#undef TEST_PRIMARY_SETTING
    ASSERT(ncm_tag_type_parse_settings_name(STRLIT("Album Artist"),
                                            &parsed));
    ASSERT(parsed == TAG_ALBUM_ARTIST);
    ASSERT(ncm_tag_type_parse_settings_name(STRLIT("ALBUM_ARTIST"),
                                            &parsed));
    ASSERT(parsed == TAG_ALBUM_ARTIST);
    ASSERT(TAG_parse(STRLIT("ALBUM_ARTIST"))
           == TAG_ALBUM_ARTIST);
    ASSERT(!ncm_tag_type_parse_settings_name(STRLIT("TAG_ARTIST"),
                                              &parsed));
    ASSERT(!ncm_tag_type_parse_settings_name(STRLIT("album"), &parsed));
    ASSERT(settings_parse_mpd_tag(STRLIT("album"), &parsed) < 0);
    return;
}


static void
test_taglib_metadata(void) {
    char buffer[TAGLIB_PROPERTY_CAP];
    int32 len;

    len = ncm_tag_type_taglib_property_len(TAG_ALBUM_ARTIST,
                                           buffer, LENGTH(buffer));
    ASSERT_EQUAL(buffer, len, "ALBUMARTIST");
    len = ncm_tag_type_taglib_name_len(TAG_ALBUM_ARTIST,
                                       buffer, LENGTH(buffer));
    ASSERT_EQUAL(buffer, len, "AlbumArtist");

    len = ncm_tag_type_taglib_property_len(TAG_TRACK,
                                           buffer, LENGTH(buffer));
    ASSERT_EQUAL(buffer, len, "TRACKNUMBER");
    len = ncm_tag_type_taglib_name_len(TAG_TRACK,
                                       buffer, LENGTH(buffer));
    ASSERT_EQUAL(buffer, len, "Track");

    len = ncm_tags_field_taglib_property_len(TAGS_FIELD_DISC,
                                             buffer, LENGTH(buffer));
    ASSERT_EQUAL(buffer, len, "DISCNUMBER");

    ASSERT(ncm_tag_type_taglib_property_len(TAG_NAME,
                                            buffer, LENGTH(buffer)) < 0);
    ASSERT(ncm_tags_field_taglib_property_len(TAGS_FIELD_COUNT,
                                              buffer, LENGTH(buffer)) < 0);
    return;
}

static void
test_search_constraint_metadata(void) {
    SearchConstraintMetadata *metadata;
    int32 idx = 1;

    metadata = search_constraint_metadata(0);
    ASSERT(metadata->tag == TAG_UNKNOWN);
    ASSERT_EQUAL(metadata->name, metadata->name_len, "Any");
    ASSERT(metadata->name_len == strlen32(metadata->name));

#define TEST_SEARCH_CONSTRAINT(suffix, display, tag_char, getter_char,     \
                               flags)                                       \
    metadata = search_constraint_metadata(idx);                              \
    ASSERT(metadata->tag == CAT(TAG_, suffix));                          \
    ASSERT_EQUAL(metadata->name, metadata->name_len,                         \
                 TAG_DISPLAY_NAME(display));                             \
    ASSERT(metadata->name_len == strlen32(metadata->name));                  \
    idx += 1;

    TAG_SEARCH_DEFS(TEST_SEARCH_CONSTRAINT)

#undef TEST_SEARCH_CONSTRAINT
    ASSERT(idx == SEARCH_ENGINE_CONSTRAINT_COUNT);
    return;
}

static void
test_song_info_tag_metadata(void) {
    int32 idx = 0;

#define TEST_SONG_INFO_TAG(suffix, display, tag_char, getter_char, flags) \
    ASSERT_EQUAL(ncm_song_info_tags[idx].name,                               \
                 ncm_song_info_tags[idx].name_len,                           \
                 TAG_DISPLAY_NAME(display));                             \
    ASSERT(ncm_song_info_tags[idx].name_len                                  \
           == TAG_DISPLAY_NAME_LEN(display));                            \
    ASSERT(ncm_song_info_tags[idx].field == CAT(TAGS_FIELD_, suffix));   \
    ASSERT(ncm_song_info_tags[idx].get == CAT(SONG_GETTER_, suffix));        \
    idx += 1;

    TAG_SONG_INFO_DEFS(TEST_SONG_INFO_TAG)

#undef TEST_SONG_INFO_TAG
    ASSERT(idx == NCM_SONG_INFO_TAG_COUNT);
    ASSERT(ncm_song_info_tags[idx].name == NULL);
    ASSERT(ncm_song_info_tags[idx].name_len == 0);
    ASSERT(ncm_song_info_tags[idx].field == TAGS_FIELD_COUNT);
    ASSERT(ncm_song_info_tags[idx].get == SONG_GETTER_NONE);
    return;
}

static void
test_tag_edit_parser_metadata(void) {
    StrBuilder legend = {0};
    char *name;
    int32 name_len;
    int32 idx = 0;

#define TEST_PARSER_FIELD(suffix, display, tag_char, getter_char, flags)  \
    ASSERT(ncm_tags_field_format_char(CAT(TAGS_FIELD_, suffix))          \
           == tag_char);                                                     \
    name_len = ncm_tags_field_parser_name_len(CAT(TAGS_FIELD_, suffix),  \
                                              &name);                        \
    ASSERT(name_len > 0);                                                     \
    tag_edit_append_parser_legend_field(&legend,                             \
                                        CAT(TAGS_FIELD_, suffix));       \
    idx += 1;

    TAG_EDIT_PARSER_DEFS(TEST_PARSER_FIELD)

#undef TEST_PARSER_FIELD
    ASSERT(idx == TAG_EDIT_PARSER_COUNT);
    ASSERT_EQUAL(legend.data, legend.len,
                 "%a - artist\n"
                 "%A - album artist\n"
                 "%t - title\n"
                 "%b - album\n"
                 "%y - date\n"
                 "%n - track number\n"
                 "%g - genre\n"
                 "%c - composer\n"
                 "%p - performer\n"
                 "%d - disc\n"
                 "%C - comment\n");
    sb_free(&legend);
    return;
}

int
main(void) {
    global_state_init();
    config_init(&Config);

    test_tag_name_normalization();
    test_tag_type_names();
    test_tag_char_and_field_conversions();
    test_song_getter_conversions();
    test_primary_tag_settings_parse();
    test_taglib_metadata();
    test_search_constraint_metadata();
    test_song_info_tag_metadata();
    test_tag_edit_parser_metadata();

    test_generated_option_identity();
    test_each_declared_default();
    test_option_table_shape();
    test_declared_defaults_and_cleanup();
    test_runtime_application_is_separate();
    test_config_options_apply_runtime_precedence();
    test_generated_numeric_boundaries();
    test_numeric_boundaries();
    test_enum_options();
    test_optional_enum_options();
    test_color_options();
    test_format_options();
    test_buffer_and_look_options();
    test_collection_options();
    test_remaining_generated_options();
    test_cross_field_validation_is_post_parse();
    test_duplicate_option_is_rejected();
    test_duplicate_state_is_per_read();

    config_destroy(&Config);
    global_state_destroy();
    return 0;
}
