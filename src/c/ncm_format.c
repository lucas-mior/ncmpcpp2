#if !defined(NCM_FORMAT_C)
#define NCM_FORMAT_C

#include "cbase.h"

#include "c/ncm_c.h"

static void ncm_format_expr_destroy(NcmFormatExpr *expr);
static int32 ncm_format_parse_bracket(NcmFormatExprList *out,
                                      char *data, int32 start, int32 end,
                                      uint32 flags, NcmError *ncm_error);
static enum NcmFormatResult ncm_format_render_list(NcmFormatExprList *list,
                                                   NcmSong *song,
                                                   NcmFormatCallbacks *cb,
                                                   void *left,
                                                   void *right,
                                                   uint32 flags,
                                                   int32 *no_output,
                                                   bool *switched);

static int32
ncm_format_set_error(NcmError *ncm_error, char *message, int32 position) {
    char buffer[256];
    int32 len;

    len = SNPRINTF(buffer,
                   "format error: %s at position %d", message, position);
    return ncm_error_set_code(ncm_error, NCM_ERROR_PARSE, buffer, len);
}

static int32
ncm_format_color_index_to_nc_color(int32 index, NcColor *color) {
    switch (index) {
    case 0:
        *color = nc_color_default();
        return 0;
    case 1:
        *color = nc_color_make(COLOR_BLACK, -1, false, false);
        return 0;
    case 2:
        *color = nc_color_make(COLOR_RED, -1, false, false);
        return 0;
    case 3:
        *color = nc_color_make(COLOR_GREEN, -1, false, false);
        return 0;
    case 4:
        *color = nc_color_make(COLOR_YELLOW, -1, false, false);
        return 0;
    case 5:
        *color = nc_color_make(COLOR_BLUE, -1, false, false);
        return 0;
    case 6:
        *color = nc_color_make(COLOR_MAGENTA, -1, false, false);
        return 0;
    case 7:
        *color = nc_color_make(COLOR_CYAN, -1, false, false);
        return 0;
    case 8:
        *color = nc_color_make(COLOR_WHITE, -1, false, false);
        return 0;
    case 9:
        *color = nc_color_end();
        return 0;
    default:
        return -NCM_ERROR_PARSE;
    }
}

static int32
ncm_format_parse_color_component(char *data, int32 data_len,
                                 bool background, int16 *result) {
    int32 value;
    int32 lower_bound;

    if (STREQUAL(data, data_len, "black")) {
        *result = COLOR_BLACK;
        return 0;
    }
    if (STREQUAL(data, data_len, "red")) {
        *result = COLOR_RED;
        return 0;
    }
    if (STREQUAL(data, data_len, "green")) {
        *result = COLOR_GREEN;
        return 0;
    }
    if (STREQUAL(data, data_len, "yellow")) {
        *result = COLOR_YELLOW;
        return 0;
    }
    if (STREQUAL(data, data_len, "blue")) {
        *result = COLOR_BLUE;
        return 0;
    }
    if (STREQUAL(data, data_len, "magenta")) {
        *result = COLOR_MAGENTA;
        return 0;
    }
    if (STREQUAL(data, data_len, "cyan")) {
        *result = COLOR_CYAN;
        return 0;
    }
    if (STREQUAL(data, data_len, "white")) {
        *result = COLOR_WHITE;
        return 0;
    }
    if (background
        && STREQUAL(data, data_len, "transparent")) {
        *result = -1;
        return 0;
    }
    if (background
        && STREQUAL(data, data_len, "current")) {
        *result = -2;
        return 0;
    }

    value = 0;
    if (data_len <= 0) {
        return -NCM_ERROR_PARSE;
    }
    for (int32 i = 0; i < data_len; i += 1) {
        int32 digit;

        if (!isdigit((uint8)data[i])) {
            return -NCM_ERROR_PARSE;
        }
        digit = data[i] - '0';
        if (value > (MAXOF(value) - digit)/10) {
            return -EOVERFLOW;
        }
        value = value*10 + digit;
    }
    if (background) {
        lower_bound = 0;
    } else {
        lower_bound = 1;
    }
    if ((value < lower_bound) || (value > 256)) {
        return -NCM_ERROR_PARSE;
    }
    *result = (int16)(value - 1);
    return 0;
}

