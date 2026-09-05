#if !defined(NCM_FORMAT_C)
#define NCM_FORMAT_C

#include "cbase.h"

#include "c/ncm_c.h"

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

static void
ncm_format_text_append(NcmFormatExprList *list, StrBuilder *token) {
    NcmFormatExpr *expr;

    if (token->len <= 0) {
        return;
    }

    expr = ncm_format_expr_list_append(list);
    expr->type = NCM_FORMAT_EXPR_TEXT;
    expr->value.text = *token;
    *token = (StrBuilder){0};

    return;
}

static void
ncm_format_expr_list_clear_unchecked(NcmFormatExprList *list) {
    for (int32 i = 0; i < list->len; i += 1) {
        NcmFormatExpr *expr = &list->items[i];

        switch (expr->type) {
        case NCM_FORMAT_EXPR_TEXT:
            sb_free(&expr->value.text);
            break;
        case NCM_FORMAT_EXPR_GROUP:
        case NCM_FORMAT_EXPR_FIRST_OF:
            ncm_format_expr_list_clear_unchecked(&expr->value.list);
            free2(expr->value.list.items,
                  expr->value.list.cap*SIZEOF(*expr->value.list.items));
            expr->value.list = (NcmFormatExprList){0};
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
    }
    list->len = 0;
    return;
}

void
ncm_format_expr_list_clear(NcmFormatExprList *list) {
    if (list == NULL) {
        return;
    }

    ncm_format_expr_list_clear_unchecked(list);
    return;
}

static void
ncm_format_expr_list_destroy_unchecked(NcmFormatExprList *list) {
    ncm_format_expr_list_clear_unchecked(list);
    free2(list->items, list->cap*SIZEOF(*list->items));
    *list = (NcmFormatExprList){0};
    return;
}

void
ncm_format_expr_list_destroy(NcmFormatExprList *list) {
    if (list == NULL) {
        return;
    }

    ncm_format_expr_list_destroy_unchecked(list);
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

    if (list->len >= list->cap) {
        int32 needed = list->len + 1;
        int32 old_cap = list->cap;
        int32 new_cap = list->cap;

        if (new_cap <= 0) {
            new_cap = 8;
        }
        while (new_cap < needed) {
            new_cap *= 2;
        }

        list->items = realloc2(list->items,
                               old_cap, new_cap, SIZEOF(*list->items));
        list->cap = new_cap;
    }

    expr = &list->items[list->len];
    list->len += 1;
    *expr = (NcmFormatExpr){0};
    return expr;
}

void
ncm_format_ast_destroy(NcmFormatAst *ast) {
    if (ast == NULL) {
        return;
    }
    ncm_format_expr_list_destroy_unchecked(&ast->root);
    return;
}

void
ncm_format_ast_clear(NcmFormatAst *ast) {
    if (ast == NULL) {
        return;
    }
    ncm_format_expr_list_clear_unchecked(&ast->root);
    return;
}

void
ncm_format_ast_move(NcmFormatAst *dest, NcmFormatAst *source) {
    ncm_format_expr_list_destroy_unchecked(&dest->root);
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

    first = ncm_format_expr_list_append(&ast->root);
    first->type = NCM_FORMAT_EXPR_FIRST_OF;
    first->value.list = (NcmFormatExprList){0};

    for (int32 i = 0; i < types_len; i += 1) {
        NcmFormatExpr *tag;

        tag = ncm_format_expr_list_append(&first->value.list);
        tag->type = NCM_FORMAT_EXPR_SONG_TAG;
        tag->value.song_tag.getter = ncm_song_getter_from_char(types[i]);
        tag->value.song_tag.delimiter = 0;
    }
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
            NcmFormatExpr *first;
            bool done;
            int32 first_i;

            ncm_format_text_append(out, &token);
            first = ncm_format_expr_list_append(out);
            first->type = NCM_FORMAT_EXPR_FIRST_OF;
            first->value.list = (NcmFormatExprList){0};

            first_i = i;
            done = false;
            while ((status == 0) && !done) {
                NcmFormatExprList inner = {0};
                int32 close = -1;
                int32 depth = 1;

                for (int32 j = first_i + 1; j < end; j += 1) {
                    if (data[j] == '{') {
                        depth += 1;
                    } else if (data[j] == '}') {
                        depth -= 1;
                        if (depth == 0) {
                            close = j;
                            break;
                        }
                    }
                }
                if (close < 0) {
                    status = ncm_format_set_error(
                        ncm_error, "unexpected end", first_i);
                    break;
                }

                status = ncm_format_parse_bracket(
                    &inner, data, first_i + 1, close, flags, ncm_error);
                if (status == 0) {
                    NcmFormatExpr *expr;

                    expr = ncm_format_expr_list_append(&first->value.list);
                    if (inner.len == 1) {
                        *expr = inner.items[0];
                        inner.items[0].type = NCM_FORMAT_EXPR_TEXT;
                        inner.items[0].value.text = (StrBuilder){0};
                        inner.len = 0;
                    } else {
                        expr->type = NCM_FORMAT_EXPR_GROUP;
                        ncm_format_expr_list_move(&expr->value.list, &inner);
                    }
                }
                ncm_format_expr_list_destroy_unchecked(&inner);
                if (status < 0) {
                    break;
                }

                first_i = close + 1;
                if ((first_i < end) && (data[first_i] == '|')) {
                    first_i += 1;
                    if ((first_i >= end) || (data[first_i] != '{')) {
                        status = ncm_format_set_error(
                            ncm_error, "expected bracket", first_i);
                    }
                } else {
                    done = true;
                }
            }
            if (status == 0) {
                i = first_i - 1;
            }
        } else if ((flags & NCM_FORMAT_FLAG_TAG) && (data[i] == '%')) {
            NcmFormatExpr *expr;
            uint32 delimiter;
            int32 percent_i;
            enum NcmSongGetter getter;

            ncm_format_text_append(out, &token);
            percent_i = i + 1;
            if (percent_i >= end) {
                status = ncm_format_set_error(
                    ncm_error, "unexpected end", percent_i);
            } else if (data[percent_i] == '%') {
                expr = ncm_format_expr_list_append(out);
                expr->type = NCM_FORMAT_EXPR_TEXT;
                sb_append_byte(&expr->value.text, '%');
                i = percent_i;
            } else {
                delimiter = 0;
                if (isdigit((uint8)data[percent_i])) {
                    int32 delimiter_start = percent_i;

                    while ((percent_i < end)
                           && isdigit((uint8)data[percent_i])) {
                        percent_i += 1;
                    }
                    if (percent_i >= end) {
                        status = ncm_format_set_error(
                            ncm_error, "unexpected end", percent_i);
                    } else {
                        for (int32 j = delimiter_start;
                             (status == 0) && (j < percent_i); j += 1) {
                            uint32 digit = (uint32)(data[j] - '0');

                            if (delimiter > (MAXOF(delimiter) - digit)/10) {
                                status = ncm_error_set_status(
                                    ncm_error, -EOVERFLOW,
                                    STRLIT("tag delimiter too large"));
                            } else {
                                delimiter = delimiter*10 + digit;
                            }
                        }
                    }
                }

                if (status == 0) {
                    getter = ncm_song_getter_from_char(data[percent_i]);
                    if (getter == NCM_SONG_GETTER_NONE) {
                        status = ncm_format_set_error(
                            ncm_error, "invalid tag", percent_i);
                    } else {
                        expr = ncm_format_expr_list_append(out);
                        expr->type = NCM_FORMAT_EXPR_SONG_TAG;
                        expr->value.song_tag.getter = getter;
                        expr->value.song_tag.delimiter = delimiter;
                        i = percent_i;
                    }
                }
            }
        } else if ((flags & (NCM_FORMAT_FLAG_COLOR
                             |NCM_FORMAT_FLAG_FORMAT
                             |NCM_FORMAT_FLAG_OUTPUT_SWITCH))
                   && (data[i] == '$')) {
            NcmFormatExpr *expr;
            int32 dollar_i = i + 1;

            ncm_format_text_append(out, &token);
            if (dollar_i >= end) {
                status = ncm_format_set_error(
                    ncm_error, "unexpected end", dollar_i);
            } else if (data[dollar_i] == '$') {
                expr = ncm_format_expr_list_append(out);
                expr->type = NCM_FORMAT_EXPR_TEXT;
                sb_append_byte(&expr->value.text, '$');
                i = dollar_i;
            } else {
                expr = ncm_format_expr_list_append(out);
                if ((flags & NCM_FORMAT_FLAG_COLOR)
                    && isdigit((uint8)data[dollar_i])) {
                    int32 color_index;

                    color_index = ncm_color_index_from_char(data[dollar_i]);
                    switch (color_index) {
                    case 0:
                        expr->value.color = nc_color_default();
                        break;
                    case 1:
                        expr->value.color = nc_color_make(
                            COLOR_BLACK, -1, false, false);
                        break;
                    case 2:
                        expr->value.color = nc_color_make(
                            COLOR_RED, -1, false, false);
                        break;
                    case 3:
                        expr->value.color = nc_color_make(
                            COLOR_GREEN, -1, false, false);
                        break;
                    case 4:
                        expr->value.color = nc_color_make(
                            COLOR_YELLOW, -1, false, false);
                        break;
                    case 5:
                        expr->value.color = nc_color_make(
                            COLOR_BLUE, -1, false, false);
                        break;
                    case 6:
                        expr->value.color = nc_color_make(
                            COLOR_MAGENTA, -1, false, false);
                        break;
                    case 7:
                        expr->value.color = nc_color_make(
                            COLOR_CYAN, -1, false, false);
                        break;
                    case 8:
                        expr->value.color = nc_color_make(
                            COLOR_WHITE, -1, false, false);
                        break;
                    case 9:
                        expr->value.color = nc_color_end();
                        break;
                    default:
                        status = ncm_format_set_error(
                            ncm_error, "invalid color", dollar_i);
                        break;
                    }
                    if (status == 0) {
                        expr->type = NCM_FORMAT_EXPR_COLOR;
                        i = dollar_i;
                    }
                } else if ((flags & NCM_FORMAT_FLAG_COLOR)
                           && (data[dollar_i] == '(')) {
                    char *color_data;
                    int32 color_len;
                    int32 color_start;
                    int32 underscore;
                    int16 foreground;
                    int16 background;
                    int32 color_status;

                    dollar_i += 1;
                    color_start = dollar_i;
                    while ((dollar_i < end) && (data[dollar_i] != ')')) {
                        dollar_i += 1;
                    }
                    if (dollar_i >= end) {
                        status = ncm_format_set_error(
                            ncm_error, "unexpected end", dollar_i);
                        continue;
                    }

                    color_data = data + color_start;
                    color_len = dollar_i - color_start;
                    color_status = 0;
                    if (STREQUAL(color_data, color_len, "default")) {
                        expr->value.color = nc_color_default();
                    } else if (STREQUAL(color_data, color_len, "end")) {
                        expr->value.color = nc_color_end();
                    } else {
                        underscore = -1;
                        for (int32 j = 0; j < color_len; j += 1) {
                            if (color_data[j] == '_') {
                                underscore = j;
                                break;
                            }
                        }

                        if (underscore < 0) {
                            color_status = ncm_format_parse_color_component(
                                color_data, color_len, false, &foreground);
                            if (color_status == 0) {
                                expr->value.color = nc_color_make(
                                    foreground, -2, false, false);
                            }
                        } else if (ncm_format_parse_color_component(
                                       color_data, underscore,
                                       false, &foreground) < 0) {
                            color_status = -NCM_ERROR_PARSE;
                        } else if (ncm_format_parse_color_component(
                                       color_data + underscore + 1,
                                       color_len - underscore - 1,
                                       true, &background) < 0) {
                            color_status = -NCM_ERROR_PARSE;
                        } else {
                            expr->value.color = nc_color_make(
                                foreground, background, false, false);
                        }
                    }

                    if (color_status < 0) {
                        status = ncm_format_set_error(
                            ncm_error, "invalid color", color_start);
                    } else {
                        expr->type = NCM_FORMAT_EXPR_COLOR;
                        i = dollar_i;
                    }
                } else if ((flags & NCM_FORMAT_FLAG_OUTPUT_SWITCH)
                           && (data[dollar_i] == 'R')) {
                    expr->type = NCM_FORMAT_EXPR_OUTPUT_SWITCH;
                    i = dollar_i;
                } else if ((flags & NCM_FORMAT_FLAG_FORMAT)
                           && (data[dollar_i] == 'b')) {
                    expr->type = NCM_FORMAT_EXPR_FORMAT;
                    expr->value.format = NC_FORMAT_BOLD;
                    i = dollar_i;
                } else if ((flags & NCM_FORMAT_FLAG_FORMAT)
                           && (data[dollar_i] == 'u')) {
                    expr->type = NCM_FORMAT_EXPR_FORMAT;
                    expr->value.format = NC_FORMAT_UNDERLINE;
                    i = dollar_i;
                } else if ((flags & NCM_FORMAT_FLAG_FORMAT)
                           && (data[dollar_i] == 'a')) {
                    expr->type = NCM_FORMAT_EXPR_FORMAT;
                    expr->value.format = NC_FORMAT_ALT_CHARSET;
                    i = dollar_i;
                } else if ((flags & NCM_FORMAT_FLAG_FORMAT)
                           && (data[dollar_i] == 'r')) {
                    expr->type = NCM_FORMAT_EXPR_FORMAT;
                    expr->value.format = NC_FORMAT_REVERSE;
                    i = dollar_i;
                } else if ((flags & NCM_FORMAT_FLAG_FORMAT)
                           && (data[dollar_i] == 'i')) {
                    expr->type = NCM_FORMAT_EXPR_FORMAT;
                    expr->value.format = NC_FORMAT_ITALIC;
                    i = dollar_i;
                } else if ((flags & NCM_FORMAT_FLAG_FORMAT)
                           && (data[dollar_i] == '/')) {
                    dollar_i += 1;
                    if (dollar_i >= end) {
                        status = ncm_format_set_error(
                            ncm_error, "unexpected end", dollar_i);
                    } else {
                        expr->type = NCM_FORMAT_EXPR_FORMAT;
                        if (data[dollar_i] == 'b') {
                            expr->value.format = NC_FORMAT_NO_BOLD;
                        } else if (data[dollar_i] == 'u') {
                            expr->value.format = NC_FORMAT_NO_UNDERLINE;
                        } else if (data[dollar_i] == 'a') {
                            expr->value.format = NC_FORMAT_NO_ALT_CHARSET;
                        } else if (data[dollar_i] == 'r') {
                            expr->value.format = NC_FORMAT_NO_REVERSE;
                        } else if (data[dollar_i] == 'i') {
                            expr->value.format = NC_FORMAT_NO_ITALIC;
                        } else {
                            status = ncm_format_set_error(
                                ncm_error, "invalid format", dollar_i);
                        }
                        if (status == 0) {
                            i = dollar_i;
                        }
                    }
                } else {
                    status = ncm_format_set_error(
                        ncm_error, "invalid character", dollar_i);
                }
            }
        } else {
            sb_append_byte(&token, data[i]);
        }
    }

    if (status == 0) {
        ncm_format_text_append(out, &token);
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
        ncm_format_expr_list_destroy_unchecked(&tmp.root);
        return status;
    }

    ncm_format_ast_move(ast, &tmp);
    ncm_format_expr_list_destroy_unchecked(&tmp.root);
    return 0;
}

