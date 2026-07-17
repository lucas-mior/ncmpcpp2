#if !defined(NCM_FORMAT_H)
#define NCM_FORMAT_H

#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "c/ncm_song.h"
#include "curses/nc_buffer.h"

enum NcmFormatFlags {
    NCM_FORMAT_FLAG_NONE = 0,
    NCM_FORMAT_FLAG_COLOR = 1,
    NCM_FORMAT_FLAG_FORMAT = 2,
    NCM_FORMAT_FLAG_OUTPUT_SWITCH = 4,
    NCM_FORMAT_FLAG_TAG = 8,
    NCM_FORMAT_FLAG_ALL = NCM_FORMAT_FLAG_COLOR
                          |NCM_FORMAT_FLAG_FORMAT
                          |NCM_FORMAT_FLAG_OUTPUT_SWITCH
                          |NCM_FORMAT_FLAG_TAG,
};

enum NcmFormatExprType {
    NCM_FORMAT_EXPR_TEXT,
    NCM_FORMAT_EXPR_COLOR,
    NCM_FORMAT_EXPR_FORMAT,
    NCM_FORMAT_EXPR_OUTPUT_SWITCH,
    NCM_FORMAT_EXPR_SONG_TAG,
    NCM_FORMAT_EXPR_GROUP,
    NCM_FORMAT_EXPR_FIRST_OF,
};

enum NcmFormatResult {
    NCM_FORMAT_RESULT_EMPTY,
    NCM_FORMAT_RESULT_MISSING,
    NCM_FORMAT_RESULT_OK,
};

typedef struct NcmFormatSongTag {
    enum NcmSongGetter getter;
    uint32 delimiter;
} NcmFormatSongTag;

typedef struct NcmFormatExpr NcmFormatExpr;

typedef struct NcmFormatExprList {
    NcmFormatExpr *items;
    int32 len;
    int32 cap;
} NcmFormatExprList;

struct NcmFormatExpr {
    enum NcmFormatExprType type;
    union {
        NcmBuffer text;
        NcColor color;
        enum NcFormat format;
        NcmFormatSongTag song_tag;
        NcmFormatExprList list;
    } value;
};

typedef struct NcmFormatAst {
    NcmFormatExprList root;
} NcmFormatAst;

typedef struct NcmFormatCallbacks {
    void (*text)(void *user, char *data, int32 data_len,
                 NcmFormatSongTag *tag);
    void (*color)(void *user, NcColor color);
    void (*format)(void *user, enum NcFormat format);
} NcmFormatCallbacks;

void ncm_format_expr_list_init(NcmFormatExprList *list);
void ncm_format_expr_list_destroy(NcmFormatExprList *list);
void ncm_format_expr_list_clear(NcmFormatExprList *list);
void ncm_format_expr_list_move(NcmFormatExprList *dest,
                               NcmFormatExprList *source);
NcmFormatExpr *ncm_format_expr_list_append(NcmFormatExprList *list);

void ncm_format_ast_init(NcmFormatAst *ast);
void ncm_format_ast_destroy(NcmFormatAst *ast);
void ncm_format_ast_clear(NcmFormatAst *ast);
void ncm_format_ast_move(NcmFormatAst *dest, NcmFormatAst *source);
bool ncm_format_ast_append_first_of_getters(NcmFormatAst *ast,
                                            enum NcmSongGetter *getters,
                                            int32 getters_len);

bool ncm_format_parse(NcmFormatAst *ast, char *data, int32 data_len,
                      uint32 flags, NcmError *error);

void ncm_format_render(NcmFormatAst *ast, NcmSong *song,
                       NcmFormatCallbacks *callbacks, void *output,
                       void *second_output, uint32 flags);
void ncm_format_render_buffer(NcmFormatAst *ast, NcmSong *song,
                              NcBuffer *buffer, NcBuffer *right_aligned,
                              uint32 flags);
NcmBuffer ncm_format_render_string(NcmFormatAst *ast, NcmSong *song);
NcmBuffer ncm_format_render_tag(NcmSong *song, NcmFormatSongTag *tag);

#endif /* NCM_FORMAT_H */
