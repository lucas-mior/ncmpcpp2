#if !defined(NCMPCPP2_H)
#define NCMPCPP2_H

#include "cbase.h"
#include "tags.h"

typedef struct StrView {
    int32 len;
    char *data;
} StrView;

typedef struct StrFlex {
    int32 len;
    char data[];
} StrFlex;

typedef struct StrViewList {
    StrView *items;
    Arena *arena;
} StrViewList;

typedef struct StrFlexList {
    StrFlex **items;
    Arena *arena;
} StrFlexList;

void strview_list_push(StrViewList *, char *, int32);
void strview_list_destroy(StrViewList *);
void strview_list_clear(StrViewList *);
int32 strview_list_len(StrViewList *);
StrView *strview_list_at(StrViewList *, int32);

void strflex_list_push(StrFlexList *, char *, int32);
void strflex_list_destroy(StrFlexList *);
void strflex_list_clear(StrFlexList *);
int32 strflex_list_len(StrFlexList *);
StrFlex *strflex_list_at(StrFlexList *, int32);

#define SFLIT(literal) ((StrFlex *)&(struct {            \
    int32 len;                                           \
    char data[sizeof(literal)];                          \
}){ sizeof(literal) - 1, literal })

#endif /* NCMPCPP2_H */
