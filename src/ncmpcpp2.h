#if !defined(NCMPCPP2_H)
#define NCMPCPP2_H

#include "cbase.h"
#include "tags.h"

typedef struct StrView {
    char *data;
    int32 len;
} StrView;

typedef struct StrViewList {
    StrView *items;
    Arena *arena;
} StrViewList;

void string_list_push(StrViewList *, char *, int32);
void string_list_destroy(StrViewList *);
void string_list_clear(StrViewList *);
int32 string_list_len(StrViewList *);
StrView *string_list_at(StrViewList *, int32);

#endif /* NCMPCPP2_H */
