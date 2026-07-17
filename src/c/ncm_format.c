#if !defined(NCM_FORMAT_C)
#define NCM_FORMAT_C

#include "c/ncm_format.h"

#include <ctype.h>
#include <stdio.h>

#include "c/ncm_string.h"
#include "c/ncm_type_conversions.h"
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "cbase/util.c"

static void ncm_format_expr_init(NcmFormatExpr *expr);
static void ncm_format_expr_destroy(NcmFormatExpr *expr);
static bool ncm_format_parse_bracket(NcmFormatExprList *out,
                                     char *data, int32 start, int32 end,
                                     uint32 flags, NcmError *error);
static enum NcmFormatResult ncm_format_render_list(NcmFormatExprList *list,
                                                   NcmSong *song,
                                                   NcmFormatCallbacks *cb,
                                                   void *left,
                                                   void *right,
                                                   uint32 flags,
                                                   int32 *no_output,
                                                   bool *switched);

static void
ncm_format_set_error(NcmError *error, char *message, int32 position) {
    char buffer[256];
    int32 len;

    if (error == NULL) {
        return;
    }

    len = SNPRINTF(buffer,
                   "format error: %s at position %d", message, position);
    if (len < 0) {
        ncm_error_set(error, 1, STRLIT_ARGS("format error"));
        return;
    }
    if (len >= SIZEOF(buffer)) {
        len = SIZEOF(buffer) - 1;
    }
    ncm_error_set(error, 1, buffer, len);
    return;
}

static int32
ncm_format_color_index_to_nc_color(int32 index, NcColor *color) {
    switch (index) {
    case 0:
        *color = nc_color_default();
        return 1;
    case 1:
        *color = nc_color_make(COLOR_BLACK, -1, false, false);
        return 1;
    case 2:
        *color = nc_color_make(COLOR_RED, -1, false, false);
        return 1;
    case 3:
        *color = nc_color_make(COLOR_GREEN, -1, false, false);
        return 1;
    case 4:
        *color = nc_color_make(COLOR_YELLOW, -1, false, false);
        return 1;
    case 5:
        *color = nc_color_make(COLOR_BLUE, -1, false, false);
        return 1;
    case 6:
        *color = nc_color_make(COLOR_MAGENTA, -1, false, false);
        return 1;
    case 7:
        *color = nc_color_make(COLOR_CYAN, -1, false, false);
        return 1;
    case 8:
        *color = nc_color_make(COLOR_WHITE, -1, false, false);
        return 1;
    case 9:
        *color = nc_color_end();
        return 1;
    default:
        return 0;
    }
}


static bool
ncm_format_parse_color_component(char *data, int32 data_len,
                                 bool background, int16 *result) {
    int32 value;

    if (STREQUAL(data, data_len, STRLIT_ARGS("black"))) {
        *result = COLOR_BLACK;
        return true;
    }
    if (STREQUAL(data, data_len, STRLIT_ARGS("red"))) {
        *result = COLOR_RED;
        return true;
    }
    if (STREQUAL(data, data_len, STRLIT_ARGS("green"))) {
        *result = COLOR_GREEN;
        return true;
    }
    if (STREQUAL(data, data_len, STRLIT_ARGS("yellow"))) {
        *result = COLOR_YELLOW;
        return true;
    }
    if (STREQUAL(data, data_len, STRLIT_ARGS("blue"))) {
        *result = COLOR_BLUE;
        return true;
    }
    if (STREQUAL(data, data_len, STRLIT_ARGS("magenta"))) {
        *result = COLOR_MAGENTA;
        return true;
    }
    if (STREQUAL(data, data_len, STRLIT_ARGS("cyan"))) {
        *result = COLOR_CYAN;
        return true;
    }
    if (STREQUAL(data, data_len, STRLIT_ARGS("white"))) {
        *result = COLOR_WHITE;
        return true;
    }
    if (background
        && STREQUAL(data, data_len,
                            STRLIT_ARGS("transparent"))) {
        *result = -1;
        return true;
    }
    if (background
        && STREQUAL(data, data_len, STRLIT_ARGS("current"))) {
        *result = -2;
        return true;
    }

    value = 0;
    if (data_len <= 0) {
        return false;
    }
    for (int32 i = 0; i < data_len; i += 1) {
        if (!isdigit((unsigned char)data[i])) {
            return false;
        }
        value = value*10 + data[i] - '0';
    }
    if (value < (background ? 0 : 1) || value > 256) {
        return false;
    }
    *result = (int16)(value - 1);
    return true;
}

