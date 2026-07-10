#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "configuration.h"
#include "global.h"
#include "settings.h"
#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "c/ncm_fs.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_option_parser.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

#define REQUIRE(COND) do {                                                \
    if (!(COND)) {                                                        \
        fail(__FILE__, __LINE__, (char *)#COND);                          \
    }                                                                     \
} while (0)

#define REQUIRE_INT(ACTUAL, EXPECTED)                                     \
    require_int(__FILE__, __LINE__, (char *)#ACTUAL, ACTUAL, EXPECTED)

#define REQUIRE_STRING(ACTUAL, ACTUAL_LEN, EXPECTED)                      \
    require_string(__FILE__, __LINE__, (char *)#ACTUAL,                   \
                   ACTUAL, ACTUAL_LEN, LIT_ARGS(EXPECTED))

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static void make_path(NcmBuffer *path, char *left, char *right);
static void write_file(char *path, int32 path_len, char *data,
                       int32 data_len);
static char *copy_string(char *string, int32 string_len);
static void test_option_parser(void);
static void test_expand_home(void);
static void test_command_line_parsing(void);
static void test_config_path_discovery(void);
static void test_config_defaults(void);
static void test_config_file_parsing(void);
static void test_regular_expression_flavors(void);
static void test_configuration_options_apply(void);
static void test_invalid_config_value(void);

static void
fail(char *file, int32 line, char *condition) {
    fprintf(stderr, "%s:%d: requirement failed: %s\n",
            file, line, condition);
    exit(EXIT_FAILURE);
}

static void
require_int(char *file, int32 line, char *name,
            int32 actual, int32 expected) {
    if (actual != expected) {
        fprintf(stderr, "%s:%d: %s: expected %d, got %d\n",
                file, line, name, expected, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
require_string(char *file, int32 line, char *name,
               char *actual, int32 actual_len,
               char *expected, int32 expected_len) {
    if (!ncm_string_equal(actual, actual_len, expected, expected_len)) {
        fprintf(stderr, "%s:%d: %s: expected '%.*s', got '%.*s'\n",
                file, line, name,
                expected_len, expected, actual_len, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
make_path(NcmBuffer *path, char *left, char *right) {
    ncm_buffer_clear(path);
    REQUIRE(ncm_fs_join(path, left, (int32)strlen(left),
                        right, (int32)strlen(right)));
    return;
}

static void
write_file(char *path, int32 path_len, char *data, int32 data_len) {
    NcmBuffer path_buffer;
    FILE *file;

    ncm_buffer_init(&path_buffer);
    ncm_buffer_append(&path_buffer, path, path_len);
    file = fopen(path_buffer.data, "w");
    REQUIRE(file != NULL);
    REQUIRE(fwrite(data, 1, (size_t)data_len, file) == (size_t)data_len);
    REQUIRE(fclose(file) == 0);
    ncm_buffer_destroy(&path_buffer);
    return;
}

static char *
copy_string(char *string, int32 string_len) {
    char *copy;

    copy = ncm_malloc(string_len + 1);
    ncm_memcpy(copy, string, string_len);
    copy[string_len] = '\0';
    return copy;
}

static void
test_option_parser(void) {
    char line[] = "  playlist_show_remaining_time = yes # comment";
    char quoted[] = "external_editor = \"nano -w\" # comment";
    NcmOptionLine parsed;
    bool flag;

    REQUIRE(ncm_option_parser_parse_line(line, (int32)strlen(line),
                                         &parsed));
    REQUIRE_STRING(parsed.option, parsed.option_len,
                   "playlist_show_remaining_time");
    REQUIRE_STRING(parsed.value, parsed.value_len, "yes");
    REQUIRE(ncm_option_parser_yes_no(parsed.value, parsed.value_len,
                                     &flag));
    REQUIRE(flag);

    REQUIRE(ncm_option_parser_parse_line(quoted, (int32)strlen(quoted),
                                         &parsed));
    REQUIRE_STRING(parsed.option, parsed.option_len, "external_editor");
    REQUIRE_STRING(parsed.value, parsed.value_len, "nano -w");
    REQUIRE(!ncm_option_parser_parse_line(LIT_ARGS("# only comment"),
                                          &parsed));
    return;
}


static void
test_command_line_parsing(void) {
    char *argv[] = {
        (char *)"ncmpcpp",
        (char *)"-qv",
        (char *)"-hexample.test",
        (char *)"-p",
        (char *)"6611",
        (char *)"--current-song=%a - %t",
        (char *)"--ignore-config-errors",
        (char *)"--screen",
        (char *)"browser",
        (char *)"--slave-screen=playlist",
        (char *)"-c/tmp/config",
        (char *)"--bindings",
        (char *)"/tmp/bindings",
    };
    char *bad_argv[] = {
        (char *)"ncmpcpp",
        (char *)"--port",
        (char *)"bad",
        (char *)"-c/tmp/config",
        (char *)"-b/tmp/bindings",
    };
    NcmConfigurationOptions options;
    NcmError error;

    ncm_error_clear(&error);
    ncm_configuration_options_init(&options);
    REQUIRE(ncm_configuration_options_parse(&options,
                                            NCM_ARRAY_LEN(argv),
                                            argv, &error));
    REQUIRE(options.quiet);
    REQUIRE(options.version);
    REQUIRE(options.current_song);
    REQUIRE(options.ignore_config_errors);
    REQUIRE(options.screen);
    REQUIRE(options.slave_screen);
    REQUIRE(options.host_provided);
    REQUIRE(options.port_provided);
    REQUIRE_INT((int32)options.port, 6611);
    REQUIRE_STRING(options.host.data, options.host.len, "example.test");
    REQUIRE_STRING(options.current_song_format.data,
                   options.current_song_format.len, "%a - %t");
    REQUIRE_STRING(options.screen_name.data, options.screen_name.len,
                   "browser");
    REQUIRE_STRING(options.slave_screen_name.data,
                   options.slave_screen_name.len, "playlist");
    REQUIRE_INT(options.config_paths.len, 1);
    REQUIRE_INT(options.bindings_paths.len, 1);
    REQUIRE_STRING(options.config_paths.items[0].data,
                   options.config_paths.items[0].len, "/tmp/config");
    REQUIRE_STRING(options.bindings_paths.items[0].data,
                   options.bindings_paths.items[0].len, "/tmp/bindings");
    ncm_configuration_options_destroy(&options);

    ncm_error_clear(&error);
    ncm_configuration_options_init(&options);
    REQUIRE(!ncm_configuration_options_parse(&options,
                                             NCM_ARRAY_LEN(bad_argv),
                                             bad_argv, &error));
    REQUIRE(ncm_error_is_set(&error));
    ncm_configuration_options_destroy(&options);
    return;
}

static void
test_expand_home(void) {
    char template[] = "/tmp/ncmpcpp-home-XXXXXX";
    char *home;
    char *path;
    int32 path_len;
    NcmError error;

    home = mkdtemp(template);
    REQUIRE(home != NULL);
    REQUIRE(setenv("HOME", home, 1) == 0);

    ncm_error_clear(&error);
    path = copy_string(LIT_ARGS("~/.ncmpcpp/config"));
    path_len = STRLIT_LEN("~/.ncmpcpp/config");
    REQUIRE(expand_home(&path, &path_len, &error));
    REQUIRE(ncm_string_starts_with(path, path_len,
                                   home, (int32)strlen(home)));
    ncm_free(path, path_len + 1);
    return;
}

static void
test_config_path_discovery(void) {
    char template[] = "/tmp/ncmpcpp-paths-XXXXXX";
    char *root;
    NcmBuffer home;
    NcmBuffer xdg;
    NcmBuffer legacy_dir;
    NcmBuffer legacy_config;
    NcmBuffer legacy_bindings;
    NcmBufferArray config_paths;
    NcmBufferArray bindings_paths;
    NcmError error;

    root = mkdtemp(template);
    REQUIRE(root != NULL);

    ncm_buffer_init(&home);
    ncm_buffer_init(&xdg);
    ncm_buffer_init(&legacy_dir);
    ncm_buffer_init(&legacy_config);
    ncm_buffer_init(&legacy_bindings);
    ncm_buffer_array_init(&config_paths);
    ncm_buffer_array_init(&bindings_paths);
    ncm_error_clear(&error);

    make_path(&home, root, "home");
    make_path(&xdg, root, "xdg");
    make_path(&legacy_dir, home.data, ".ncmpcpp");
    REQUIRE(ncm_fs_mkdir_all(home.data, home.len, &error));
    REQUIRE(ncm_fs_mkdir_all(xdg.data, xdg.len, &error));
    REQUIRE(ncm_fs_mkdir_all(legacy_dir.data, legacy_dir.len, &error));
    REQUIRE(setenv("HOME", home.data, 1) == 0);
    REQUIRE(setenv("XDG_CONFIG_HOME", xdg.data, 1) == 0);

    make_path(&legacy_config, legacy_dir.data, "config");
    make_path(&legacy_bindings, legacy_dir.data, "bindings");
    write_file(legacy_config.data, legacy_config.len, LIT_ARGS("# old\n"));
    write_file(legacy_bindings.data, legacy_bindings.len, LIT_ARGS("# old\n"));

    REQUIRE(configuration_discover_default_paths(&config_paths,
                                                 &bindings_paths,
                                                 &error));
    REQUIRE_INT(config_paths.len, 2);
    REQUIRE_INT(bindings_paths.len, 2);
    REQUIRE(ncm_string_ends_with(config_paths.items[0].data,
                                 config_paths.items[0].len,
                                 LIT_ARGS("xdg/ncmpcpp/config")));
    REQUIRE(ncm_string_ends_with(bindings_paths.items[0].data,
                                 bindings_paths.items[0].len,
                                 LIT_ARGS("xdg/ncmpcpp/bindings")));
    REQUIRE(ncm_string_equal(config_paths.items[1].data,
                             config_paths.items[1].len,
                             legacy_config.data, legacy_config.len));
    REQUIRE(ncm_fs_is_directory(config_paths.items[0].data,
                                config_paths.items[0].len
                                - STRLIT_LEN("/config")));

    ncm_buffer_array_destroy(&bindings_paths);
    ncm_buffer_array_destroy(&config_paths);
    ncm_buffer_destroy(&legacy_bindings);
    ncm_buffer_destroy(&legacy_config);
    ncm_buffer_destroy(&legacy_dir);
    ncm_buffer_destroy(&xdg);
    ncm_buffer_destroy(&home);
    return;
}

static void
test_config_defaults(void) {
    char template[] = "/tmp/ncmpcpp-defaults-XXXXXX";
    char *home;
    NcmStringViewArray paths;
    Configuration config;
    NcmError error;

    home = mkdtemp(template);
    REQUIRE(home != NULL);
    REQUIRE(setenv("HOME", home, 1) == 0);
    ncm_error_clear(&error);
    ncm_string_view_array_init(&paths);
    configuration_init(&config);
    global_state_init();

    REQUIRE(configuration_read(&config, &paths, false, &error));
    REQUIRE_INT((int32)config.playlist_disable_highlight_delay_seconds, 5);
    REQUIRE_INT((int32)ncm_mpd_client_port(&global_mpd), 6600);
    REQUIRE(ncm_string_starts_with(config.mpd_music_dir,
                                   config.mpd_music_dir_len,
                                   home, (int32)strlen(home)));
    REQUIRE(config.lyrics_fetchers.fetchers.len > 0);
    REQUIRE_INT((int32)config.regex_type,
                (int32)NCM_REGEX_EXTENDED_CASE_INSENSITIVE);

    global_state_destroy();
    configuration_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    return;
}

static void
test_config_file_parsing(void) {
    char template[] = "/tmp/ncmpcpp-config-XXXXXX";
    char *root;
    NcmBuffer path;
    NcmStringViewArray paths;
    NcmStringView *view;
    Configuration config;
    NcmError error;
    char data[] =
        "mpd_host = \"example.test\"\n"
        "mpd_port = 6611\n"
        "playlist_show_remaining_time = yes\n"
        "startup_screen = browser\n";

    root = mkdtemp(template);
    REQUIRE(root != NULL);
    REQUIRE(setenv("HOME", root, 1) == 0);
    ncm_buffer_init(&path);
    ncm_string_view_array_init(&paths);
    configuration_init(&config);
    global_state_init();
    ncm_error_clear(&error);

    make_path(&path, root, "config");
    write_file(path.data, path.len, data, (int32)strlen(data));
    view = ncm_string_view_array_append(&paths);
    REQUIRE(view != NULL);
    view->data = path.data;
    view->len = path.len;

    REQUIRE(configuration_read(&config, &paths, false, &error));
    REQUIRE_STRING(ncm_mpd_client_hostname(&global_mpd),
                   (int32)strlen(ncm_mpd_client_hostname(&global_mpd)),
                   "example.test");
    REQUIRE_INT((int32)ncm_mpd_client_port(&global_mpd), 6611);
    REQUIRE(config.playlist_show_remaining_time);
    REQUIRE(config.startup_screen_type == NCM_SCREEN_TYPE_BROWSER);

    global_state_destroy();
    configuration_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    ncm_buffer_destroy(&path);
    return;
}


static void
test_regular_expression_flavors(void) {
    char *values[] = {
        (char *)"none",
        (char *)"basic",
        (char *)"extended",
    };
    uint32 expected[] = {
        NCM_REGEX_LITERAL_CASE_INSENSITIVE,
        NCM_REGEX_BASIC_CASE_INSENSITIVE,
        NCM_REGEX_EXTENDED_CASE_INSENSITIVE,
    };
    char template[] = "/tmp/ncmpcpp-regex-flavors-XXXXXX";
    char *root;
    NcmBuffer path;
    NcmBuffer data;
    NcmStringViewArray paths;
    NcmStringView *view;
    Configuration config;
    NcmError error;

    root = mkdtemp(template);
    REQUIRE(root != NULL);
    REQUIRE(setenv("HOME", root, 1) == 0);
    ncm_buffer_init(&path);
    ncm_buffer_init(&data);
    configuration_init(&config);
    global_state_init();
    make_path(&path, root, "config");

    for (int32 i = 0; i < NCM_ARRAY_LEN(values); i += 1) {
        ncm_buffer_clear(&data);
        ncm_buffer_append(&data,
                          STRLIT_ARGS("regular_expressions = "));
        ncm_buffer_append(&data, values[i],
                          (int32)strlen(values[i]));
        ncm_buffer_append(&data, STRLIT_ARGS("\n"));
        write_file(path.data, path.len, data.data, data.len);

        ncm_string_view_array_init(&paths);
        view = ncm_string_view_array_append(&paths);
        REQUIRE(view != NULL);
        view->data = path.data;
        view->len = path.len;
        ncm_error_clear(&error);

        REQUIRE(configuration_read(&config, &paths, false, &error));
        REQUIRE_INT((int32)config.regex_type, (int32)expected[i]);

        configuration_clear(&config);
        ncm_string_view_array_destroy(&paths);
    }

    ncm_buffer_clear(&data);
    ncm_buffer_append(&data,
                      STRLIT_ARGS("regular_expressions = perl\n"));
    write_file(path.data, path.len, data.data, data.len);

    ncm_string_view_array_init(&paths);
    view = ncm_string_view_array_append(&paths);
    REQUIRE(view != NULL);
    view->data = path.data;
    view->len = path.len;
    ncm_error_clear(&error);

    REQUIRE(!configuration_read(&config, &paths, false, &error));
    REQUIRE(ncm_error_is_set(&error));
    REQUIRE(ncm_string_starts_with(
        error.message, (int32)strlen(error.message),
        LIT_ARGS("error while processing option \"regular_expressions\"")));

    global_state_destroy();
    configuration_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    ncm_buffer_destroy(&data);
    ncm_buffer_destroy(&path);
    return;
}


static void
test_configuration_options_apply(void) {
    char template[] = "/tmp/ncmpcpp-apply-XXXXXX";
    char *root;
    char *argv[12];
    NcmBuffer config_path;
    NcmBuffer bindings_path;
    NcmBuffer app_dir;
    NcmBuffer lyrics_dir;
    NcmBuffer data;
    NcmConfigurationOptions options;
    NcmError error;

    root = mkdtemp(template);
    REQUIRE(root != NULL);
    REQUIRE(setenv("HOME", root, 1) == 0);
    REQUIRE(unsetenv("MPD_HOST") == 0);
    REQUIRE(unsetenv("MPD_PORT") == 0);

    ncm_buffer_init(&config_path);
    ncm_buffer_init(&bindings_path);
    ncm_buffer_init(&app_dir);
    ncm_buffer_init(&lyrics_dir);
    ncm_buffer_init(&data);
    ncm_configuration_options_init(&options);
    ncm_error_clear(&error);
    global_state_init();
    configuration_init(&Config);

    make_path(&config_path, root, "config");
    make_path(&bindings_path, root, "bindings");
    make_path(&app_dir, root, "app");
    make_path(&lyrics_dir, root, "lyrics");

    ncm_buffer_append(&data, STRLIT_ARGS("ncmpcpp_directory = \""));
    ncm_buffer_append(&data, app_dir.data, app_dir.len);
    ncm_buffer_append(&data, STRLIT_ARGS("\"\nlyrics_directory = \""));
    ncm_buffer_append(&data, lyrics_dir.data, lyrics_dir.len);
    ncm_buffer_append(&data, STRLIT_ARGS("\"\nmpd_host = config.test\n"));
    ncm_buffer_append(&data, STRLIT_ARGS("mpd_port = 6612\n"));
    write_file(config_path.data, config_path.len, data.data, data.len);
    write_file(bindings_path.data, bindings_path.len, LIT_ARGS("# ok\n"));

    argv[0] = (char *)"ncmpcpp";
    argv[1] = (char *)"-c";
    argv[2] = config_path.data;
    argv[3] = (char *)"-b";
    argv[4] = bindings_path.data;
    argv[5] = (char *)"-hcli.test";
    argv[6] = (char *)"-p";
    argv[7] = (char *)"6620";
    argv[8] = (char *)"--screen";
    argv[9] = (char *)"browser";
    argv[10] = (char *)"--slave-screen=playlist";
    argv[11] = NULL;

    REQUIRE(ncm_configuration_options_parse(&options, 11, argv, &error));
    REQUIRE(ncm_configuration_options_apply(&options, &error));
    REQUIRE_STRING(ncm_mpd_client_hostname(&global_mpd),
                   (int32)strlen(ncm_mpd_client_hostname(&global_mpd)),
                   "cli.test");
    REQUIRE_INT((int32)ncm_mpd_client_port(&global_mpd), 6620);
    REQUIRE_INT((int32)ncm_mpd_client_timeout_ms(&global_mpd), 5000);
    REQUIRE(Config.startup_screen_type == NCM_SCREEN_TYPE_BROWSER);
    REQUIRE(Config.has_startup_slave_screen_type);
    REQUIRE(Config.startup_slave_screen_type == NCM_SCREEN_TYPE_PLAYLIST);
    REQUIRE(ncm_fs_is_directory(app_dir.data, app_dir.len));
    REQUIRE(ncm_fs_is_directory(lyrics_dir.data, lyrics_dir.len));

    configuration_destroy(&Config);
    global_state_destroy();
    ncm_configuration_options_destroy(&options);
    ncm_buffer_destroy(&data);
    ncm_buffer_destroy(&lyrics_dir);
    ncm_buffer_destroy(&app_dir);
    ncm_buffer_destroy(&bindings_path);
    ncm_buffer_destroy(&config_path);
    return;
}

static void
test_invalid_config_value(void) {
    char template[] = "/tmp/ncmpcpp-invalid-XXXXXX";
    char *root;
    NcmBuffer path;
    NcmStringViewArray paths;
    NcmStringView *view;
    Configuration config;
    NcmError error;
    char data[] = "volume_change_step = no\n";

    root = mkdtemp(template);
    REQUIRE(root != NULL);
    REQUIRE(setenv("HOME", root, 1) == 0);
    ncm_buffer_init(&path);
    ncm_string_view_array_init(&paths);
    configuration_init(&config);
    global_state_init();
    ncm_error_clear(&error);

    make_path(&path, root, "bad-config");
    write_file(path.data, path.len, data, (int32)strlen(data));
    view = ncm_string_view_array_append(&paths);
    REQUIRE(view != NULL);
    view->data = path.data;
    view->len = path.len;

    REQUIRE(!configuration_read(&config, &paths, false, &error));
    REQUIRE(ncm_error_is_set(&error));
    REQUIRE(ncm_string_starts_with(error.message,
                                   (int32)strlen(error.message),
                                   LIT_ARGS("error while processing option")));

    global_state_destroy();
    configuration_destroy(&config);
    ncm_string_view_array_destroy(&paths);
    ncm_buffer_destroy(&path);
    return;
}

int
main(void) {
    test_option_parser();
    test_command_line_parsing();
    test_expand_home();
    test_config_path_discovery();
    test_config_defaults();
    test_config_file_parsing();
    test_regular_expression_flavors();
    test_configuration_options_apply();
    test_invalid_config_value();
    return EXIT_SUCCESS;
}
