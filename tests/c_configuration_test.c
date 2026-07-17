#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "bindings.h"
#include "configuration.h"
#include "global.h"
#include "settings.h"
#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "c/ncm_fs.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_option_parser.h"
#include "c/ncm_path.h"
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

#define REQUIRE_CONTAINS(ACTUAL, EXPECTED)                               \
    require_contains(__FILE__, __LINE__, (char *)#ACTUAL,                 \
                     ACTUAL, (char *)EXPECTED)

typedef struct CapturedConfigureResult {
    NcmBuffer output;
    NcmBuffer error_output;
    int32 exit_status;
} CapturedConfigureResult;

static void __attribute__((noreturn))
fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static void require_contains(char *file, int32 line, char *name,
                             char *actual, char *expected);
static void captured_configure_result_init(CapturedConfigureResult *result);
static void captured_configure_result_destroy(CapturedConfigureResult *result);
static void read_stream(FILE *stream, NcmBuffer *buffer);
static void run_configure_capture(int32 argc, char **argv,
                                  CapturedConfigureResult *result);
static void require_parse_failure(int32 argc, char **argv,
                                  char *expected_message);
static void make_path(NcmBuffer *path, char *left, char *right);
static void write_file(char *path, int32 path_len, char *data,
                       int32 data_len);
static void test_option_parser(void);
static void test_expand_home(void);
static void test_command_line_short_options(void);
static void test_command_line_long_options(void);
static void test_command_line_current_song(void);
static void test_command_line_errors(void);
static void test_config_path_discovery(void);
static void test_early_exit_modes(void);
static void test_quiet_mode(void);
static void test_config_defaults(void);
static void test_config_file_parsing(void);
static void test_regular_expression_flavors(void);
static void test_configuration_options_apply(void);
static void test_invalid_config_value(void);