static bool
ncm_format_parse_named_color(char *data, int32 data_len, NcColor *color) {
    int32 underscore;
    int16 foreground;
    int16 background;

    if (STREQUAL(data, data_len, STRLIT_ARGS("default"))) {
        *color = nc_color_default();
        return true;
    }
    if (STREQUAL(data, data_len, STRLIT_ARGS("end"))) {
        *color = nc_color_end();
        return true;
    }

    underscore = -1;
    for (int32 i = 0; i < data_len; i += 1) {
        if (data[i] == '_') {
            underscore = i;
            break;
        }
    }

    if (underscore < 0) {
        if (!ncm_format_parse_color_component(data, data_len,
                                              false, &foreground)) {
            return false;
        }
        *color = nc_color_make(foreground, -2, false, false);
        return true;
    }

    if (!ncm_format_parse_color_component(data, underscore,
                                          false, &foreground)) {
        return false;
    }
    if (!ncm_format_parse_color_component(data + underscore + 1,
                                          data_len - underscore - 1,
                                          true, &background)) {
        return false;
    }
    *color = nc_color_make(foreground, background, false, false);
    return true;
}

static bool
ncm_format_read_uint(char *data, int32 start, int32 end,
                     uint32 *value) {
    uint32 result;

    result = 0;
    if (start >= end) {
        return false;
    }

    for (int32 i = start; i < end; i += 1) {
        if (!isdigit((unsigned char)data[i])) {
            return false;
        }
        result = result*10 + (uint32)(data[i] - '0');
    }

    *value = result;
    return true;
}

static bool
ncm_format_text_append(NcmFormatExprList *list,
                       NcmBuffer *token) {
    NcmFormatExpr *expr;

    if (token->len <= 0) {
        return true;
    }

    if ((expr = ncm_format_expr_list_append(list)) == NULL) {
        return false;
    }
    expr->type = NCM_FORMAT_EXPR_TEXT;
    expr->value.text = *token;
    ncm_buffer_init(token);
    return true;
}

static bool
ncm_format_append_group_or_single(NcmFormatExprList *list,
                                  NcmFormatExprList *source) {
    NcmFormatExpr *expr;

    if (source->len == 1) {
        expr = ncm_format_expr_list_append(list);
        if (expr == NULL) {
            return false;
        }
        *expr = source->items[0];
        source->items[0].type = NCM_FORMAT_EXPR_TEXT;
        ncm_buffer_init(&source->items[0].value.text);
        source->len = 0;
        return true;
    }

    expr = ncm_format_expr_list_append(list);
    if (expr == NULL) {
        return false;
    }
    expr->type = NCM_FORMAT_EXPR_GROUP;
    ncm_format_expr_list_move(&expr->value.list, source);
    return true;
}

static bool
ncm_format_expr_list_reserve(NcmFormatExprList *list, int32 extra) {
    int32 needed;
    int32 old_cap;
    int32 new_cap;

    if (extra <= 0) {
        return true;
    }

    needed = list->len + extra;
    if (needed <= list->cap) {
        return true;
    }

    old_cap = list->cap;
    new_cap = list->cap;
    if (new_cap <= 0) {
        new_cap = 8;
    }
    while (new_cap < needed) {
        new_cap *= 2;
    }

    list->items = realloc2(list->items, old_cap, new_cap,
                         SIZEOF(*list->items));
    list->cap = new_cap;
    return true;
}

void
ncm_format_expr_list_init(NcmFormatExprList *list) {
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
    return;
}

void
ncm_format_expr_list_clear(NcmFormatExprList *list) {
    if (list == NULL) {
        return;
    }
    for (int32 i = 0; i < list->len; i += 1) {
        ncm_format_expr_destroy(&list->items[i]);
    }
    list->len = 0;
    return;
}

void
ncm_format_expr_list_destroy(NcmFormatExprList *list) {
    if (list == NULL) {
        return;
    }
    ncm_format_expr_list_clear(list);
    if (list->items != NULL) {
        free2(list->items, list->cap*SIZEOF(*list->items));
    }
    ncm_format_expr_list_init(list);
    return;
}

