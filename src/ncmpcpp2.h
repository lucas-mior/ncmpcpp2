#if !defined(NCMPCPP2_H)
#define NCMPCPP2_H

#include "cbase.h"
#include "tags.h"

typedef struct StrView {
    int32 len;
    char *data;
} StrView;

typedef struct StrViewList {
    StrView *items;
    Arena *arena;
} StrViewList;

void strview_list_push(StrViewList *, char *, int32);
void strview_list_destroy(StrViewList *);
void strview_list_clear(StrViewList *);
int32 strview_list_len(StrViewList *);
StrView *strview_list_at(StrViewList *, int32);

#endif /* NCMPCPP2_H */