static StrBuilder
ncm_format_render_tag_unchecked(NcmSong *song, NcmFormatSongTag *tag) {
    StrBuilder result;

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

StrBuilder
ncm_format_render_tag(NcmSong *song, NcmFormatSongTag *tag) {
    if ((song == NULL) || (tag == NULL)) {
        return (StrBuilder){0};
    }

    return ncm_format_render_tag_unchecked(song, tag);
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

static enum NcmFormatResult
ncm_format_render_expr(NcmFormatExpr *expr, NcmSong *song,
                       NcmFormatCallbacks *cb,
                       void *left, void *right,
                       uint32 flags,
                       int32 *no_output, bool *switched) {
    void *output = left;
    StrBuilder tag;
    enum NcmFormatResult result;

    if (*switched && right) {
        output = right;
    }

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
        if ((*no_output <= 0) && (flags & NCM_FORMAT_FLAG_COLOR)
            && cb && cb->color) {
            cb->color(output, expr->value.color);
        }
        return NCM_FORMAT_RESULT_EMPTY;
    case NCM_FORMAT_EXPR_FORMAT:
        if ((*no_output <= 0) && (flags & NCM_FORMAT_FLAG_FORMAT)
            && cb && cb->format) {
            cb->format(output, expr->value.format);
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
        tag = ncm_format_render_tag_unchecked(song, &expr->value.song_tag);
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

        if ((result == NCM_FORMAT_RESULT_MISSING)
            || (part == NCM_FORMAT_RESULT_MISSING)) {
            result = NCM_FORMAT_RESULT_MISSING;
        } else if ((result == NCM_FORMAT_RESULT_OK)
                   || (part == NCM_FORMAT_RESULT_OK)) {
            result = NCM_FORMAT_RESULT_OK;
        } else {
            result = NCM_FORMAT_RESULT_EMPTY;
        }
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

    ncm_format_render_list(&ast->root, song, callbacks, output,
                           second_output, flags, &no_output, &switched);
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
    NcBuffer *buffer = user;
    nc_buffer_add_color(buffer, buffer->len, color, MAXOF((int64)0));
    return;
}

static void
ncm_format_buffer_format(void *user, enum NcFormat format) {
    NcBuffer *buffer = user;
    nc_buffer_add_format(buffer, buffer->len,
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