void
ncm_format_expr_list_move(NcmFormatExprList *dest,
                          NcmFormatExprList *source) {
    *dest = *source;
    ncm_format_expr_list_init(source);
    return;
}

NcmFormatExpr *
ncm_format_expr_list_append(NcmFormatExprList *list) {
    NcmFormatExpr *expr;

    if (!ncm_format_expr_list_reserve(list, 1)) {
        return NULL;
    }

    expr = &list->items[list->len];
    list->len += 1;
    ncm_format_expr_init(expr);
    return expr;
}

static void
ncm_format_expr_init(NcmFormatExpr *expr) {
    expr->type = NCM_FORMAT_EXPR_TEXT;
    ncm_buffer_init(&expr->value.text);
    return;
}

static void
ncm_format_expr_destroy(NcmFormatExpr *expr) {
    switch (expr->type) {
    case NCM_FORMAT_EXPR_TEXT:
        ncm_buffer_destroy(&expr->value.text);
        break;
    case NCM_FORMAT_EXPR_GROUP:
    case NCM_FORMAT_EXPR_FIRST_OF:
        ncm_format_expr_list_destroy(&expr->value.list);
        break;
    case NCM_FORMAT_EXPR_COLOR:
    case NCM_FORMAT_EXPR_FORMAT:
    case NCM_FORMAT_EXPR_OUTPUT_SWITCH:
    case NCM_FORMAT_EXPR_SONG_TAG:
        break;
    }
    ncm_format_expr_init(expr);
    return;
}

void
ncm_format_ast_init(NcmFormatAst *ast) {
    ncm_format_expr_list_init(&ast->root);
    return;
}

void
ncm_format_ast_destroy(NcmFormatAst *ast) {
    if (ast == NULL) {
        return;
    }
    ncm_format_expr_list_destroy(&ast->root);
    return;
}

void
ncm_format_ast_clear(NcmFormatAst *ast) {
    if (ast == NULL) {
        return;
    }
    ncm_format_expr_list_clear(&ast->root);
    return;
}

void
ncm_format_ast_move(NcmFormatAst *dest, NcmFormatAst *source) {
    ncm_format_ast_destroy(dest);
    ncm_format_expr_list_move(&dest->root, &source->root);
    return;
}

bool
ncm_format_ast_append_first_of_getters(NcmFormatAst *ast,
                                       enum NcmSongGetter *getters,
                                       int32 getters_len) {
    NcmFormatExpr *first;

    if (getters_len <= 0) {
        return true;
    }

    first = ncm_format_expr_list_append(&ast->root);
    if (first == NULL) {
        return false;
    }
    first->type = NCM_FORMAT_EXPR_FIRST_OF;
    ncm_format_expr_list_init(&first->value.list);

    for (int32 i = 0; i < getters_len; i += 1) {
        NcmFormatExpr *tag;

        tag = ncm_format_expr_list_append(&first->value.list);
        if (tag == NULL) {
            return false;
        }
        tag->type = NCM_FORMAT_EXPR_SONG_TAG;
        tag->value.song_tag.getter = getters[i];
        tag->value.song_tag.delimiter = 0;
    }
    return true;
}

static bool
ncm_format_find_matching_brace(char *data, int32 start,
                               int32 end, int32 *result) {
    int32 depth;

    depth = 1;
    for (int32 i = start + 1; i < end; i += 1) {
        if (data[i] == '{') {
            depth += 1;
        } else if (data[i] == '}') {
            depth -= 1;
            if (depth == 0) {
                *result = i;
                return true;
            }
        }
    }
    return false;
}

