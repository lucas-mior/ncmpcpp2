#if !defined(NCMPCPP_TESTS_CURSES_H)
#define NCMPCPP_TESTS_CURSES_H

#include "cbase/primitives.h"

typedef struct WINDOW WINDOW;
typedef uint64 chtype;

typedef struct MEVENT {
    int32 id;
    int32 x;
    int32 y;
    int32 z;
    uint64 bstate;
} MEVENT;

#define NCURSES_MOUSE_VERSION 2

int32 mvwhline(WINDOW *win, int32 y, int32 x, chtype ch, int32 n);

#endif /* NCMPCPP_TESTS_CURSES_H */