static int32
ncm_format_parse_named_color(char *data, int32 data_len, NcColor *color) {
    int32 underscore;
    int16 foreground;
    int16 background;

    if (STREQUAL(data, data_len, "default")) {
        *color = nc_color_default();
        return 0;
    }
    if (STREQUAL(data, data_len, "end")) {
        *color = nc_color_end();
        return 0;
    }

    underscore = -1;
    for (int32 i = 0; i < data_len; i += 1) {
        if (data[i] == '_') {
            underscore = i;
            break;
        }
    }

    if (underscore < 0) {
        int32 status;

        status = ncm_format_parse_color_component(data, data_len,
                                                  false, &foreground);
        if (status < 0) {
            return status;
        }
        *color = nc_color_make(foreground, -2, false, false);
        return 0;
    }

    if (ncm_format_parse_color_component(data, underscore,
                                         false, &foreground) < 0) {
        return -NCM_ERROR_PARSE;
    }
    if (ncm_format_parse_color_component(data + underscore + 1,
                                         data_len - underscore - 1,
                                         true, &background) < 0) {
        return -NCM_ERROR_PARSE;
    }
    *color = nc_color_make(foreground, background, false, false);
    return 0;
}

static int32
ncm_format_read_uint(char *data, int32 start, int32 end, uint32 *value) {
    uint32 result;

    result = 0;
    if (start >= end) {
        return -NCM_ERROR_PARSE;
    }

    for (int32 i = start; i < end; i += 1) {
        uint32 digit;

        if (!isdigit((uint8)data[i])) {
            return -NCM_ERROR_PARSE;
        }
        digit = (uint32)(data[i] - '0');
        if (result > (MAXOF(result) - digit)/10) {
            return -EOVERFLOW;
        }
        result = result*10 + digit;
    }

    *value = result;
    return 0;
}

static int32
ncm_format_text_append(NcmFormatExprList *list, StrBuilder *token) {
    NcmFormatExpr *expr;

    if (token->len <= 0) {
        return 0;
    }

    if ((expr = ncm_format_expr_list_append(list)) == NULL) {
        return -ENOMEM;
    }

    expr->type = NCM_FORMAT_EXPR_TEXT;
    expr->value.text = *token;
    *token = (StrBuilder){0};

    return 0;
}

static int32
ncm_format_append_group_or_single(NcmFormatExprList *list,
                                  NcmFormatExprList *source) {
    NcmFormatExpr *expr;

    if (source->len == 1) {
        if ((expr = ncm_format_expr_list_append(list)) == NULL) {
            return -ENOMEM;
        }
        *expr = source->items[0];
        source->items[0].type = NCM_FORMAT_EXPR_TEXT;
        source->items[0].value.text = (StrBuilder){0};
        source->len = 0;
        return 0;
    }

    if ((expr = ncm_format_expr_list_append(list)) == NULL) {
        return -ENOMEM;
    }
    expr->type = NCM_FORMAT_EXPR_GROUP;
    ncm_format_expr_list_move(&expr->value.list, source);
    return 0;
}

static int32
ncm_format_expr_list_reserve(NcmFormatExprList *list, int32 extra) {
    int32 needed;
    int32 old_cap;
    int32 new_cap;

    if (extra <= 0) {
        return 0;
    }

    needed = list->len + extra;
    if (needed <= list->cap) {
        return 0;
    }

    old_cap = list->cap;
    new_cap = list->cap;
    if (new_cap <= 0) {
        new_cap = 8;
    }
    while (new_cap < needed) {
        new_cap *= 2;
    }

    list->items = realloc2(list->items,
                           old_cap, new_cap, SIZEOF(*list->items));
    list->cap = new_cap;
    return 0;
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
    free2(list->items, list->cap*SIZEOF(*list->items));
    *list = (NcmFormatExprList){0};

    return;
}

void
ncm_format_expr_list_move(NcmFormatExprList *dest, NcmFormatExprList *source) {
    *dest = *source;
    *source = (NcmFormatExprList){0};
    return;
}

NcmFormatExpr *
ncm_format_expr_list_append(NcmFormatExprList *list) {
    NcmFormatExpr *expr;

    if (ncm_format_expr_list_reserve(list, 1) < 0) {
        return NULL;
    }

    expr = &list->items[list->len];
    list->len += 1;
    *expr = (NcmFormatExpr){0};
    return expr;
}