static bool
ncm_format_parse_dollar(NcmFormatExprList *out, char *data,
                        int32 *pos, int32 end, uint32 flags,
                        NcmError *error) {
    NcmFormatExpr *expr;
    int32 i;

    i = *pos + 1;
    if (i >= end) {
        ncm_format_set_error(error, "unexpected end", i);
        return false;
    }

    if (data[i] == '$') {
        expr = ncm_format_expr_list_append(out);
        if (expr == NULL) {
            return false;
        }
        expr->type = NCM_FORMAT_EXPR_TEXT;
        ncm_buffer_append_byte(&expr->value.text, '$');
        *pos = i;
        return true;
    }

    expr = ncm_format_expr_list_append(out);
    if (expr == NULL) {
        return false;
    }

    if ((flags & NCM_FORMAT_FLAG_COLOR)
        && isdigit((unsigned char)data[i])) {
        int32 color_index;

        color_index = ncm_color_index_from_char(data[i]);
        if (!ncm_format_color_index_to_nc_color(color_index,
                                                &expr->value.color)) {
            ncm_format_set_error(error, "invalid color", i);
            return false;
        }
        expr->type = NCM_FORMAT_EXPR_COLOR;
    } else if ((flags & NCM_FORMAT_FLAG_COLOR) && (data[i] == '(')) {
        int32 color_start;

        i += 1;
        color_start = i;
        while ((i < end) && (data[i] != ')')) {
            i += 1;
        }
        if (i >= end) {
            ncm_format_set_error(error, "unexpected end", i);
            return false;
        }
        if (!ncm_format_parse_named_color(data + color_start,
                                          i - color_start,
                                          &expr->value.color)) {
            ncm_format_set_error(error, "invalid color", color_start);
            return false;
        }
        expr->type = NCM_FORMAT_EXPR_COLOR;
    } else if ((flags & NCM_FORMAT_FLAG_OUTPUT_SWITCH)
               && (data[i] == 'R')) {
        expr->type = NCM_FORMAT_EXPR_OUTPUT_SWITCH;
    } else if ((flags & NCM_FORMAT_FLAG_FORMAT) && (data[i] == 'b')) {
        expr->type = NCM_FORMAT_EXPR_FORMAT;
        expr->value.format = NC_FORMAT_BOLD;
    } else if ((flags & NCM_FORMAT_FLAG_FORMAT) && (data[i] == 'u')) {
        expr->type = NCM_FORMAT_EXPR_FORMAT;
        expr->value.format = NC_FORMAT_UNDERLINE;
    } else if ((flags & NCM_FORMAT_FLAG_FORMAT) && (data[i] == 'a')) {
        expr->type = NCM_FORMAT_EXPR_FORMAT;
        expr->value.format = NC_FORMAT_ALT_CHARSET;
    } else if ((flags & NCM_FORMAT_FLAG_FORMAT) && (data[i] == 'r')) {
        expr->type = NCM_FORMAT_EXPR_FORMAT;
        expr->value.format = NC_FORMAT_REVERSE;
    } else if ((flags & NCM_FORMAT_FLAG_FORMAT) && (data[i] == 'i')) {
        expr->type = NCM_FORMAT_EXPR_FORMAT;
        expr->value.format = NC_FORMAT_ITALIC;
    } else if ((flags & NCM_FORMAT_FLAG_FORMAT) && (data[i] == '/')) {
        i += 1;
        if (i >= end) {
            ncm_format_set_error(error, "unexpected end", i);
            return false;
        }
        expr->type = NCM_FORMAT_EXPR_FORMAT;
        if (data[i] == 'b') {
            expr->value.format = NC_FORMAT_NO_BOLD;
        } else if (data[i] == 'u') {
            expr->value.format = NC_FORMAT_NO_UNDERLINE;
        } else if (data[i] == 'a') {
            expr->value.format = NC_FORMAT_NO_ALT_CHARSET;
        } else if (data[i] == 'r') {
            expr->value.format = NC_FORMAT_NO_REVERSE;
        } else if (data[i] == 'i') {
            expr->value.format = NC_FORMAT_NO_ITALIC;
        } else {
            ncm_format_set_error(error, "invalid format", i);
            return false;
        }
    } else {
        ncm_format_set_error(error, "invalid character", i);
        return false;
    }

    *pos = i;
    return true;
}

static bool
ncm_format_parse_percent(NcmFormatExprList *out, char *data,
                         int32 *pos, int32 end, NcmError *error) {
    NcmFormatExpr *expr;
    uint32 delimiter;
    int32 i;
    int32 delimiter_start;
    enum NcmSongGetter getter;

    i = *pos + 1;
    if (i >= end) {
        ncm_format_set_error(error, "unexpected end", i);
        return false;
    }

    if (data[i] == '%') {
        expr = ncm_format_expr_list_append(out);
        if (expr == NULL) {
            return false;
        }
        expr->type = NCM_FORMAT_EXPR_TEXT;
        ncm_buffer_append_byte(&expr->value.text, '%');
        *pos = i;
        return true;
    }

    delimiter = 0;
    if (isdigit((unsigned char)data[i])) {
        delimiter_start = i;
        while ((i < end) && isdigit((unsigned char)data[i])) {
            i += 1;
        }
        if (i >= end) {
            ncm_format_set_error(error, "unexpected end", i);
            return false;
        }
        if (!ncm_format_read_uint(data, delimiter_start, i, &delimiter)) {
            ncm_format_set_error(error, "invalid tag delimiter", i);
            return false;
        }
    }

    getter = ncm_song_getter_from_char(data[i]);
    if (getter == NCM_SONG_GETTER_NONE) {
        ncm_format_set_error(error, "invalid tag", i);
        return false;
    }

    expr = ncm_format_expr_list_append(out);
    if (expr == NULL) {
        return false;
    }
    expr->type = NCM_FORMAT_EXPR_SONG_TAG;
    expr->value.song_tag.getter = getter;
    expr->value.song_tag.delimiter = delimiter;
    *pos = i;
    return true;
}