static void __attribute__((noreturn))
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
require_contains(char *file, int32 line, char *name,
                 char *actual, char *expected) {
    if ((actual == NULL) || (strstr(actual, expected) == NULL)) {
        fprintf(stderr, "%s:%d: %s does not contain '%s'\n",
                file, line, name, expected);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
captured_configure_result_init(CapturedConfigureResult *result) {
    ncm_buffer_init(&result->output);
    ncm_buffer_init(&result->error_output);
    result->exit_status = -1;
    return;
}

static void
captured_configure_result_destroy(CapturedConfigureResult *result) {
    ncm_buffer_destroy(&result->error_output);
    ncm_buffer_destroy(&result->output);
    result->exit_status = -1;
    return;
}

static void
read_stream(FILE *stream, NcmBuffer *buffer) {
    char chunk[512];
    size_t count;

    ncm_buffer_clear(buffer);
    REQUIRE(fseek(stream, 0, SEEK_SET) == 0);
    while ((count = fread(chunk, 1, sizeof(chunk), stream)) > 0) {
        ncm_buffer_append(buffer, chunk, (int32)count);
    }
    REQUIRE(!ferror(stream));
    return;
}

static void
run_configure_capture(int32 argc, char **argv,
                      CapturedConfigureResult *result) {
    FILE *output;
    FILE *error_output;
    pid_t child;
    int32 status;

    output = tmpfile();
    error_output = tmpfile();
    REQUIRE(output != NULL);
    REQUIRE(error_output != NULL);
    REQUIRE(fflush(NULL) == 0);

    switch (child = fork()) {
    case -1:
        fail(__FILE__, __LINE__, "fork() >= 0");
        break;
    case 0:
        if ((dup2(fileno(output), STDOUT_FILENO) < 0)
            || (dup2(fileno(error_output), STDERR_FILENO) < 0)) {
            _exit(127);
        }
        fclose(output);
        fclose(error_output);
        (void)configure(argc, argv);
        fflush(NULL);
        _exit(EXIT_SUCCESS);
    default:
        REQUIRE(waitpid(child, &status, 0) == child);
        break;
    }

    if (WIFEXITED(status)) {
        result->exit_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result->exit_status = 128 + WTERMSIG(status);
    } else {
        result->exit_status = 127;
    }
    read_stream(output, &result->output);
    read_stream(error_output, &result->error_output);
    REQUIRE(fclose(error_output) == 0);
    REQUIRE(fclose(output) == 0);
    return;
}

static void
require_parse_failure(int32 argc, char **argv, char *expected_message) {
    NcmConfigurationOptions options;
    NcmError error;

    ncm_configuration_options_init(&options);
    ncm_error_clear(&error);
    REQUIRE(!ncm_configuration_options_parse(&options, argc, argv, &error));
    REQUIRE(ncm_error_is_set(&error));
    REQUIRE_CONTAINS(error.message, expected_message);
    ncm_configuration_options_destroy(&options);
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
test_command_line_short_options(void) {
    char *argv[] = {
        "ncmpcpp",
        "-?vq",
        "-h",
        "short.test",
        "-p65535",
        "-cfirst-config",
        "-c",
        "second-config",
        "-bfirst-bindings",
        "-b",
        "second-bindings",
        "-sbrowser",
        "-S",
        "playlist",
    };
    NcmConfigurationOptions options;
    NcmError error;

    ncm_configuration_options_init(&options);
    ncm_error_clear(&error);
    REQUIRE(ncm_configuration_options_parse(&options,
                                            NCM_ARRAY_LEN(argv),
                                            argv, &error));
    REQUIRE(options.help);
    REQUIRE(options.version);
    REQUIRE(options.quiet);
    REQUIRE(options.host_provided);
    REQUIRE(options.port_provided);
    REQUIRE(options.screen);
    REQUIRE(options.slave_screen);
    REQUIRE_INT((int32)options.port, 65535);
    REQUIRE_STRING(options.host.data, options.host.len, "short.test");
    REQUIRE_STRING(options.screen_name.data, options.screen_name.len,
                   "browser");
    REQUIRE_STRING(options.slave_screen_name.data,
                   options.slave_screen_name.len, "playlist");
    REQUIRE_INT(options.config_paths.len, 2);
    REQUIRE_STRING(options.config_paths.items[0].data,
                   options.config_paths.items[0].len, "first-config");
    REQUIRE_STRING(options.config_paths.items[1].data,
                   options.config_paths.items[1].len, "second-config");
    REQUIRE_INT(options.bindings_paths.len, 2);
    REQUIRE_STRING(options.bindings_paths.items[0].data,
                   options.bindings_paths.items[0].len, "first-bindings");
    REQUIRE_STRING(options.bindings_paths.items[1].data,
                   options.bindings_paths.items[1].len, "second-bindings");
    ncm_configuration_options_destroy(&options);
    return;
}

static void
test_command_line_long_options(void) {
    char *argv[] = {
        "ncmpcpp",
        "--host=long.test",
        "--port",
        "0",
        "--current-song=%f",
        "--config=first-config",
        "--config",
        "second-config",
        "--ignore-config-errors",
        "--test-lyrics-fetchers",
        "--bindings=first-bindings",
        "--bindings",
        "second-bindings",
        "--screen=browser",
        "--slave-screen",
        "playlist",
        "--help",
        "--version",
        "--quiet",
    };
    NcmConfigurationOptions options;
    NcmError error;

    ncm_configuration_options_init(&options);
    ncm_error_clear(&error);
    REQUIRE(ncm_configuration_options_parse(&options,
                                            NCM_ARRAY_LEN(argv),
                                            argv, &error));
    REQUIRE(options.host_provided);
    REQUIRE(options.port_provided);
    REQUIRE(options.current_song);
    REQUIRE(options.ignore_config_errors);
    REQUIRE(options.test_lyrics_fetchers);
    REQUIRE(options.screen);
    REQUIRE(options.slave_screen);
    REQUIRE(options.help);
    REQUIRE(options.version);
    REQUIRE(options.quiet);
    REQUIRE_INT((int32)options.port, 0);
    REQUIRE_STRING(options.host.data, options.host.len, "long.test");
    REQUIRE_STRING(options.current_song_format.data,
                   options.current_song_format.len, "%f");
    REQUIRE_INT(options.config_paths.len, 2);
    REQUIRE_INT(options.bindings_paths.len, 2);
    REQUIRE_STRING(options.screen_name.data, options.screen_name.len,
                   "browser");
    REQUIRE_STRING(options.slave_screen_name.data,
                   options.slave_screen_name.len, "playlist");
    ncm_configuration_options_destroy(&options);
    return;
}

static void
test_command_line_current_song(void) {
    char *default_argv[] = {
        "ncmpcpp",
        "--current-song",
        "--quiet",
        "-cconfig",
        "-bbindings",
    };
    char *separate_argv[] = {
        "ncmpcpp",
        "--current-song",
        "%a - %t",
        "-cconfig",
        "-bbindings",
    };
    char *empty_argv[] = {
        "ncmpcpp",
        "--current-song=",
        "-cconfig",
        "-bbindings",
    };
    NcmConfigurationOptions options;
    NcmError error;

    ncm_configuration_options_init(&options);
    ncm_error_clear(&error);
    REQUIRE(ncm_configuration_options_parse(&options,
                                            NCM_ARRAY_LEN(default_argv),
                                            default_argv, &error));
    REQUIRE(options.current_song);
    REQUIRE(options.quiet);
    REQUIRE_STRING(options.current_song_format.data,
                   options.current_song_format.len,
                   "{{{(%l) }{{%a - }%t}}|{%f}}");
    ncm_configuration_options_destroy(&options);

    ncm_configuration_options_init(&options);
    ncm_error_clear(&error);
    REQUIRE(ncm_configuration_options_parse(&options,
                                            NCM_ARRAY_LEN(separate_argv),
                                            separate_argv, &error));
    REQUIRE_STRING(options.current_song_format.data,
                   options.current_song_format.len, "%a - %t");
    ncm_configuration_options_destroy(&options);

    ncm_configuration_options_init(&options);
    ncm_error_clear(&error);
    REQUIRE(ncm_configuration_options_parse(&options,
                                            NCM_ARRAY_LEN(empty_argv),
                                            empty_argv, &error));
    REQUIRE_INT(options.current_song_format.len, 0);
    ncm_configuration_options_destroy(&options);
    return;
}

static void
test_command_line_errors(void) {
    char *missing_short_value[] = {
        "ncmpcpp",
        "-h",
    };
    char *missing_long_value[] = {
        "ncmpcpp",
        "--config",
    };
    char *invalid_port[] = {
        "ncmpcpp",
        "--port=bad",
    };
    char *negative_port[] = {
        "ncmpcpp",
        "--port=-1",
    };
    char *large_port[] = {
        "ncmpcpp",
        "--port=65536",
    };
    char *flag_with_value[] = {
        "ncmpcpp",
        "--quiet=yes",
    };
    char *unknown_short[] = {
        "ncmpcpp",
        "-x",
    };
    char *unknown_long[] = {
        "ncmpcpp",
        "--unknown",
    };
    char *mixed_short_bundle[] = {
        "ncmpcpp",
        "-qvhserver",
    };
    char *positional[] = {
        "ncmpcpp",
        "song",
    };
    char *after_delimiter[] = {
        "ncmpcpp",
        "--",
        "song",
    };
    char *delimiter[] = {
        "ncmpcpp",
        "--",
    };
    NcmConfigurationOptions options;
    NcmError error;

    require_parse_failure(NCM_ARRAY_LEN(missing_short_value),
                          missing_short_value, "requires an argument");
    require_parse_failure(NCM_ARRAY_LEN(missing_long_value),
                          missing_long_value, "requires an argument");
    require_parse_failure(NCM_ARRAY_LEN(invalid_port), invalid_port,
                          "is invalid");
    require_parse_failure(NCM_ARRAY_LEN(negative_port), negative_port,
                          "is invalid");
    require_parse_failure(NCM_ARRAY_LEN(large_port), large_port,
                          "between 0 and 65535");
    require_parse_failure(NCM_ARRAY_LEN(flag_with_value), flag_with_value,
                          "does not take an argument");
    require_parse_failure(NCM_ARRAY_LEN(unknown_short), unknown_short,
                          "unrecognized option '-x'");
    require_parse_failure(NCM_ARRAY_LEN(unknown_long), unknown_long,
                          "unrecognized option '--unknown'");
    require_parse_failure(NCM_ARRAY_LEN(mixed_short_bundle),
                          mixed_short_bundle, "unrecognized option '-q'");
    require_parse_failure(NCM_ARRAY_LEN(positional), positional,
                          "unexpected positional argument");
    require_parse_failure(NCM_ARRAY_LEN(after_delimiter), after_delimiter,
                          "unexpected positional argument");

    ncm_configuration_options_init(&options);
    ncm_error_clear(&error);
    REQUIRE(ncm_configuration_options_parse(&options,
                                            NCM_ARRAY_LEN(delimiter),
                                            delimiter, &error));
    REQUIRE_INT(options.config_paths.len, 2);
    REQUIRE_INT(options.bindings_paths.len, 2);
    ncm_configuration_options_destroy(&options);
    return;
}

static void
test_expand_home(void) {
    char template[] = "/tmp/ncmpcpp-home-XXXXXX";
    char *home;
    NcmBuffer path;
    NcmBuffer expected;
    NcmError error;

    home = mkdtemp(template);
    REQUIRE(home != NULL);
    REQUIRE(setenv("HOME", home, 1) == 0);

    ncm_buffer_init(&path);
    ncm_buffer_init(&expected);
    ncm_buffer_append(&path, LIT_ARGS("~/.ncmpcpp/config"));
    ncm_error_clear(&error);
    REQUIRE(ncm_path_expand_home(&path, &error));
    REQUIRE(ncm_string_starts_with(path.data, path.len,
                                   home, (int32)strlen(home)));

    ncm_buffer_clear(&path);
    ncm_buffer_append(&path, LIT_ARGS("user@~/music"));
    ncm_error_clear(&error);
    REQUIRE(ncm_path_expand_home(&path, &error));
    ncm_buffer_append(&expected, LIT_ARGS("user@"));
    ncm_buffer_append(&expected, home, (int32)strlen(home));
    ncm_buffer_append(&expected, LIT_ARGS("/music"));
    REQUIRE(ncm_string_equal(path.data, path.len,
                             expected.data, expected.len));

    REQUIRE(unsetenv("HOME") == 0);
    ncm_buffer_clear(&path);
    ncm_buffer_append(&path, LIT_ARGS("/tmp/config"));
    ncm_error_clear(&error);
    REQUIRE(ncm_path_expand_home(&path, &error));
    REQUIRE_STRING(path.data, path.len, "/tmp/config");

    ncm_buffer_clear(&path);
    ncm_buffer_append(&path, LIT_ARGS("~/.ncmpcpp/config"));
    ncm_error_clear(&error);
    REQUIRE(!ncm_path_expand_home(&path, &error));
    REQUIRE_CONTAINS(error.message, "HOME environment variable is not set");

    REQUIRE(setenv("HOME", "", 1) == 0);
    ncm_error_clear(&error);
    REQUIRE(!ncm_path_expand_home(&path, &error));
    REQUIRE_CONTAINS(error.message, "HOME environment variable is not set");

    ncm_error_clear(&error);
    REQUIRE(!ncm_path_expand_home(NULL, &error));
    REQUIRE_CONTAINS(error.message, "missing path buffer");

    ncm_buffer_destroy(&expected);
    ncm_buffer_destroy(&path);
    return;
}

static void
test_config_path_discovery(void) {
    char template[] = "/tmp/ncmpcpp-paths-XXXXXX";
    char *root;
    NcmBuffer home;
    NcmBuffer xdg;
    NcmBuffer expected;
    NcmBufferArray config_paths;
    NcmBufferArray bindings_paths;
    NcmError error;

    root = mkdtemp(template);
    REQUIRE(root != NULL);
    ncm_buffer_init(&home);
    ncm_buffer_init(&xdg);
    ncm_buffer_init(&expected);
    make_path(&home, root, "home");
    make_path(&xdg, root, "xdg");
    REQUIRE(setenv("HOME", home.data, 1) == 0);
    REQUIRE(setenv("XDG_CONFIG_HOME", xdg.data, 1) == 0);

    ncm_buffer_array_init(&config_paths);
    ncm_buffer_array_init(&bindings_paths);
    ncm_error_clear(&error);
    REQUIRE(configuration_discover_default_paths(&config_paths,
                                                 &bindings_paths,
                                                 &error));
    REQUIRE_INT(config_paths.len, 2);
    REQUIRE_INT(bindings_paths.len, 2);
    make_path(&expected, xdg.data, "ncmpcpp/config");
    REQUIRE(ncm_string_equal(config_paths.items[0].data,
                             config_paths.items[0].len,
                             expected.data, expected.len));
    make_path(&expected, xdg.data, "ncmpcpp/bindings");
    REQUIRE(ncm_string_equal(bindings_paths.items[0].data,
                             bindings_paths.items[0].len,
                             expected.data, expected.len));
    REQUIRE_STRING(config_paths.items[1].data,
                   config_paths.items[1].len, "~/.ncmpcpp/config");
    REQUIRE_STRING(bindings_paths.items[1].data,
                   bindings_paths.items[1].len, "~/.ncmpcpp/bindings");
    REQUIRE(!ncm_fs_exists(xdg.data, xdg.len));
    REQUIRE(!ncm_fs_exists(home.data, home.len));
    ncm_buffer_array_destroy(&bindings_paths);
    ncm_buffer_array_destroy(&config_paths);

    REQUIRE(unsetenv("HOME") == 0);
    for (int32 i = 0; i < 2; i += 1) {
        if (i == 0) {
            REQUIRE(unsetenv("XDG_CONFIG_HOME") == 0);
        } else {
            REQUIRE(setenv("XDG_CONFIG_HOME", "", 1) == 0);
        }

        ncm_buffer_array_init(&config_paths);
        ncm_buffer_array_init(&bindings_paths);
        ncm_error_clear(&error);
        REQUIRE(configuration_discover_default_paths(&config_paths,
                                                     &bindings_paths,
                                                     &error));
        REQUIRE_STRING(config_paths.items[0].data,
                       config_paths.items[0].len,
                       "~/.config/ncmpcpp/config");
        REQUIRE_STRING(config_paths.items[1].data,
                       config_paths.items[1].len,
                       "~/.ncmpcpp/config");
        REQUIRE_STRING(bindings_paths.items[0].data,
                       bindings_paths.items[0].len,
                       "~/.config/ncmpcpp/bindings");
        REQUIRE_STRING(bindings_paths.items[1].data,
                       bindings_paths.items[1].len,
                       "~/.ncmpcpp/bindings");
        ncm_buffer_array_destroy(&bindings_paths);
        ncm_buffer_array_destroy(&config_paths);
    }

    ncm_buffer_destroy(&expected);
    ncm_buffer_destroy(&xdg);
    ncm_buffer_destroy(&home);
    return;
}

static void
test_early_exit_modes(void) {
    char *help_argv[] = {
        "ncmpcpp",
        "--help",
        "--config=custom-config",
        "--bindings=custom-bindings",
    };
    char *version_argv[] = {
        "ncmpcpp",
        "--version",
    };
    char *normal_argv[] = {
        "ncmpcpp",
    };
    CapturedConfigureResult result;

    REQUIRE(unsetenv("HOME") == 0);
    REQUIRE(unsetenv("XDG_CONFIG_HOME") == 0);
    captured_configure_result_init(&result);

    run_configure_capture(NCM_ARRAY_LEN(help_argv), help_argv, &result);
    REQUIRE_INT(result.exit_status, EXIT_SUCCESS);
    REQUIRE_CONTAINS(result.output.data, "Usage: ncmpcpp [options]...");
    REQUIRE_CONTAINS(result.output.data,
                     "~/.config/ncmpcpp/config AND ~/.ncmpcpp/config");
    REQUIRE_CONTAINS(result.output.data,
                     "~/.config/ncmpcpp/bindings AND ~/.ncmpcpp/bindings");
    REQUIRE((result.output.data == NULL)
            || (strstr(result.output.data, "custom-config") == NULL));
    REQUIRE_INT(result.error_output.len, 0);

    run_configure_capture(NCM_ARRAY_LEN(version_argv), version_argv, &result);
    REQUIRE_INT(result.exit_status, EXIT_SUCCESS);
    REQUIRE_CONTAINS(result.output.data, "ncmpcpp ");
    REQUIRE_CONTAINS(result.output.data, "encoding: UTF-8");
    REQUIRE_INT(result.error_output.len, 0);

    run_configure_capture(NCM_ARRAY_LEN(normal_argv), normal_argv, &result);
    REQUIRE_INT(result.exit_status, EXIT_FAILURE);
    REQUIRE_CONTAINS(result.error_output.data,
                     "HOME environment variable is not set");

    captured_configure_result_destroy(&result);
    return;
}

static void
test_quiet_mode(void) {
    char template[] = "/tmp/ncmpcpp-quiet-XXXXXX";
    char *root;
    char *normal_argv[6];
    char *quiet_argv[7];
    char *bad_argv[] = {
        "ncmpcpp",
        "--quiet",
        "--port=bad",
    };
    NcmBuffer config_path;
    NcmBuffer bindings_path;
    NcmBuffer app_dir;
    NcmBuffer lyrics_dir;
    NcmBuffer data;
    CapturedConfigureResult result;

    root = mkdtemp(template);
    REQUIRE(root != NULL);
    REQUIRE(setenv("HOME", root, 1) == 0);
    REQUIRE(unsetenv("MPD_HOST") == 0);
    REQUIRE(unsetenv("MPD_PORT") == 0);
    REQUIRE(setenv("TERM", "linux", 1) == 0);

    ncm_buffer_init(&config_path);
    ncm_buffer_init(&bindings_path);
    ncm_buffer_init(&app_dir);
    ncm_buffer_init(&lyrics_dir);
    ncm_buffer_init(&data);
    make_path(&config_path, root, "config");
    make_path(&bindings_path, root, "bindings");
    make_path(&app_dir, root, "app");
    make_path(&lyrics_dir, root, "lyrics");

    ncm_buffer_append(&data, STRLIT_ARGS("ncmpcpp_directory = \""));
    ncm_buffer_append(&data, app_dir.data, app_dir.len);
    ncm_buffer_append(&data, STRLIT_ARGS("\"\nlyrics_directory = \""));
    ncm_buffer_append(&data, lyrics_dir.data, lyrics_dir.len);
    ncm_buffer_append(&data, STRLIT_ARGS("\"\n"));
    write_file(config_path.data, config_path.len, data.data, data.len);
    write_file(bindings_path.data, bindings_path.len, LIT_ARGS("# ok\n"));

    normal_argv[0] = "ncmpcpp";
    normal_argv[1] = "--config";
    normal_argv[2] = config_path.data;
    normal_argv[3] = "--bindings";
    normal_argv[4] = bindings_path.data;
    normal_argv[5] = NULL;

    quiet_argv[0] = "ncmpcpp";
    quiet_argv[1] = "--quiet";
    quiet_argv[2] = "--config";
    quiet_argv[3] = config_path.data;
    quiet_argv[4] = "--bindings";
    quiet_argv[5] = bindings_path.data;
    quiet_argv[6] = NULL;

    global_state_init();
    configuration_init(&Config);
    ncm_bindings_configuration_init(&Bindings);
    captured_configure_result_init(&result);

    run_configure_capture(5, normal_argv, &result);
    REQUIRE_INT(result.exit_status, EXIT_SUCCESS);
    REQUIRE_CONTAINS(result.error_output.data,
                     "Reading configuration from ");
    REQUIRE_CONTAINS(result.error_output.data,
                     "Terminal doesn't support window title");

    run_configure_capture(6, quiet_argv, &result);
    REQUIRE_INT(result.exit_status, EXIT_SUCCESS);
    REQUIRE_INT(result.error_output.len, 0);

    run_configure_capture(NCM_ARRAY_LEN(bad_argv), bad_argv, &result);
    REQUIRE_INT(result.exit_status, EXIT_FAILURE);
    REQUIRE_CONTAINS(result.error_output.data,
                     "Error while processing configuration");
    REQUIRE_CONTAINS(result.error_output.data, "is invalid");

    captured_configure_result_destroy(&result);
    ncm_bindings_configuration_destroy(&Bindings);
    configuration_destroy(&Config);
    global_state_destroy();
    ncm_buffer_destroy(&data);
    ncm_buffer_destroy(&lyrics_dir);
    ncm_buffer_destroy(&app_dir);
    ncm_buffer_destroy(&bindings_path);
    ncm_buffer_destroy(&config_path);
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

    REQUIRE(configuration_read(&config, &paths, false, false, &error));
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

    REQUIRE(configuration_read(&config, &paths, false, false, &error));
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
        "none",
        "basic",
        "extended",
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

        REQUIRE(configuration_read(&config, &paths, false, false, &error));
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

    REQUIRE(!configuration_read(&config, &paths, false, false, &error));
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

    argv[0] = "ncmpcpp";
    argv[1] = "-c";
    argv[2] = config_path.data;
    argv[3] = "-b";
    argv[4] = bindings_path.data;
    argv[5] = "-hcli.test";
    argv[6] = "-p";
    argv[7] = "6620";
    argv[8] = "--screen";
    argv[9] = "browser";
    argv[10] = "--slave-screen=playlist";
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

    REQUIRE(!configuration_read(&config, &paths, false, false, &error));
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
    test_command_line_short_options();
    test_command_line_long_options();
    test_command_line_current_song();
    test_command_line_errors();
    test_expand_home();
    test_config_path_discovery();
    test_early_exit_modes();
    test_quiet_mode();
    test_config_defaults();
    test_config_file_parsing();
    test_regular_expression_flavors();
    test_configuration_options_apply();
    test_invalid_config_value();
    return EXIT_SUCCESS;
}