static void
ncm_format_expr_destroy(NcmFormatExpr *expr) {
    switch (expr->type) {
    case NCM_FORMAT_EXPR_TEXT:
        sb_free(&expr->value.text);
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
    case NCM_FORMAT_EXPR_COUNT:
    default:
        break;
    }
    *expr = (NcmFormatExpr){0};
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

int32
ncm_format_ast_append_column_types(NcmFormatAst *ast,
                                   char *types, int32 types_len) {
    NcmFormatExpr *first;

    if ((ast == NULL) || (types_len < 0)) {
        return -EINVAL;
    }
    if ((types == NULL) && (types_len > 0)) {
        return -EINVAL;
    }
    if (types_len <= 0) {
        return 0;
    }

    for (int32 i = 0; i < types_len; i += 1) {
        if (ncm_song_getter_from_char(types[i]) == NCM_SONG_GETTER_NONE) {
            return -NCM_ERROR_PARSE;
        }
    }

    if ((first = ncm_format_expr_list_append(&ast->root)) == NULL) {
        return -ENOMEM;
    }
    first->type = NCM_FORMAT_EXPR_FIRST_OF;
    first->value.list = (NcmFormatExprList){0};

    for (int32 i = 0; i < types_len; i += 1) {
        NcmFormatExpr *tag;

        if ((tag = ncm_format_expr_list_append(&first->value.list)) == NULL) {
            return -ENOMEM;
        }
        tag->type = NCM_FORMAT_EXPR_SONG_TAG;
        tag->value.song_tag.getter = ncm_song_getter_from_char(types[i]);
        tag->value.song_tag.delimiter = 0;
    }
    return 0;
}

static int32
ncm_format_find_matching_brace(char *data, int32 start, int32 end,
                               int32 *result) {
    int32 depth = 1;

    for (int32 i = start + 1; i < end; i += 1) {
        if (data[i] == '{') {
            depth += 1;
        } else if (data[i] == '}') {
            depth -= 1;
            if (depth == 0) {
                *result = i;
                return 0;
            }
        }
    }

    return -NCM_ERROR_PARSE;
}

static int32
ncm_format_parse_dollar(NcmFormatExprList *out, char *data,
                        int32 *pos, int32 end, uint32 flags,
                        NcmError *ncm_error) {
    NcmFormatExpr *expr;
    int32 i = *pos + 1;

    if (i >= end) {
        return ncm_format_set_error(ncm_error, "unexpected end", i);
    }

    if (data[i] == '$') {
        if ((expr = ncm_format_expr_list_append(out)) == NULL) {
            return -ENOMEM;
        }
        expr->type = NCM_FORMAT_EXPR_TEXT;
        sb_append_byte(&expr->value.text, '$');
        *pos = i;
        return 0;
    }

    if ((expr = ncm_format_expr_list_append(out)) == NULL) {
        return -ENOMEM;
    }

    if ((flags & NCM_FORMAT_FLAG_COLOR)
        && isdigit((uint8)data[i])) {
        int32 color_index;

        color_index = ncm_color_index_from_char(data[i]);
        if (ncm_format_color_index_to_nc_color(color_index,
                                               &expr->value.color) < 0) {
            return ncm_format_set_error(ncm_error, "invalid color", i);
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
            return ncm_format_set_error(ncm_error, "unexpected end", i);
        }
        if (ncm_format_parse_named_color(data + color_start,
                                         i - color_start,
                                         &expr->value.color) < 0) {
            return ncm_format_set_error(ncm_error,
                                        "invalid color", color_start);
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
            return ncm_format_set_error(ncm_error, "unexpected end", i);
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
            return ncm_format_set_error(ncm_error, "invalid format", i);
        }
    } else {
        return ncm_format_set_error(ncm_error, "invalid character", i);
    }

    *pos = i;
    return 0;
}

static int32
ncm_format_parse_percent(NcmFormatExprList *out, char *data,
                         int32 *pos, int32 end, NcmError *ncm_error) {
    NcmFormatExpr *expr;
    uint32 delimiter;
    int32 i;
    int32 delimiter_start;
    int32 status;
    enum NcmSongGetter getter;

    i = *pos + 1;
    if (i >= end) {
        return ncm_format_set_error(ncm_error, "unexpected end", i);
    }

    if (data[i] == '%') {
        if ((expr = ncm_format_expr_list_append(out)) == NULL) {
            return -ENOMEM;
        }
        expr->type = NCM_FORMAT_EXPR_TEXT;
        sb_append_byte(&expr->value.text, '%');
        *pos = i;
        return 0;
    }

    delimiter = 0;
    if (isdigit((uint8)data[i])) {
        delimiter_start = i;
        while ((i < end) && isdigit((uint8)data[i])) {
            i += 1;
        }
        if (i >= end) {
            return ncm_format_set_error(ncm_error, "unexpected end", i);
        }
        status = ncm_format_read_uint(data, delimiter_start, i, &delimiter);
        if (status == -EOVERFLOW) {
            return ncm_error_set_status(ncm_error, status,
                                        STRLIT("tag delimiter too large"));
        }
        if (status < 0) {
            return ncm_format_set_error(ncm_error, "invalid tag delimiter", i);
        }
    }

    getter = ncm_song_getter_from_char(data[i]);
    if (getter == NCM_SONG_GETTER_NONE) {
        return ncm_format_set_error(ncm_error, "invalid tag", i);
    }

    if ((expr = ncm_format_expr_list_append(out)) == NULL) {
        return -ENOMEM;
    }
    expr->type = NCM_FORMAT_EXPR_SONG_TAG;
    expr->value.song_tag.getter = getter;
    expr->value.song_tag.delimiter = delimiter;
    *pos = i;

    return 0;
}

static int32
ncm_format_parse_first_of(NcmFormatExprList *out, char *data,
                          int32 *pos, int32 end, uint32 flags,
                          NcmError *ncm_error) {
    NcmFormatExpr *first;
    bool done;
    int32 i;
    int32 status;

    if ((first = ncm_format_expr_list_append(out)) == NULL) {
        return -ENOMEM;
    }
    first->type = NCM_FORMAT_EXPR_FIRST_OF;
    first->value.list = (NcmFormatExprList){0};

    i = *pos;
    done = false;
    while (!done) {
        NcmFormatExprList inner;
        int32 close;

        if (ncm_format_find_matching_brace(data, i, end, &close) < 0) {
            return ncm_format_set_error(ncm_error, "unexpected end", i);
        }

        inner = (NcmFormatExprList){0};
        status = ncm_format_parse_bracket(&inner, data, i + 1, close,
                                          flags, ncm_error);
        if (status < 0) {
            ncm_format_expr_list_destroy(&inner);
            return status;
        }
        status = ncm_format_append_group_or_single(&first->value.list,
                                                   &inner);
        if (status < 0) {
            ncm_format_expr_list_destroy(&inner);
            return status;
        }
        ncm_format_expr_list_destroy(&inner);

        i = close + 1;
        if ((i < end) && (data[i] == '|')) {
            i += 1;
            if ((i >= end) || (data[i] != '{')) {
                return ncm_format_set_error(ncm_error, "expected bracket", i);
            }
        } else {
            done = true;
        }
    }

    *pos = i - 1;
    return 0;
}

static int32
ncm_format_parse_bracket(NcmFormatExprList *out, char *data,
                         int32 start, int32 end, uint32 flags,
                         NcmError *ncm_error) {
    StrBuilder token = {0};
    int32 status;

    status = 0;
    for (int32 i = start; (status == 0) && (i < end); i += 1) {
        if (data[i] == '{') {
            status = ncm_format_text_append(out, &token);
            if (status == 0) {
                status = ncm_format_parse_first_of(out, data, &i, end,
                                                   flags, ncm_error);
            }
        } else if ((flags & NCM_FORMAT_FLAG_TAG) && (data[i] == '%')) {
            status = ncm_format_text_append(out, &token);
            if (status == 0) {
                status = ncm_format_parse_percent(out, data, &i, end,
                                                  ncm_error);
            }
        } else if ((flags & (NCM_FORMAT_FLAG_COLOR
                             |NCM_FORMAT_FLAG_FORMAT
                             |NCM_FORMAT_FLAG_OUTPUT_SWITCH))
                   && (data[i] == '$')) {
            status = ncm_format_text_append(out, &token);
            if (status == 0) {
                status = ncm_format_parse_dollar(out, data, &i, end,
                                                 flags, ncm_error);
            }
        } else {
            sb_append_byte(&token, data[i]);
        }
    }

    if (status == 0) {
        status = ncm_format_text_append(out, &token);
    }
    sb_free(&token);
    return status;
}

int32
ncm_format_parse(NcmFormatAst *ast, char *data, int32 data_len,
                 uint32 flags, NcmError *ncm_error) {
    NcmFormatAst tmp;
    int32 status;

    if ((ast == NULL) || (data_len < 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("invalid format input"));
    }
    if ((data == NULL) && (data_len > 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("invalid format input"));
    }

    tmp = (NcmFormatAst){0};
    status = ncm_format_parse_bracket(&tmp.root, data, 0, data_len,
                                      flags, ncm_error);
    if (status < 0) {
        ncm_format_ast_destroy(&tmp);
        return status;
    }

    ncm_format_ast_move(ast, &tmp);
    ncm_format_ast_destroy(&tmp);
    return 0;
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

StrBuilder
ncm_format_render_tag(NcmSong *song, NcmFormatSongTag *tag) {
    StrBuilder result = {0};

    if ((song == NULL) || (tag == NULL)) {
        return result;
    }

    result = ncm_song_tags_buffer(song, tag->getter, STRLIT(" | "), true);
    if ((tag->delimiter > 0) && (result.len > 0)) {
        int32 limit;

        if (tag->delimiter > (uint32)MAXOF(limit)) {
            limit = MAXOF(limit);
        } else {
            limit = (int32)tag->delimiter;
        }

        if ((tag->getter == NCM_SONG_GETTER_DATE)
            || (tag->getter == NCM_SONG_GETTER_LENGTH)) {
            if (limit < result.len) {
                result.len = limit;
            }
        } else {
            result.len = utf8_cut_width(result.data, result.len, limit);
        }
        result.data[result.len] = '\0';
    }

    return result;
}

static void
ncm_format_emit_text(NcmFormatCallbacks *cb, void *user,
                     char *data, int32 data_len,
                     NcmFormatSongTag *tag) {
    if (cb && cb->text && (data_len > 0)) {
        cb->text(user, data, data_len, tag);
    }
    return;
}

static void
ncm_format_emit_color(NcmFormatCallbacks *cb, void *user, NcColor color) {
    if (cb && cb->color) {
        cb->color(user, color);
    }
    return;
}

static void
ncm_format_emit_format(NcmFormatCallbacks *cb, void *user,
                       enum NcFormat format) {
    if (cb && cb->format) {
        cb->format(user, format);
    }
    return;
}

static void *
ncm_format_current_output(void *left, void *right, bool switched) {
    if (switched && right) {
        return right;
    }
    return left;
}

static enum NcmFormatResult
ncm_format_render_expr(NcmFormatExpr *expr, NcmSong *song,
                       NcmFormatCallbacks *cb,
                       void *left, void *right,
                       uint32 flags,
                       int32 *no_output, bool *switched) {
    void *output = ncm_format_current_output(left, right, *switched);
    StrBuilder tag;
    enum NcmFormatResult result;

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
            sb_free(&tag);
            return NCM_FORMAT_RESULT_MISSING;
        }
        if (*no_output <= 0) {
            ncm_format_emit_text(cb, output, tag.data, tag.len,
                                 &expr->value.song_tag);
        }
        sb_free(&tag);
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
    case NCM_FORMAT_EXPR_COUNT:
    default:
        break;
    }

    return NCM_FORMAT_RESULT_EMPTY;
}