static bool
ncm_format_parse_first_of(NcmFormatExprList *out, char *data,
                          int32 *pos, int32 end, uint32 flags,
                          NcmError *error) {
    NcmFormatExpr *first;
    bool done;
    int32 i;

    first = ncm_format_expr_list_append(out);
    if (first == NULL) {
        return false;
    }
    first->type = NCM_FORMAT_EXPR_FIRST_OF;
    ncm_format_expr_list_init(&first->value.list);

    i = *pos;
    done = false;
    while (!done) {
        NcmFormatExprList inner;
        int32 close;

        if (!ncm_format_find_matching_brace(data, i, end, &close)) {
            ncm_format_set_error(error, "unexpected end", i);
            return false;
        }

        ncm_format_expr_list_init(&inner);
        if (!ncm_format_parse_bracket(&inner, data, i + 1, close,
                                      flags, error)) {
            ncm_format_expr_list_destroy(&inner);
            return false;
        }
        if (!ncm_format_append_group_or_single(&first->value.list,
                                               &inner)) {
            ncm_format_expr_list_destroy(&inner);
            return false;
        }
        ncm_format_expr_list_destroy(&inner);

        i = close + 1;
        if ((i < end) && (data[i] == '|')) {
            i += 1;
            if ((i >= end) || (data[i] != '{')) {
                ncm_format_set_error(error, "expected bracket", i);
                return false;
            }
        } else {
            done = true;
        }
    }

    *pos = i - 1;
    return true;
}

static bool
ncm_format_parse_bracket(NcmFormatExprList *out, char *data,
                         int32 start, int32 end, uint32 flags,
                         NcmError *error) {
    NcmBuffer token;
    bool ok;

    ncm_buffer_init(&token);
    ok = true;
    for (int32 i = start; ok && (i < end); i += 1) {
        if (data[i] == '{') {
            ok = ncm_format_text_append(out, &token)
                 && ncm_format_parse_first_of(out, data, &i, end,
                                              flags, error);
        } else if ((flags & NCM_FORMAT_FLAG_TAG) && (data[i] == '%')) {
            ok = ncm_format_text_append(out, &token)
                 && ncm_format_parse_percent(out, data, &i, end, error);
        } else if ((flags & (NCM_FORMAT_FLAG_COLOR
                             |NCM_FORMAT_FLAG_FORMAT
                             |NCM_FORMAT_FLAG_OUTPUT_SWITCH))
                   && (data[i] == '$')) {
            ok = ncm_format_text_append(out, &token)
                 && ncm_format_parse_dollar(out, data, &i, end,
                                            flags, error);
        } else {
            ncm_buffer_append_byte(&token, data[i]);
        }
    }

    if (ok) {
        ok = ncm_format_text_append(out, &token);
    }
    ncm_buffer_destroy(&token);
    return ok;
}

bool
ncm_format_parse(NcmFormatAst *ast, char *data, int32 data_len,
                 uint32 flags, NcmError *error) {
    NcmFormatAst tmp;

    ncm_format_ast_init(&tmp);
    if (!ncm_format_parse_bracket(&tmp.root, data, 0, data_len,
                                  flags, error)) {
        ncm_format_ast_destroy(&tmp);
        return false;
    }

    ncm_format_ast_move(ast, &tmp);
    ncm_format_ast_destroy(&tmp);
    return true;
}

