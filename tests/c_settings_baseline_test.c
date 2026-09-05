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
    int32 len;

    len = SNPRINTF(value, "%d", minimum);
    ASSERT(len > 0);
    ASSERT_ZERO(settings_test_apply(apply, config, value));
    ASSERT(*field == minimum);

    if (minimum != INT32_MIN) {
        len = SNPRINTF(value, "%d", minimum - 1);
        ASSERT(len > 0);
        ASSERT(settings_test_apply(apply, config, value) < 0);
        ASSERT(*field == minimum);
    }

    len = SNPRINTF(value, "%d", maximum);
    ASSERT(len > 0);
    ASSERT_ZERO(settings_test_apply(apply, config, value));
    ASSERT(*field == maximum);

    if (maximum != INT32_MAX) {
        len = SNPRINTF(value, "%d", maximum + 1);
        ASSERT(len > 0);
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
    int32 len;

    if (minimum != -HUGE_VAL) {
        len = SNPRINTF(value, "%.17g", minimum);
        ASSERT(len > 0);
        ASSERT_ZERO(settings_test_apply(apply, config, value));
        ASSERT(*field == minimum);

        outside = nextafter(minimum, -HUGE_VAL);
        len = SNPRINTF(value, "%.17g", outside);
        ASSERT(len > 0);
        ASSERT(settings_test_apply(apply, config, value) < 0);
        ASSERT(*field == minimum);
    }

    if (maximum != HUGE_VAL) {
        len = SNPRINTF(value, "%.17g", maximum);
        ASSERT(len > 0);
        ASSERT_ZERO(settings_test_apply(apply, config, value));
        ASSERT(*field == maximum);

        outside = nextafter(maximum, HUGE_VAL);
        len = SNPRINTF(value, "%.17g", outside);
        ASSERT(len > 0);
        ASSERT(settings_test_apply(apply, config, value) < 0);
        ASSERT(*field == maximum);
    }
    return;
}

static void
settings_assert_generated_empty(Configuration *config) {
#define XX_BOOL(NAME, DEFAULT_VALUE) ASSERT(!config->NAME);
#define XX_STRING(NAME, DEFAULT_VALUE) \
    ASSERT(config->NAME == NULL); \
    ASSERT(config->NAME##_len == 0);
#define XX_PATH(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_DIR(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_INT_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) \
    ASSERT(config->NAME == 0);
#define XX_DOUBLE_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) \
    ASSERT(config->NAME == 0);
#define XX_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER) \
    ASSERT(config->NAME == (C_TYPE)0);
#define XX_OPTIONAL_ENUM( \
    NAME, C_TYPE, DEFAULT_VALUE, PARSER, PRESENT_FIELD, UNSET_VALUE \
) \
    ASSERT(config->NAME == (C_TYPE)(UNSET_VALUE)); \
    ASSERT(!config->PRESENT_FIELD);
#define XX_COLOR(NAME, DEFAULT_VALUE) \
    ASSERT(nc_color_is_default(config->NAME));
#define XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE) \
    ASSERT(config->NAME.formats == NULL); \
    ASSERT(nc_color_is_default(config->NAME.color));
#define XX_BORDER(NAME, DEFAULT_VALUE) \
    ASSERT(!config->NAME.enabled); \
    ASSERT(nc_color_is_default(config->NAME.color));
#define XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS) \
    ASSERT(config->NAME.root.items == NULL); \
    ASSERT(config->NAME.root.len == 0); \
    ASSERT(config->NAME.root.cap == 0);
#define XX_BUFFER(NAME, DEFAULT_VALUE, KEEP_EXISTING) \
    ASSERT(config->NAME.data == NULL); \
    ASSERT(config->NAME.properties == NULL); \
    ASSERT(config->NAME.len == 0); \
    ASSERT(config->NAME.cap == 0);
#define XX_BUFFER_WIDTH(NAME, DEFAULT_VALUE, KEEP_EXISTING) \
    XX_BUFFER(NAME, DEFAULT_VALUE, KEEP_EXISTING) \
    ASSERT(config->NAME##_length == 0);
#define XX_LOOK(NAME, DEFAULT_VALUE, MIN_CHARS, MAX_CHARS, PAD_TO_MAX) \
    ASSERT(config->NAME.data == NULL); \
    ASSERT(config->NAME.len == 0); \
    ASSERT(config->NAME.cap == 0);
#define XX_RATIO(NAME, DEFAULT_VALUE, EXPECTED_LEN) \
    ASSERT(config->NAME.items == NULL); \
    ASSERT(config->NAME.len == 0); \
    ASSERT(config->NAME.cap == 0);
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT_VALUE) \
    ASSERT(config->NAME.items == NULL); \
    ASSERT(config->NAME.len == 0); \
    ASSERT(config->NAME.cap == 0);
#define XX_LYRICS_FETCHERS(NAME, DEFAULT_VALUE) \
    ASSERT(config->NAME.fetchers.items == NULL); \
    ASSERT(config->NAME.fetchers.len == 0); \
    ASSERT(config->NAME.fetchers.cap == 0);
#define XX_SCREEN_LIST(NAME, DEFAULT_VALUE, PREVIOUS_FIELD) \
    ASSERT(config->NAME.items == NULL); \
    ASSERT(config->NAME.len == 0); \
    ASSERT(config->NAME.cap == 0); \
    ASSERT(!config->PREVIOUS_FIELD);
#define XX_NAMED_BOOL(NAME, DEFAULT_VALUE, TRUE_VALUE, FALSE_VALUE) \
    ASSERT(!config->NAME);
#define XX_UINT32_CHOICE(NAME, DEFAULT_VALUE, PARSER, UNSET_VALUE) \
    ASSERT(config->NAME == (UNSET_VALUE));
#define XX_COLUMNS(NAME, DEFAULT_VALUE, FORMAT_FIELD) \
    ASSERT(config->FORMAT_FIELD.root.items == NULL); \
    ASSERT(config->FORMAT_FIELD.root.len == 0); \
    ASSERT(config->FORMAT_FIELD.root.cap == 0); \
    ASSERT(config->NAME.items == NULL); \
    ASSERT(config->NAME.len == 0); \
    ASSERT(config->NAME.cap == 0);
#include "config_options_pass.h"

    return;
}