static enum NcmFormatResult
ncm_format_render_list(NcmFormatExprList *list, NcmSong *song,
                       NcmFormatCallbacks *cb, void *left,
                       void *right, uint32 flags, int32 *no_output,
                       bool *switched) {
    enum NcmFormatResult result = NCM_FORMAT_RESULT_EMPTY;

    for (int32 i = 0; i < list->len; i += 1) {
        enum NcmFormatResult part = ncm_format_render_expr(&list->items[i],
                                                           song, cb,
                                                           left, right,
                                                           flags,
                                                           no_output, switched);

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
    int32 no_output = 0;
    bool switched = false;

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

    nc_buffer_add_color(buffer, nc_buffer_len(buffer), color, MAXOF((int64)0));
    return;
}

static void
ncm_format_buffer_format(void *user, enum NcFormat format) {
    NcBuffer *buffer = (NcBuffer *)user;

    nc_buffer_add_format(buffer, nc_buffer_len(buffer),
                         format, MAXOF((int64)0));
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
    SB_APPEND((StrBuilder *)user, data, data_len);
    return;
}

StrBuilder
ncm_format_render_string(NcmFormatAst *ast, NcmSong *song) {
    NcmFormatCallbacks callbacks;
    StrBuilder result = {0};

    callbacks.text = ncm_format_string_text;
    callbacks.color = NULL;
    callbacks.format = NULL;

    ncm_format_render(ast, song,
                      &callbacks, &result, &result,
                      NCM_FORMAT_FLAG_TAG);

    return result;
}

#endif /* NCM_FORMAT_C */
