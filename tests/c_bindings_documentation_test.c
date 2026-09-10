#include "cbase.h"
#include "ncmpcpp2.h"

#define main ncmpcpp2_application_main
#include "main.c"
#undef main

static bool
bindings_doc_starts_with(char *string, char *prefix) {
    int32 prefix_len = strlen32(prefix);

    for (int32 i = 0; i < prefix_len; i += 1) {
        if (string[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

static bool
bindings_doc_block_end(char *line) {
    return line[0] == '#' && ((line[1] == '\n') || (line[1] == '\0'));
}

static int32
bindings_doc_token_len(char *token) {
    int32 len = 0;

    while ((token[len] != '\0') && !isspace((unsigned char)token[len])) {
        len += 1;
    }
    return len;
}

static bool
bindings_doc_token_equal(char *token, int32 token_len, char *value) {
    return STREQUAL(token, token_len, value, strlen32(value));
}

static bool
bindings_doc_directive(char *token, int32 token_len) {
    static char *directives[] = {
        "push_character",
        "push_characters",
        "require_screen",
        "require_runnable",
        "run_external_command",
        "run_external_console_command",
        "set_visualizer_sample_multiplier",
    };

    for (int32 i = 0; i < LENGTH(directives); i += 1) {
        if (bindings_doc_token_equal(token, token_len, directives[i])) {
            return true;
        }
    }
    return false;
}

static void
bindings_doc_assert_action_line(char *line, int32 line_number,
                                int32 *count) {
    enum ActionType type = ACTION_COUNT;
    char *token = line + 3;
    int32 token_len;
    int32 status;

    token_len = bindings_doc_token_len(token);
    if ((token_len == 0) || bindings_doc_directive(token, token_len)) {
        return;
    }

    status = ncm_action_type_parse(token, token_len, &type);
    if (status < 0) {
        fprintf(stderr, "%s:%d: invalid action '%.*s'\n",
                "doc/bindings", line_number, token_len, token);
    }
    ASSERT_ZERO(status);
    ASSERT(type != ACTION_COUNT);
    *count += 1;
    return;
}

static void
test_documented_binding_actions_parse(void) {
    FILE *file = fopen("doc/bindings", "r");
    char line[512];
    int32 line_number = 0;
    int32 action_count = 0;
    bool in_binding = false;

    ASSERT(file != NULL);
    while (fgets(line, (int)SIZEOF(line), file) != NULL) {
        line_number += 1;
        if (bindings_doc_starts_with(line, "#def_key ")
            || bindings_doc_starts_with(line, "#def_command ")) {
            in_binding = true;
            continue;
        }
        if (in_binding && bindings_doc_block_end(line)) {
            in_binding = false;
            continue;
        }
        if (in_binding && bindings_doc_starts_with(line, "#  ")) {
            bindings_doc_assert_action_line(line, line_number,
                                            &action_count);
        }
    }
    ASSERT_ZERO(ferror(file));
    ASSERT_ZERO(fclose(file));
    ASSERT(action_count > 0);
    return;
}

int
main(void) {
    test_documented_binding_actions_parse();
    return 0;
}
