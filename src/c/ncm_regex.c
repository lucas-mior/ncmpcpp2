#if !defined(NCM_REGEX_C)
#define NCM_REGEX_C

#include "cbase.h"

#include "c/ncm_c.h"

static bool
ncm_regex_prepare_string(char *string, int32 string_len, StrBuilder *buffer,
                         NcmError *ncm_error) {
    if (buffer == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing regex buffer"));
        return false;
    }
    sb_clear(buffer);

    if (string == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing regex string"));
        return false;
    }
    if (string_len < 0) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("negative string length"));
        return false;
    }

    SB_APPEND(buffer, string, string_len);
    if (buffer->data == NULL) {
        sb_reserve(buffer, 1);
        buffer->data[0] = '\0';
    }
    return true;
}

static void
ncm_regex_set_error(NcmRegex *regex, int32 code, NcmError *ncm_error) {
    char message[256];
    int32 message_len;

    if (regex == NULL) {
        ncm_error_set(ncm_error, code, STRLIT("regex error"));
        return;
    }

    message_len = (int32)regerror(code, &regex->regex,
                                  message, SIZEOF(message));
    if (message_len > 0) {
        message_len -= 1;
    }
    ncm_error_set(ncm_error, code, message, message_len);
    return;
}

void
ncm_regex_init(NcmRegex *regex) {
    regex->compiled = false;
    regex->flags = 0;
    return;
}

void
ncm_regex_destroy(NcmRegex *regex) {
    if (regex == NULL) {
        return;
    }
    if (regex->compiled) {
        regfree(&regex->regex);
    }
    regex->compiled = false;
    regex->flags = 0;
    return;
}

void
ncm_regex_escape_literal(StrBuilder *buffer, char *pattern, int32 pattern_len) {
    char c;

    sb_clear(buffer);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        return;
    }

    for (int32 i = 0; i < pattern_len; i += 1) {
        c = pattern[i];
        switch (c) {
        case '\\':
        case '^':
        case '$':
        case '.':
        case '|':
        case '?':
        case '*':
        case '+':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
            sb_append_byte(buffer, '\\');
            break;
        default:
            break;
        }
        sb_append_byte(buffer, c);
    }
    return;
}

bool
ncm_regex_compile(NcmRegex *regex, char *pattern, int32 pattern_len,
                  uint32 flags, NcmError *ncm_error) {
    StrBuilder escaped = {0};
    StrBuilder compiled_pattern = {0};
    int32 reg_flags;
    int32 code;

    if (regex == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing regex"));
        return false;
    }
    if (pattern == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing regex pattern"));
        return false;
    }
    if (pattern_len < 0) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("negative pattern length"));
        return false;
    }

    ncm_regex_destroy(regex);

    if (pattern_len == 0) {
        SB_APPEND(&compiled_pattern, STRLIT("^"));
    } else if (flags & NCM_REGEX_LITERAL) {
        ncm_regex_escape_literal(&escaped, pattern, pattern_len);
        SB_APPEND(&compiled_pattern, escaped.data, escaped.len);
    } else {
        SB_APPEND(&compiled_pattern, pattern, pattern_len);
    }

    reg_flags = 0;
    if (flags & NCM_REGEX_EXTENDED) {
        reg_flags |= REG_EXTENDED;
    }
    if (flags & NCM_REGEX_ICASE) {
        reg_flags |= REG_ICASE;
    }
    if (flags & NCM_REGEX_NOSUB) {
        reg_flags |= REG_NOSUB;
    }

    if (compiled_pattern.data == NULL) {
        sb_reserve(&compiled_pattern, 1);
        compiled_pattern.data[0] = '\0';
    }

    code = regcomp(&regex->regex, compiled_pattern.data, reg_flags);
    sb_free(&compiled_pattern);
    sb_free(&escaped);
    if (code != 0) {
        ncm_regex_set_error(regex, code, ncm_error);
        return false;
    }

    regex->compiled = true;
    regex->flags = flags;
    ncm_error_clear(ncm_error);
    return true;
}

bool
ncm_regex_search(NcmRegex *regex, char *string, int32 string_len) {
    StrBuilder buffer = {0};
    bool result;

    if ((regex == NULL) || !regex->compiled) {
        return false;
    }

    if (!ncm_regex_prepare_string(string, string_len, &buffer, NULL)) {
        sb_free(&buffer);
        return false;
    }

    result = regexec(&regex->regex, buffer.data, 0, NULL, 0) == 0;
    sb_free(&buffer);
    return result;
}

bool
ncm_regex_for_each_match(NcmRegex *regex, char *string, int32 string_len,
                         NcmRegexMatchCallback *callback, void *user) {
    StrBuilder buffer = {0};
    regmatch_t match[1];
    char *cursor;
    int32 offset;
    int32 match_start;
    int32 match_len;
    bool matched;

    if ((regex == NULL) || !regex->compiled) {
        return false;
    }
    if (callback == NULL) {
        return false;
    }

    if (!ncm_regex_prepare_string(string, string_len, &buffer, NULL)) {
        sb_free(&buffer);
        return false;
    }

    matched = false;
    cursor = buffer.data;
    offset = 0;
    while (offset <= string_len) {
        if (regexec(&regex->regex, cursor, 1, match, 0) != 0) {
            break;
        }
        if (match[0].rm_so < 0) {
            break;
        }

        match_start = offset + (int32)match[0].rm_so;
        match_len = (int32)(match[0].rm_eo - match[0].rm_so);
        matched = true;
        if (!callback(match_start, match_len, user)) {
            break;
        }

        if (match_len <= 0) {
            if (cursor[0] == '\0') {
                break;
            }
            cursor += 1;
            offset += 1;
        } else {
            cursor += match[0].rm_eo;
            offset += (int32)match[0].rm_eo;
        }
    }

    sb_free(&buffer);
    return matched;
}

#endif /* NCM_REGEX_C */
