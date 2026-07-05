#if !defined(NCMPCPP_NC_SCREEN_H)
#define NCMPCPP_NC_SCREEN_H

#include <stdbool.h>

#include "cbase/primitives.h"
#include "curses/nc_window.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NC_SCREEN_DEFAULT_WINDOW_TIMEOUT 500
#define NC_SCREEN_TYPE_UNKNOWN 0
#define NC_SCREEN_TYPE_BROWSER 1
#define NC_SCREEN_TYPE_HELP 2
#define NC_SCREEN_TYPE_LASTFM 3
#define NC_SCREEN_TYPE_LYRICS 4
#define NC_SCREEN_TYPE_MEDIA_LIBRARY 5
#define NC_SCREEN_TYPE_OUTPUTS 6
#define NC_SCREEN_TYPE_PLAYLIST 7
#define NC_SCREEN_TYPE_PLAYLIST_EDITOR 8
#define NC_SCREEN_TYPE_SEARCH_ENGINE 9
#define NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER 10
#define NC_SCREEN_TYPE_SERVER_INFO 11
#define NC_SCREEN_TYPE_SONG_INFO 12
#define NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG 13
#define NC_SCREEN_TYPE_TAG_EDITOR 14
#define NC_SCREEN_TYPE_TINY_TAG_EDITOR 15
#define NC_SCREEN_TYPE_VISUALIZER 16
#define NC_SCREEN_REGISTRY_MAX_SCREENS 64

typedef struct NcScreen NcScreen;
typedef struct NcScreenRegistry NcScreenRegistry;
typedef void (*NcScreenEachCallback)(NcScreen *screen, void *user);

typedef struct NcScreenCallbacks {
    NcWindow *(*active_window)(NcScreen *screen);
    void (*refresh)(NcScreen *screen);
    void (*refresh_window)(NcScreen *screen);
    void (*scroll)(NcScreen *screen, enum NcScroll where);
    void (*switch_to)(NcScreen *screen);
    void (*resize)(NcScreen *screen);
    int32 (*window_timeout)(NcScreen *screen);
    char *(*title)(NcScreen *screen);
    void (*update)(NcScreen *screen);
    void (*mouse_button_pressed)(NcScreen *screen, MEVENT event);
    bool (*is_lockable)(NcScreen *screen);
    bool (*is_mergable)(NcScreen *screen);
    void (*destroy)(NcScreen *screen);
} NcScreenCallbacks;

struct NcScreen {
    NcScreenCallbacks callbacks;
    void *user;
    int32 type;
    bool has_to_be_resized;
};

struct NcScreenRegistry {
    NcScreen *screens[NC_SCREEN_REGISTRY_MAX_SCREENS];
    NcScreen *current_screen;
    NcScreen *previous_screen;
    NcScreen *locked_screen;
    NcScreen *inactive_screen;
    int32 screens_len;
};

void nc_screen_init(NcScreen *screen, NcScreenCallbacks callbacks,
                    void *user, int32 type);
NcWindow *nc_screen_active_window(NcScreen *screen);
void nc_screen_refresh(NcScreen *screen);
void nc_screen_refresh_window(NcScreen *screen);
void nc_screen_scroll(NcScreen *screen, enum NcScroll where);
void nc_screen_switch_to(NcScreen *screen);
void nc_screen_resize(NcScreen *screen);
int32 nc_screen_window_timeout(NcScreen *screen);
char *nc_screen_title(NcScreen *screen);
int32 nc_screen_type(NcScreen *screen);
void nc_screen_set_type(NcScreen *screen, int32 type);
void nc_screen_update(NcScreen *screen);
void nc_screen_mouse_button_pressed(NcScreen *screen, MEVENT event);
bool nc_screen_is_lockable(NcScreen *screen);
bool nc_screen_is_mergable(NcScreen *screen);
bool nc_screen_has_to_be_resized(NcScreen *screen);
void nc_screen_set_has_to_be_resized(NcScreen *screen,
                                     bool has_to_be_resized);
void nc_screen_destroy(NcScreen *screen);
void *nc_screen_user(NcScreen *screen);

void nc_screen_registry_init(NcScreenRegistry *registry);
bool nc_screen_registry_register(NcScreenRegistry *registry,
                                 NcScreen *screen);
bool nc_screen_registry_unregister(NcScreenRegistry *registry,
                                   NcScreen *screen);
NcScreen *nc_screen_registry_find(NcScreenRegistry *registry, int32 type);
NcScreen *nc_screen_registry_current(NcScreenRegistry *registry);
NcScreen *nc_screen_registry_previous(NcScreenRegistry *registry);
NcScreen *nc_screen_registry_locked(NcScreenRegistry *registry);
NcScreen *nc_screen_registry_inactive(NcScreenRegistry *registry);
int32 nc_screen_registry_count(NcScreenRegistry *registry);
bool nc_screen_registry_is_registered(NcScreenRegistry *registry,
                                      NcScreen *screen);
bool nc_screen_registry_switch_to(NcScreenRegistry *registry,
                                  NcScreen *screen);
bool nc_screen_registry_switch_to_type(NcScreenRegistry *registry,
                                       int32 type);
bool nc_screen_registry_lock_current(NcScreenRegistry *registry);
void nc_screen_registry_unlock(NcScreenRegistry *registry);
bool nc_screen_registry_is_visible(NcScreenRegistry *registry,
                                   NcScreen *screen);
void nc_screen_registry_each_visible(NcScreenRegistry *registry,
                                     NcScreenEachCallback callback,
                                     void *user);
void nc_screen_registry_update_current(NcScreenRegistry *registry);
void nc_screen_registry_update_visible(NcScreenRegistry *registry);
void nc_screen_registry_update_all(NcScreenRegistry *registry);
void nc_screen_registry_resize_current(NcScreenRegistry *registry);
void nc_screen_registry_resize_visible(NcScreenRegistry *registry);
void nc_screen_registry_resize_all(NcScreenRegistry *registry);
bool nc_screen_registry_destroy_screen(NcScreenRegistry *registry,
                                       NcScreen *screen);
bool nc_screen_registry_destroy_type(NcScreenRegistry *registry,
                                     int32 type);
void nc_screen_registry_destroy_all(NcScreenRegistry *registry);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SCREEN_H */
