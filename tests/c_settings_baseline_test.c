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
test_option_table_shape(void) {
    ASSERT(LENGTH(ncmpcpp_options) == 134);

    for (int32 i = 0; i < LENGTH(ncmpcpp_options); i += 1) {
        SettingsOption left = ncmpcpp_options[i];

        ASSERT(left.name != NULL);
        ASSERT(left.default_value != NULL);
        ASSERT(left.apply != NULL);
        ASSERT(left.name_len == strlen32(left.name));
        ASSERT(left.default_value_len == strlen32(left.default_value));
        for (int32 j = i + 1; j < LENGTH(ncmpcpp_options); j += 1) {
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
    ASSERT_ZERO(configuration_read(&config, &paths, false, true, &ncm_error));

    ASSERT(config.mpd_port == 6600);
    ASSERT(config.visualizer_fps == 60);
    ASSERT(config.visualizer_spectrum_gain == 10.0);
    ASSERT(config.visualizer_spectrum_hz_min == 20.0);
    ASSERT(config.visualizer_spectrum_hz_max == 20000.0);
    ASSERT(config.locked_screen_width_part == 0.5);
    ASSERT(config.search_engine_default_search_mode == 0);
    ASSERT(config.screen_switcher_mode.len == 2);
    ASSERT(!config.has_startup_slave_screen_type);
    ASSERT(config.regular_expressions
           == NCM_REGEX_EXTENDED_CASE_INSENSITIVE);

    configuration_destroy(&config);
    ASSERT(config.ncmpcpp_directory == NULL);
    ASSERT(config.ncmpcpp_directory_len == 0);
    ASSERT(config.progressbar_look.data == NULL);
    ASSERT(config.current_item_prefix.data == NULL);
    ASSERT(config.visualizer_color.items == NULL);
    ASSERT(config.screen_switcher_mode.items == NULL);
    ASSERT(config.lyrics_fetchers.fetchers.items == NULL);

    configuration_destroy(&config);
    return;
}

static void
test_numeric_boundaries(void) {
    Configuration config = {0};

    configuration_init(&config);

    ASSERT_ZERO(settings_test_apply(apply_mpd_port, &config, "-1"));
    ASSERT(config.mpd_port == -1);
    ASSERT_ZERO(settings_test_apply(apply_mpd_port, &config, "65535"));
    ASSERT(settings_test_apply(apply_mpd_port, &config, "65536") < 0);

    ASSERT(settings_test_apply(apply_visualizer_fps, &config, "29") < 0);
    ASSERT_ZERO(settings_test_apply(apply_visualizer_fps, &config, "30"));
    ASSERT_ZERO(settings_test_apply(apply_visualizer_fps, &config, "1000"));
    ASSERT(settings_test_apply(apply_visualizer_fps, &config, "1001") < 0);

    ASSERT(settings_test_apply(
        apply_visualizer_spectrum_dft_size, &config, "0") < 0);
    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_spectrum_dft_size, &config, "1"));
    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_spectrum_dft_size, &config, "5"));
    ASSERT(settings_test_apply(
        apply_visualizer_spectrum_dft_size, &config, "6") < 0);

    ASSERT(settings_test_apply(
        apply_visualizer_spectrum_gain, &config, "-0.1") < 0);
    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_spectrum_gain, &config, "0"));
    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_spectrum_gain, &config, "100"));
    ASSERT(settings_test_apply(
        apply_visualizer_spectrum_gain, &config, "100.1") < 0);

    ASSERT(settings_test_apply(
        apply_visualizer_spectrum_hz_min, &config, "0") < 0);
    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_spectrum_hz_min, &config, "20"));
    ASSERT(settings_test_apply(
        apply_visualizer_spectrum_hz_max, &config, "20") < 0);
    ASSERT_ZERO(settings_test_apply(
        apply_visualizer_spectrum_hz_max, &config, "21"));

    ASSERT(settings_test_apply(
        apply_search_engine_default_search_mode, &config, "0") < 0);
    ASSERT_ZERO(settings_test_apply(
        apply_search_engine_default_search_mode, &config, "1"));
    ASSERT(config.search_engine_default_search_mode == 0);
    ASSERT_ZERO(settings_test_apply(
        apply_search_engine_default_search_mode, &config, "3"));
    ASSERT(config.search_engine_default_search_mode == 2);
    ASSERT(settings_test_apply(
        apply_search_engine_default_search_mode, &config, "4") < 0);

    configuration_destroy(&config);
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

    test_option_table_shape();
    test_declared_defaults_and_cleanup();
    test_numeric_boundaries();
    test_duplicate_option_is_rejected();
    test_duplicate_state_is_per_read();

    global_state_destroy();
    return 0;
}