static enum NcmFormatResult
ncm_format_result_add(enum NcmFormatResult base,
                      enum NcmFormatResult result) {
    if ((base == NCM_FORMAT_RESULT_MISSING)
        || (result == NCM_FORMAT_RESULT_MISSING)) {
        return NCM_FORMAT_RESULT_MISSING;
    }
    if ((base == NCM_FORMAT_RESULT_OK)
        || (result == NCM_FORMAT_RESULT_OK)) {
        return NCM_FORMAT_RESULT_OK;
    }
    return NCM_FORMAT_RESULT_EMPTY;
}

NcmBuffer
ncm_format_render_tag(NcmSong *song, NcmFormatSongTag *tag) {
    NcmBuffer result;

    ncm_buffer_init(&result);
    if ((song == NULL) || (tag == NULL)) {
        return result;
    }

    result = ncm_song_tags_buffer(song, tag->getter, STRLIT_ARGS(" | "),
                                  true);
    if ((tag->delimiter > 0) && (result.len > 0)) {
        int32 limit;

        if ((tag->getter == NCM_SONG_GETTER_DATE)
            || (tag->getter == NCM_SONG_GETTER_LENGTH)) {
            limit = (int32)tag->delimiter;
            if (limit < result.len) {
                result.len = limit;
            }
        } else {
            limit = utf8_cut_width(result.data, result.len,
                                       (int32)tag->delimiter);
            result.len = limit;
        }
        result.data[result.len] = '\0';
    }

    return result;
}

static void
ncm_format_emit_text(NcmFormatCallbacks *cb, void *user,
                     char *data, int32 data_len,
                     NcmFormatSongTag *tag) {
    if ((cb != NULL) && (cb->text != NULL) && (data_len > 0)) {
        cb->text(user, data, data_len, tag);
    }
    return;
}

static void
ncm_format_emit_color(NcmFormatCallbacks *cb, void *user, NcColor color) {
    if ((cb != NULL) && (cb->color != NULL)) {
        cb->color(user, color);
    }
    return;
}

static void
ncm_format_emit_format(NcmFormatCallbacks *cb, void *user,
                       enum NcFormat format) {
    if ((cb != NULL) && (cb->format != NULL)) {
        cb->format(user, format);
    }
    return;
}

static void *
ncm_format_current_output(void *left, void *right, bool switched) {
    if (switched && (right != NULL)) {
        return right;
    }
    return left;
}

static enum NcmFormatResult
ncm_format_render_expr(NcmFormatExpr *expr, NcmSong *song,
                       NcmFormatCallbacks *cb, void *left,
                       void *right, uint32 flags, int32 *no_output,
                       bool *switched) {
    void *output;
    NcmBuffer tag;
    enum NcmFormatResult result;

    output = ncm_format_current_output(left, right, *switched);
    switch (expr->type) {
    case NCM_FORMAT_EXPR_TEXT:
        if (expr->value.text.len <= 0) {
            return NCM_FORMAT_RESULT_EMPTY;
        }
        if (*no_output <= 0) {
            ncm_format_emit_text(cb, output, expr->value.text.data,
                                 expr->value.text.len, NULL);
        }
        return NCM_FORMAT_RESULT_OK;
    case NCM_FORMAT_EXPR_COLOR:
        if ((*no_output <= 0) && (flags & NCM_FORMAT_FLAG_COLOR)) {
            ncm_format_emit_color(cb, output, expr->value.color);
        }
        return NCM_FORMAT_RESULT_EMPTY;
    case NCM_FORMAT_EXPR_FORMAT:
        if ((*no_output <= 0) && (flags & NCM_FORMAT_FLAG_FORMAT)) {
            ncm_format_emit_format(cb, output, expr->value.format);
        }
        return NCM_FORMAT_RESULT_EMPTY;
    case NCM_FORMAT_EXPR_OUTPUT_SWITCH:
        if (*no_output <= 0) {
            *switched = true;
        }
        return NCM_FORMAT_RESULT_OK;
    case NCM_FORMAT_EXPR_SONG_TAG:
        if (!(flags & NCM_FORMAT_FLAG_TAG) || (song == NULL)) {
            return NCM_FORMAT_RESULT_MISSING;
        }
        tag = ncm_format_render_tag(song, &expr->value.song_tag);
        if (tag.len <= 0) {
            ncm_buffer_destroy(&tag);
            return NCM_FORMAT_RESULT_MISSING;
        }
        if (*no_output <= 0) {
            ncm_format_emit_text(cb, output, tag.data, tag.len,
                                 &expr->value.song_tag);
        }
        ncm_buffer_destroy(&tag);
        return NCM_FORMAT_RESULT_OK;
    case NCM_FORMAT_EXPR_GROUP:
        *no_output += 1;
        result = ncm_format_render_list(&expr->value.list, song, cb, left,
                                        right, flags, no_output, switched);
        *no_output -= 1;
        if (result == NCM_FORMAT_RESULT_MISSING) {
            return NCM_FORMAT_RESULT_EMPTY;
        }
        if ((*no_output <= 0) && (result == NCM_FORMAT_RESULT_OK)) {
            result = ncm_format_render_list(&expr->value.list, song, cb,
                                            left, right, flags, no_output,
                                            switched);
        }
        return result;
    case NCM_FORMAT_EXPR_FIRST_OF:
        for (int32 i = 0; i < expr->value.list.len; i += 1) {
            result = ncm_format_render_expr(&expr->value.list.items[i],
                                            song, cb, left, right, flags,
                                            no_output, switched);
            if (result == NCM_FORMAT_RESULT_OK) {
                return NCM_FORMAT_RESULT_OK;
            }
        }
        return NCM_FORMAT_RESULT_EMPTY;
    }

    return NCM_FORMAT_RESULT_EMPTY;
}

