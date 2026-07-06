#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "c/ncm_array.h"
#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "c/ncm_fs.h"
#include "c/ncm_job.h"
#include "c/ncm_log.h"
#include "c/ncm_process.h"
#include "c/ncm_random.h"
#include "c/ncm_regex.h"
#include "c/ncm_string.h"
#include "c/ncm_time.h"
#include "cbase/base_macros.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

#define REQUIRE(COND) do {                                                \
    if (!(COND)) {                                                        \
        fail(__FILE__, __LINE__, (char *)#COND);                         \
    }                                                                     \
} while (0)

#define REQUIRE_INT(ACTUAL, EXPECTED)                                     \
    require_int(__FILE__, __LINE__, (char *)#ACTUAL, ACTUAL, EXPECTED)

#define REQUIRE_STRING(ACTUAL, ACTUAL_LEN, EXPECTED)                      \
    require_string(__FILE__, __LINE__, (char *)#ACTUAL,                   \
                   ACTUAL, ACTUAL_LEN, LIT_ARGS(EXPECTED))

typedef struct TestJobData {
    int32 value;
    int32 completed;
} TestJobData;

NCM_ARRAY_DECLARE(test_int_array, TestIntArray, int32)
NCM_ARRAY_DEFINE(test_int_array, TestIntArray, int32, NULL)

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static bool digit_match_callback(int32 start, int32 len, void *user);
static bool test_job_run(void *user, NcmError *error);
static void test_job_complete(bool success, NcmError *error, void *user);
static void test_array(void);
static void test_time(void);
static void test_regex(void);
static void test_fs(void);
static void test_process(void);
static void test_random(void);
static void test_log(void);
static void test_job(void);

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

static bool
digit_match_callback(int32 start, int32 len, void *user) {
    int32 *count;

    (void)start;
    (void)len;
    count = user;
    *count += 1;
    return true;
}

static bool
test_job_run(void *user, NcmError *error) {
    TestJobData *data;

    (void)error;
    data = user;
    data->value += 1;
    return true;
}

static void
test_job_complete(bool success, NcmError *error, void *user) {
    TestJobData *data;

    (void)error;
    data = user;
    if (success) {
        data->completed += 1;
    }
    return;
}

static void
test_array(void) {
    TestIntArray array;
    int32 *item;
    int32 value;

    test_int_array_init(&array);
    item = test_int_array_append(&array);
    REQUIRE(item != NULL);
    *item = 10;
    value = 20;
    REQUIRE(test_int_array_append_copy(&array, &value));
    REQUIRE_INT(array.len, 2);
    REQUIRE_INT(array.items[0], 10);
    REQUIRE_INT(array.items[1], 20);
    test_int_array_remove_ordered(&array, 0);
    REQUIRE_INT(array.len, 1);
    REQUIRE_INT(array.items[0], 20);
    test_int_array_destroy(&array);
    return;
}

static void
test_time(void) {
    NcmTimePoint start;
    NcmTimePoint end;
    int64 elapsed;
    NcmError error;

    ncm_error_clear(&error);
    REQUIRE(ncm_time_monotonic_now(&start, &error));
    REQUIRE(ncm_time_sleep_ms(1, &error));
    REQUIRE(ncm_time_monotonic_now(&end, &error));
    elapsed = ncm_time_elapsed_ms(start, end);
    REQUIRE(elapsed >= 0);
    REQUIRE(ncm_time_since_ms(start, &elapsed, &error));
    REQUIRE(elapsed >= 0);
    return;
}

static void
test_regex(void) {
    NcmRegex regex;
    NcmBuffer escaped;
    NcmError error;
    int32 matches;

    ncm_error_clear(&error);
    ncm_regex_init(&regex);
    ncm_buffer_init(&escaped);

    ncm_regex_escape_literal(&escaped, LIT_ARGS("a+b"));
    REQUIRE_STRING(escaped.data, escaped.len, "a\\+b");

    REQUIRE(ncm_regex_compile(&regex, LIT_ARGS("A+B"),
                              NCM_REGEX_EXTENDED
                              |NCM_REGEX_LITERAL
                              |NCM_REGEX_ICASE,
                              &error));
    REQUIRE(ncm_regex_search(&regex, LIT_ARGS("xx a+b yy")));
    ncm_regex_destroy(&regex);

    REQUIRE(ncm_regex_compile(&regex, LIT_ARGS("[0-9]+"),
                              NCM_REGEX_EXTENDED, &error));
    matches = 0;
    REQUIRE(ncm_regex_for_each_match(&regex, LIT_ARGS("a1 b22 c333"),
                                     digit_match_callback, &matches));
    REQUIRE_INT(matches, 3);

    ncm_buffer_destroy(&escaped);
    ncm_regex_destroy(&regex);
    return;
}

static void
test_fs(void) {
    char template[] = "/tmp/ncmpcpp-infra-XXXXXX";
    char *tmpdir;
    NcmBuffer path;
    NcmFsStat stat;
    NcmFsDirectory directory;
    NcmFsEntry entry;
    NcmError error;
    bool found;
    FILE *file;

    ncm_error_clear(&error);
    tmpdir = mkdtemp(template);
    REQUIRE(tmpdir != NULL);

    ncm_buffer_init(&path);
    ncm_fs_entry_init(&entry);

    ncm_fs_join(&path, tmpdir, (int32)strlen(tmpdir), LIT_ARGS("a/b"));
    REQUIRE(ncm_fs_mkdir_all(path.data, path.len, &error));
    REQUIRE(ncm_fs_is_directory(path.data, path.len));

    ncm_fs_join(&path, tmpdir, (int32)strlen(tmpdir), LIT_ARGS("file.txt"));
    file = fopen(path.data, "wb");
    REQUIRE(file != NULL);
    fwrite("ok", 1, 2, file);
    fclose(file);
    REQUIRE(ncm_fs_stat(path.data, path.len, &stat, &error));
    REQUIRE(stat.exists);
    REQUIRE(stat.type == NCM_FS_ENTRY_FILE);
    REQUIRE_INT((int32)stat.size, 2);

    REQUIRE(ncm_fs_directory_open(&directory, tmpdir,
                                  (int32)strlen(tmpdir), &error));
    found = false;
    while (ncm_fs_directory_read(&directory, &entry, &error)) {
        if (ncm_string_equal(entry.name, entry.name_len,
                             LIT_ARGS("file.txt"))) {
            found = true;
        }
        ncm_fs_entry_destroy(&entry);
    }
    ncm_fs_directory_close(&directory);
    REQUIRE(found);

    REQUIRE(ncm_fs_unlink(path.data, path.len, &error));
    REQUIRE(!ncm_fs_exists(path.data, path.len));
    ncm_fs_join(&path, tmpdir, (int32)strlen(tmpdir), LIT_ARGS("a/b"));
    rmdir(path.data);
    ncm_fs_join(&path, tmpdir, (int32)strlen(tmpdir), LIT_ARGS("a"));
    rmdir(path.data);
    rmdir(tmpdir);

    ncm_buffer_destroy(&path);
    return;
}

static void
test_process(void) {
    NcmProcessCommand command;
    NcmProcessResult result;
    NcmError error;
    int32 status;

    ncm_error_clear(&error);
    ncm_process_command_init(&command);
    ncm_process_result_init(&result);

    REQUIRE(ncm_process_command_add_arg(&command, LIT_ARGS("/bin/sh")));
    REQUIRE(ncm_process_command_add_arg(&command, LIT_ARGS("-c")));
    REQUIRE(ncm_process_command_add_arg(&command, LIT_ARGS("printf infra")));
    REQUIRE(ncm_process_run_capture(&command, &result, &error));
    REQUIRE_INT(result.status, 0);
    REQUIRE_STRING(result.output.data, result.output.len, "infra");

    REQUIRE(ncm_process_run_shell(LIT_ARGS("exit 0"), &status, &error));
    REQUIRE_INT(status, 0);

    ncm_process_result_destroy(&result);
    ncm_process_command_destroy(&command);
    return;
}

static void
test_random(void) {
    NcmRandom first;
    NcmRandom second;
    uint64 first_value;
    uint64 second_value;

    ncm_random_init(&first, 123);
    ncm_random_init(&second, 123);
    first_value = ncm_random_u64(&first);
    second_value = ncm_random_u64(&second);
    REQUIRE(first_value == second_value);
    REQUIRE(ncm_random_range_i32(&first, 5) >= 0);
    REQUIRE(ncm_random_range_i32(&first, 5) < 5);
    return;
}

static void
test_log(void) {
    char template[] = "/tmp/ncmpcpp-log-XXXXXX";
    int32 fd;
    NcmLog log;
    NcmError error;
    FILE *file;
    char buffer[16];
    int32 len;

    ncm_error_clear(&error);
    fd = mkstemp(template);
    REQUIRE(fd >= 0);
    close(fd);

    ncm_log_init(&log);
    REQUIRE(ncm_log_open(&log, template, (int32)strlen(template), &error));
    ncm_log_write(&log, LIT_ARGS("log"));
    ncm_log_destroy(&log);

    file = fopen(template, "rb");
    REQUIRE(file != NULL);
    len = (int32)fread(buffer, 1, SIZEOF(buffer), file);
    fclose(file);
    REQUIRE_STRING(buffer, len, "log");
    unlink(template);
    return;
}

static void
test_job(void) {
    NcmJobQueue queue;
    TestJobData data;
    NcmJob job;
    NcmError error;

    ncm_error_clear(&error);
    data.value = 0;
    data.completed = 0;
    job.run = test_job_run;
    job.complete = test_job_complete;
    job.destroy = NULL;
    job.user = &data;
    ncm_error_clear(&job.error);
    job.success = false;

    ncm_job_queue_init(&queue);
    REQUIRE(ncm_job_queue_start(&queue, &error));
    REQUIRE(ncm_job_queue_push(&queue, job, &error));
    for (int32 i = 0; i < 100; i += 1) {
        if (ncm_job_queue_dispatch_completed(&queue) > 0) {
            break;
        }
        ncm_time_sleep_ms(1, NULL);
    }
    REQUIRE_INT(data.value, 1);
    REQUIRE_INT(data.completed, 1);
    ncm_job_queue_destroy(&queue);
    return;
}

int
main(void) {
    test_array();
    test_time();
    test_regex();
    test_fs();
    test_process();
    test_random();
    test_log();
    test_job();
    return EXIT_SUCCESS;
}
