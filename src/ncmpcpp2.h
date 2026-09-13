#if !defined(NCMPCPP2_H)
#define NCMPCPP2_H

#include "cbase.h"
#include "tags.h"

typedef struct StringView {
    char *data;
    int32 len;
} StringView;

typedef struct StringViewList {
    StringView *items;
    Arena *arena;
} StringViewList;

void string_list_push(StringViewList *, char *);
void string_list_destroy(StringViewList *);
void string_list_clear(StringViewList *);
int32 string_list_len(StringViewList *);
StringView *string_list_at(StringViewList *, int32);

#endif /* NCMPCPP2_H */