static enum NcmFormatResult
ncm_format_render_list(NcmFormatExprList *list, NcmSong *song,
                       NcmFormatCallbacks *cb, void *left,
                       void *right, uint32 flags, int32 *no_output,
                       bool *switched) {
    enum NcmFormatResult result;

    result = NCM_FORMAT_RESULT_EMPTY;
    for (int32 i = 0; i < list->len; i += 1) {
        enum NcmFormatResult part;

        part = ncm_format_render_expr(&list->items[i], song, cb, left,
                                      right, flags, no_output, switched);
        result = ncm_format_result_add(result, part);
        if (result == NCM_FORMAT_RESULT_MISSING) {
            break;
        }
    }
    return result;
}

void
ncm_format_render(NcmFormatAst *ast, NcmSong *song,
                  NcmFormatCallbacks *callbacks, void *output,
                  void *second_output, uint32 flags) {
    int32 no_output;
    bool switched;

    no_output = 0;
    switched = false;
    if (ast == NULL) {
        return;
    }

    (void)ncm_format_render_list(&ast->root, song, callbacks, output,
                                 second_output, flags, &no_output,
                                 &switched);
    return;
}

static void
ncm_format_buffer_text(void *user, char *data, int32 data_len,
                       NcmFormatSongTag *tag) {
    (void)tag;
    nc_buffer_append_data((NcBuffer *)user, data, data_len);
    return;
}

static void
ncm_format_buffer_color(void *user, NcColor color) {
    NcBuffer *buffer = (NcBuffer *)user;

    nc_buffer_add_color(buffer, nc_buffer_len(buffer), color, (uint64)-1);
    return;
}

static void
ncm_format_buffer_format(void *user, enum NcFormat format) {
    NcBuffer *buffer = (NcBuffer *)user;

    nc_buffer_add_format(buffer, nc_buffer_len(buffer), format, (uint64)-1);
    return;
}

void
ncm_format_render_buffer(NcmFormatAst *ast, NcmSong *song,
                         NcBuffer *buffer, NcBuffer *right_aligned,
                         uint32 flags) {
    NcmFormatCallbacks callbacks;

    callbacks.text = ncm_format_buffer_text;
    callbacks.color = ncm_format_buffer_color;
    callbacks.format = ncm_format_buffer_format;
    ncm_format_render(ast, song, &callbacks, buffer, right_aligned, flags);
    return;
}

static void
ncm_format_string_text(void *user, char *data, int32 data_len,
                       NcmFormatSongTag *tag) {
    (void)tag;
    ncm_buffer_append((NcmBuffer *)user, data, data_len);
    return;
}

NcmBuffer
ncm_format_render_string(NcmFormatAst *ast, NcmSong *song) {
    NcmFormatCallbacks callbacks;
    NcmBuffer result;

    callbacks.text = ncm_format_string_text;
    callbacks.color = NULL;
    callbacks.format = NULL;
    ncm_buffer_init(&result);
    ncm_format_render(ast, song, &callbacks, &result, &result,
                      NCM_FORMAT_FLAG_TAG);
    return result;
}

#endif /* NCM_FORMAT_C */