static void
test_generated_option_identity(void) {
#define XX_OPTION(NAME, DEFAULT_VALUE, ...) \
    ASSERT(STREQUAL( \
        ncmpcpp_options[SETTINGS_OPTION_##NAME].name, \
        ncmpcpp_options[SETTINGS_OPTION_##NAME].name_len, #NAME)); \
    ASSERT(STREQUAL( \
        ncmpcpp_options[SETTINGS_OPTION_##NAME].default_value, \
        ncmpcpp_options[SETTINGS_OPTION_##NAME].default_value_len, \
        DEFAULT_VALUE)); \
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

        configuration_init(&config);
        status = option.apply(&config, option.default_value,
                              option.default_value_len, &ncm_error);
        ASSERT_ZERO(status);
        configuration_destroy(&config);
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
    NcmStringViewArray paths = {0};
    NcmError ncm_error = {0};

    configuration_init(&config);
    settings_assert_generated_empty(&config);
    ASSERT_ZERO(configuration_read(&config, &paths, false, true, &ncm_error));

    ASSERT(config.mpd_port == 6600);
    ASSERT(config.visualizer_fps == 60);
    ASSERT(config.visualizer_spectrum_gain == 10.0);
    ASSERT(config.visualizer_spectrum_hz_min == 20.0);
    ASSERT(config.visualizer_spectrum_hz_max == 20000.0);
    ASSERT(config.locked_screen_width_part == 50.0);
    ASSERT(config.search_engine_default_search_mode == 1);
    ASSERT(config.screen_switcher_mode.len == 2);
    ASSERT(!config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == NCM_SCREEN_TYPE_COUNT);
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
    ASSERT(config.playlist_editor_display_mode == NCM_DISPLAY_MODE_CLASSIC);
    ASSERT(config.user_interface == NCM_DESIGN_CLASSIC);
    ASSERT(config.media_library_primary_tag == MPD_TAG_ARTIST);
    ASSERT(config.space_add_mode == NCM_SPACE_ADD_MODE_ADD_REMOVE);
    ASSERT(config.startup_screen == NCM_SCREEN_TYPE_PLAYLIST);

    configuration_destroy(&config);
    settings_assert_generated_empty(&config);
    ASSERT(config.ncmpcpp_directory == NULL);
    ASSERT(config.ncmpcpp_directory_len == 0);
    ASSERT(config.visualizer_color.items == NULL);
    ASSERT(config.screen_switcher_mode.items == NULL);
    ASSERT(config.lyrics_fetchers.fetchers.items == NULL);

    configuration_destroy(&config);
    return;
}

static void
test_runtime_application_is_separate(void) {
    Configuration config = {0};
    NcmStringViewArray paths = {0};
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

    configuration_init(&config);

    ASSERT_ZERO(ncm_mpd_client_set_hostname(
        &global_mpd, STRLIT("before-host"), &ncm_error));
    ncm_mpd_client_set_port(&global_mpd, 1234);
    ASSERT_ZERO(ncm_mpd_client_set_password(
        &global_mpd, STRLIT("before-password"), &ncm_error));
    ASSERT_ZERO(ncm_mpd_client_set_timeout_ms(
        &global_mpd, 4321, &ncm_error));

    ASSERT_ZERO(configuration_read(&config, &paths, false, true, &ncm_error));
    ASSERT(config.enable_window_title);
    ASSERT(STREQUAL(global_mpd.host.data, global_mpd.host.len,
                    "before-host"));
    ASSERT(global_mpd.port == 1234);
    ASSERT(STREQUAL(global_mpd.password.data, global_mpd.password.len,
                    "before-password"));
    ASSERT(global_mpd.timeout_ms == 4321);

    ASSERT_ZERO(settings_test_apply(
        apply_mpd_host, &config, "after-host"));
    ASSERT_ZERO(settings_test_apply(apply_mpd_port, &config, "4567"));
    ASSERT_ZERO(settings_test_apply(
        apply_mpd_password, &config, "after-password"));
    ASSERT_ZERO(settings_test_apply(
        apply_mpd_connection_timeout, &config, "9"));
    ASSERT_ZERO(settings_test_apply(
        apply_enable_window_title, &config, "yes"));

    ASSERT(config.enable_window_title);
    ASSERT(STREQUAL(global_mpd.host.data, global_mpd.host.len,
                    "before-host"));
    ASSERT(global_mpd.port == 1234);
    ASSERT(STREQUAL(global_mpd.password.data, global_mpd.password.len,
                    "before-password"));
    ASSERT(global_mpd.timeout_ms == 4321);

    ASSERT_ZERO(configuration_apply_runtime(
        &config, &global_mpd, true, &ncm_error));
    ASSERT(config.enable_window_title);
    ASSERT(!window_title_enabled);
    ASSERT(STREQUAL(global_mpd.host.data, global_mpd.host.len, "after-host"));
    ASSERT(global_mpd.port == 4567);
    ASSERT(STREQUAL(global_mpd.password.data, global_mpd.password.len,
                    "after-password"));
    ASSERT(global_mpd.timeout_ms == 9000);

    if (had_term) {
        ASSERT_ZERO(setenv("TERM", sb_opt_cstr(&previous_term), 1));
    } else {
        ASSERT_ZERO(unsetenv("TERM"));
    }
    sb_free(&previous_term);
    configuration_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    return;
}

static void
test_configuration_options_apply_runtime_precedence(void) {
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

    ncm_configuration_options_init(&options);
    options.quiet = true;
    config_path = str_builder_array_append(&options.config_paths);
    SB_APPEND(config_path, path, strlen32(path));

    ASSERT_ZERO(ncm_configuration_options_apply(&options, &ncm_error));
    ASSERT(STREQUAL(global_mpd.host.data, global_mpd.host.len, "config-host"));
    ASSERT(global_mpd.port == 1111);
    ASSERT(STREQUAL(global_mpd.password.data, global_mpd.password.len,
                    "config-password"));
    ASSERT(global_mpd.timeout_ms == 7000);
    ASSERT(STREQUAL(Config.mpd_host, Config.mpd_host_len, "config-host"));
    ASSERT(Config.mpd_port == 1111);
    ASSERT(STREQUAL(Config.mpd_password, Config.mpd_password_len,
                    "config-password"));
    ASSERT(Config.mpd_connection_timeout == 7);
    ASSERT(!window_title_enabled);

    ASSERT_ZERO(setenv("MPD_HOST", "env-host", 1));
    ASSERT_ZERO(setenv("MPD_PORT", "2222", 1));
    ncm_error_clear(&ncm_error);
    ASSERT_ZERO(ncm_configuration_options_apply(&options, &ncm_error));
    ASSERT(STREQUAL(global_mpd.host.data, global_mpd.host.len, "env-host"));
    ASSERT(global_mpd.port == 2222);
    ASSERT(STREQUAL(global_mpd.password.data, global_mpd.password.len,
                    "config-password"));
    ASSERT(global_mpd.timeout_ms == 7000);
    ASSERT(STREQUAL(Config.mpd_host, Config.mpd_host_len, "config-host"));
    ASSERT(Config.mpd_port == 1111);
    ASSERT(STREQUAL(Config.mpd_password, Config.mpd_password_len,
                    "config-password"));
    ASSERT(Config.mpd_connection_timeout == 7);

    sb_clear(&options.host);
    SB_APPEND(&options.host, "cli-host");
    options.host_provided = true;
    options.port = 3333;
    options.port_provided = true;
    ncm_error_clear(&ncm_error);
    ASSERT_ZERO(ncm_configuration_options_apply(&options, &ncm_error));
    ASSERT(STREQUAL(global_mpd.host.data, global_mpd.host.len, "cli-host"));
    ASSERT(global_mpd.port == 3333);
    ASSERT(STREQUAL(global_mpd.password.data, global_mpd.password.len,
                    "config-password"));
    ASSERT(global_mpd.timeout_ms == 7000);
    ASSERT(STREQUAL(Config.mpd_host, Config.mpd_host_len, "config-host"));
    ASSERT(Config.mpd_port == 1111);
    ASSERT(STREQUAL(Config.mpd_password, Config.mpd_password_len,
                    "config-password"));
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
    ncm_configuration_options_destroy(&options);
    ASSERT_ZERO(cbase_remove_file(path));
    return;
}

static void
test_generated_numeric_boundaries(void) {
    Configuration config = {0};

    configuration_init(&config);

#define XX_INT_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) \
    settings_test_int_range(apply_##NAME, &config, &config.NAME, \
                            MINIMUM, MAXIMUM);
#define XX_DOUBLE_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) \
    settings_test_double_range(apply_##NAME, &config, &config.NAME, \
                               MINIMUM, MAXIMUM);
#include "config_options_pass.h"

    configuration_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_numeric_boundaries(void) {
    Configuration config = {0};
    NcmError ncm_error = {0};

    configuration_init(&config);

    ASSERT_ZERO(settings_test_apply(apply_mpd_port, &config, "-1"));
    ASSERT(config.mpd_port == -1);

    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_spectrum_hz_min, &config, "20"));
    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_spectrum_hz_max, &config, "20"));
    ASSERT(configuration_validate(&config, &ncm_error) < 0);
    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_spectrum_hz_max, &config, "21"));
    ncm_error_clear(&ncm_error);
    ASSERT_ZERO(configuration_validate(&config, &ncm_error));

    ASSERT_ZERO(settings_test_apply(
        apply_locked_screen_width_part, &config, "20"));
    ASSERT(config.locked_screen_width_part == 20.0);
    ASSERT(configuration_locked_screen_width_fraction(&config) == 0.2);
    ASSERT_ZERO(settings_test_apply(
        apply_locked_screen_width_part, &config, "80"));
    ASSERT(config.locked_screen_width_part == 80.0);
    ASSERT(configuration_locked_screen_width_fraction(&config) == 0.8);

    ASSERT_ZERO(settings_test_apply(
        apply_search_engine_default_search_mode, &config, "1"));
    ASSERT(config.search_engine_default_search_mode == 1);
    ASSERT(configuration_search_engine_default_mode(&config)
           == SEARCH_ENGINE_SEARCH_MODE_LITERAL);
    ASSERT_ZERO(settings_test_apply(
        apply_search_engine_default_search_mode, &config, "3"));
    ASSERT(config.search_engine_default_search_mode == 3);
    ASSERT(configuration_search_engine_default_mode(&config)
           == SEARCH_ENGINE_SEARCH_MODE_EXACT);
    ASSERT(settings_test_apply(
        apply_search_engine_default_search_mode, &config, "4") < 0);
    ASSERT(config.search_engine_default_search_mode == 3);

    ASSERT_ZERO(settings_test_apply(apply_system_encoding, &config, "UTF-8"));
    ASSERT(STREQUAL(config.system_encoding, config.system_encoding_len,
                    "UTF-8"));

    configuration_destroy(&config);
    return;
}

static void
test_enum_options(void) {
    Configuration config = {0};

    configuration_init(&config);

    ASSERT_ZERO(settings_test_apply(
        apply_browser_sort_mode, &config, "noop"));
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
    ASSERT(config.media_library_primary_tag == MPD_TAG_PERFORMER);
    ASSERT(settings_test_apply(
        apply_media_library_primary_tag, &config, "invalid") < 0);
    ASSERT(config.media_library_primary_tag == MPD_TAG_PERFORMER);

    ASSERT_ZERO(settings_test_apply(
        apply_startup_screen, &config, "playlist"));
    ASSERT(config.startup_screen == NCM_SCREEN_TYPE_PLAYLIST);

    configuration_destroy(&config);
    return;
}

static void
test_optional_enum_options(void) {
    Configuration config = {0};

    configuration_init(&config);

    ASSERT(!config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == NCM_SCREEN_TYPE_COUNT);
    ASSERT_ZERO(settings_test_apply(
        apply_startup_slave_screen, &config, "browser"));
    ASSERT(config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == NCM_SCREEN_TYPE_BROWSER);

    ASSERT(settings_test_apply(
        apply_startup_slave_screen, &config, "invalid") < 0);
    ASSERT(config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == NCM_SCREEN_TYPE_COUNT);

    ASSERT_ZERO(settings_test_apply(
        apply_startup_slave_screen, &config, ""));
    ASSERT(!config.has_startup_slave_screen_type);
    ASSERT(config.startup_slave_screen == NCM_SCREEN_TYPE_COUNT);

    configuration_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_color_options(void) {
    Configuration config = {0};
    NcColor expected;

    configuration_init(&config);

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
    ASSERT(nc_formatted_color_format_count(&config.state_flags_color) == 2);
    ASSERT(config.state_flags_color.formats[0] == NC_FORMAT_BOLD);
    ASSERT(config.state_flags_color.formats[1] == NC_FORMAT_UNDERLINE);
    ASSERT(settings_test_apply(
        apply_state_flags_color, &config, "green:x") < 0);
    ASSERT(nc_formatted_color_format_count(&config.state_flags_color) == 2);

    ASSERT_ZERO(settings_test_apply(
        apply_window_border_color, &config, "cyan"));
    expected = nc_color_make(COLOR_CYAN, NC_COLOR_CURRENT, false, false);
    ASSERT(config.window_border_color.enabled);
    ASSERT(nc_color_is_equal(config.window_border_color.color, expected));
    ASSERT(settings_test_apply(
        apply_window_border_color, &config, "invalid") < 0);
    ASSERT(config.window_border_color.enabled);
    ASSERT(nc_color_is_equal(config.window_border_color.color, expected));

    configuration_destroy(&config);
    return;
}

static void
test_format_options(void) {
    Configuration config = {0};

    configuration_init(&config);

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

    configuration_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_buffer_and_look_options(void) {
    Configuration config = {0};

    configuration_init(&config);

    ASSERT_ZERO(settings_test_apply(
        apply_selected_item_prefix, &config, "selected"));
    ASSERT(STREQUAL(config.selected_item_prefix.data,
                    config.selected_item_prefix.len, "selected"));
    ASSERT(config.selected_item_prefix_length == 8);

    ASSERT_ZERO(settings_test_apply(
        apply_current_item_prefix, &config, "first"));
    ASSERT_ZERO(settings_test_apply(
        apply_current_item_prefix, &config, "second"));
    ASSERT(STREQUAL(config.current_item_prefix.data,
                    config.current_item_prefix.len, "first"));
    ASSERT(config.current_item_prefix_length == 5);

    ASSERT_ZERO(settings_test_apply(
        apply_browser_playlist_prefix, &config, "playlist "));
    ASSERT(STREQUAL(config.browser_playlist_prefix.data,
                    config.browser_playlist_prefix.len, "playlist "));

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

    configuration_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_collection_options(void) {
    Configuration config = {0};

    configuration_init(&config);

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
           == NCM_SCREEN_TYPE_PLAYLIST);
    ASSERT(config.screen_switcher_mode.items[1]
           == NCM_SCREEN_TYPE_BROWSER);
    ASSERT_ZERO(settings_test_apply(
        apply_screen_switcher_mode, &config, "previous"));
    ASSERT(config.screen_switcher_previous);
    ASSERT(config.screen_switcher_mode.len == 0);

    configuration_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_remaining_generated_options(void) {
    Configuration config = {0};

    configuration_init(&config);

    ASSERT_ZERO(settings_test_apply(
        apply_default_place_to_search_in, &config, "database"));
    ASSERT(config.default_place_to_search_in);
    ASSERT_ZERO(settings_test_apply(
        apply_default_place_to_search_in, &config, "playlist"));
    ASSERT(!config.default_place_to_search_in);
    ASSERT(settings_test_apply(
        apply_default_place_to_search_in, &config, "invalid") < 0);
    ASSERT(!config.default_place_to_search_in);

    ASSERT_ZERO(settings_test_apply(
        apply_default_find_mode, &config, "wrapped"));
    ASSERT(config.default_find_mode);
    ASSERT_ZERO(settings_test_apply(
        apply_default_find_mode, &config, "normal"));
    ASSERT(!config.default_find_mode);
    ASSERT(settings_test_apply(
        apply_default_find_mode, &config, "invalid") < 0);
    ASSERT(!config.default_find_mode);

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
    ASSERT(STREQUAL(config.song_columns_list_format.items[0].name,
                    config.song_columns_list_format.items[0].name_len,
                    "Artist"));
    ASSERT(config.song_columns_list_format.items[1].fixed);
    ASSERT(config.song_columns_list_format.items[1].right_alignment);
    ASSERT(!config.song_columns_list_format.items[1].display_empty_tag);
    ASSERT(STREQUAL(config.song_columns_list_format.items[1].name,
                    config.song_columns_list_format.items[1].name_len,
                    "Title"));
    ASSERT(settings_test_apply(
        apply_song_columns_list_format, &config, "invalid") < 0);
    ASSERT(config.song_columns_list_format.len == 0);

    configuration_destroy(&config);
    settings_assert_generated_empty(&config);
    return;
}

static void
test_cross_field_validation_is_post_parse(void) {
    static char contents[] =
        "visualizer_spectrum_hz_min = 200\n"
        "visualizer_spectrum_hz_max = 100\n";
    Configuration config = {0};
    NcmStringViewArray paths = {0};
    NcmError ncm_error = {0};
    NcmStringView *path_view;
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

    configuration_init(&config);
    status = configuration_read(&config, &paths, false, true, &ncm_error);
    ASSERT(status < 0);
    ASSERT(config.visualizer_spectrum_hz_min == 200.0);
    ASSERT(config.visualizer_spectrum_hz_max == 100.0);
    ASSERT_CONTAINS(ncm_error.message, strlen32(ncm_error.message),
                    "visualizer_spectrum_hz_max");

    configuration_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    ASSERT_ZERO(cbase_remove_file(path));
    return;
}

static void
test_duplicate_option_is_rejected(void) {
    static char first_contents[] = "lines_scrolled = 4\n";
    static char second_contents[] = "lines_scrolled = 6\n";
    Configuration config = {0};
    NcmStringViewArray paths = {0};
    NcmError ncm_error = {0};
    NcmStringView *path_view;
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

    configuration_init(&config);
    status = configuration_read(&config, &paths, false, true, &ncm_error);
    ASSERT(status < 0);
    ASSERT(config.lines_scrolled == 4);
    ASSERT_CONTAINS(ncm_error.message, strlen32(ncm_error.message),
                    "option already set");

    configuration_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    ASSERT_ZERO(cbase_remove_file(first_path));
    ASSERT_ZERO(cbase_remove_file(second_path));
    return;
}

static void
test_duplicate_state_is_per_read(void) {
    static char contents[] = "lines_scrolled = 4\n";
    Configuration config = {0};
    NcmStringViewArray paths = {0};
    NcmError ncm_error = {0};
    NcmStringView *path_view;
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

    configuration_init(&config);
    ASSERT_ZERO(configuration_read(&config, &paths, false, true, &ncm_error));
    ASSERT(config.lines_scrolled == 4);
    ncm_error_clear(&ncm_error);
    ASSERT_ZERO(configuration_read(&config, &paths, false, true, &ncm_error));
    ASSERT(config.lines_scrolled == 4);

    configuration_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    ASSERT_ZERO(cbase_remove_file(path));
    return;
}

int
main(void) {
    global_state_init();
    configuration_init(&Config);

    test_generated_option_identity();
    test_each_declared_default();
    test_option_table_shape();
    test_declared_defaults_and_cleanup();
    test_runtime_application_is_separate();
    test_configuration_options_apply_runtime_precedence();
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

    configuration_destroy(&Config);
    global_state_destroy();
    return 0;
}
