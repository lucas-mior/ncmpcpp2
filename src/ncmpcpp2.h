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

#endif /* NCMPCPP2_H */
