# Playlist C migration inventory and behavior checklist

This document establishes the implementation boundary for removing
`src/screens/playlist.h`. It records every direct dependency found before the
cutover and maps it to an existing C API or a concrete missing capability.

## Inventory command

```sh
rg -n \
  '\bmyPlaylist\b|screens/playlist\.h|NativePlaylistBridge|set_display_menu|registerNativeScreen' \
  src tests Makefile
```

The initial inventory contains:

- 56 `myPlaylist` occurrences on 54 source lines.
- Five direct includes of `screens/playlist.h`.
- Bridge definitions and uses in `src/screens/nc_playlist.h`,
  `src/screens/nc_playlist.c`, `src/screens/playlist.h`, and
  `tests/c_playlist_screen_test.c`.

## Direct include inventory

| File | Dependency type | Required migration |
| --- | --- | --- |
| `src/actions_legacy.cpp` | Functional | Replace lifecycle, screen checks, menu access, and playlist actions. |
| `src/display.h` | Functional | Replace playlist-window detection and song-membership lookup. |
| `src/helpers_legacy.h` | Transitive only | Remove include and add any actually required direct includes. |
| `src/screens/playlist_editor.h` | Transitive only | Remove include and add any actually required direct includes. |
| `src/screens/tag_editor.h` | Transitive only | Remove include and add any actually required direct includes. |

No use of `Playlist` or `myPlaylist` was found in the last three files beyond
the include itself.

## `actions_legacy.cpp` call-site classification

| Function or area | Lines | Category | Existing C replacement | Remaining work |
| --- | ---: | --- | --- | --- |
| `highlightPlaylistMpdPosition` | 252-284 | Highlighting and filtering | `native_playlist_screen_locate_position()` | Preserve the filtered-out status message. |
| `Actions::initializeScreens` | 497-522 | Construction and registration | `native_c_screen_playlist_init()` and `native_c_screen_playlist_register()` | Remove allocation and legacy owner registration. Preserve screen order. |
| `Actions::setResizeFlags` | 524-535 | Resize lifecycle | `native_c_screen_playlist_set_resize()` | Replace the C++ resize flag write. |
| `UpdateEnvironment::run` | 649-667 | Current-screen check | `native_c_screen_playlist_is_current()` | Replace pointer comparison. |
| `MoveSelectedItemsUp` | 1177-1211 | Selection and MPD mutation | `NCM_ACTION_MOVE_SELECTED_ITEMS_UP` in `actions.c` | Preserve the filtered-playlist prohibition and message. |
| `MoveSelectedItemsDown` | 1213-1247 | Selection and MPD mutation | `NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN` in `actions.c` | Preserve the filtered-playlist prohibition and message. |
| `MoveSelectedItemsTo` | 1249-1270 | Prompted MPD mutation | None in the C runtime | Implement C `can_run`, prompt, and move behavior. |
| `ToggleDisplayMode` | 1340-1371 | Display configuration | `NCM_ACTION_TOGGLE_DISPLAY_MODE` in `actions.c` | Add playlist column-title parity, refresh behavior, and status text. |
| `TogglePlayingSongCentering` | 1449-1461 | Configuration and locating | C action toggles `Config.autocenter_mode` | When enabling, immediately locate the playing position as legacy code does. |
| `JumpToPlayingSong` | 1463-1491 | Current song and locating | `native_playlist_screen_locate_position()` and the C action runtime | Replace only the playlist branch; legacy browser/editor branches remain until their migrations. |
| `Shuffle` | 1493-1510 | Selected range and MPD mutation | `NCM_ACTION_SHUFFLE` in `actions.c` | Preserve confirmation and status messages. Verify range semantics. |
| `JumpToPositionInSong` | 1976-2030 | Current song metadata | Status state exposes queue position and duration | C runtime action is still unimplemented. Remove its dependency on `MPD::Song`. |
| `CropMainPlaylist` | 2152-2163 | Selection and MPD mutation | `ncm_action_crop_main_playlist()` | Preserve the size guard, confirmation, and status messages. |
| `ClearMainPlaylist` | 2188-2196 | MPD mutation and local state | `NCM_ACTION_CLEAR_MAIN_PLAYLIST` in `actions.c` | Preserve confirmation and status text. Clear or reload native state immediately. |
| `ReversePlaylist` | 2216-2235 | Selected range and MPD mutation | `NCM_ACTION_REVERSE_PLAYLIST` in `actions.c` | Preserve status messages and verify selected-range parity. |
| `SetSelectedItemsPriority` | 2556-2580 | Prompted MPD mutation | `native_playlist_screen_set_selected_priority()` | C runtime action is unimplemented. Add MPD version check and priority prompt. |
| `ShowPlaylist` | 2733-2745 | Screen switching | `NCM_ACTION_SHOW_PLAYLIST` or `native_c_screen_playlist_switch_to()` | Replace pointer comparison and C++ `switchTo()`. |

## `display.h` call-site classification

`Display::Detail::setProperties()` has three direct dependencies:

1. `myPlaylist->isActiveWindow(menu)` identifies rows drawn by the C++ playlist
   menu so the playing row receives the now-playing prefix and suffix.
2. The inverse active-window check prevents the main playlist from marking its
   own entries as "already in playlist".
3. `myPlaylist->checkForSong(s)` queries the C++ song-reference map.

After cutover, the native playlist no longer uses `Display::Songs()` or a C++
`NC::Menu<MPD::Song>`. Therefore legacy menus can be treated as non-playlist
menus. The remaining required capability is a native song-membership query.
No public C function currently provides that query.

Required API gap:

```c
bool native_playlist_screen_contains_song(NativePlaylistScreen *screen,
                                          NcmSong *song);
```

The implementation must use the same identity semantics as `MPD::Song`:
`ncm_song_hash()` for lookup and `ncm_song_equal()` for equality. Duplicate
entries must remain present until the last matching queue entry is removed.

## Compatibility bridge inventory

### C++ side: `src/screens/playlist.h`

The constructor currently:

- Creates and configures `NC::Menu<MPD::Song>`.
- Installs a sync callback.
- Installs `NativePlaylistBridge` callbacks.
- Replaces the native display menu with the C++ menu.
- Binds the native `NcScreen` to a C++ `BaseScreen` owner.

The destructor reverses those operations and unregisters the screen.

`syncNative()` copies all C++ rows, flags, highlight state, and selection state
into the native menu. The update callbacks copy MPD changes in the opposite
direction. This creates two playlist stores that must remain synchronized.

### C side: `src/screens/nc_playlist.h` and `.c`

The compatibility surface consists of:

- `NativePlaylistSync`
- `NativePlaylistBridge`
- `sync` and `sync_user`
- The `bridge` field
- `native_playlist_screen_set_sync_callback()`
- `native_playlist_screen_set_bridge()`
- `native_playlist_screen_set_display_menu()`
- Conditional bridge paths for active window, refresh, resize, updates,
  registration, and unregistration

These interfaces are deletion targets after the native screen becomes the sole
playlist owner.

### Tests

`tests/c_playlist_screen_test.c` currently tests compatibility behavior through:

- `native_playlist_screen_set_display_menu()`
- `native_playlist_screen_set_sync_callback()`
- `NativePlaylistBridge.update_song`

Those tests must later be replaced with bridge-free native-state tests. They
are not removed during the inventory step.

## Public `Playlist` behavior checklist

The table below maps the complete public surface and constructor behavior of
`Playlist` to the current native implementation.

| Legacy behavior | Native implementation | Status before cutover |
| --- | --- | --- |
| Initialize geometry, color, and border | `native_playlist_screen_init()` | Present |
| Cyclic scrolling | `nc_menu_set_cyclic_scrolling()` | Present |
| Centered cursor | `nc_menu_set_centered_cursor()` | Present |
| Highlight prefix and suffix | `nc_menu_set_highlight_prefix/suffix()` | Present |
| Selected prefix and suffix | `nc_menu_set_selected_prefix/suffix()` | Present |
| Mouse scroll settings | `native_playlist_screen_set_mouse_config()` | Present |
| Classic row rendering | `native_playlist_draw_song()` | Present |
| Column row rendering | `native_playlist_draw_song()` | Present |
| Column heading title | No native playlist implementation found | Missing |
| Activate current song by MPD ID | `native_playlist_activate_song()` | Present |
| Refresh screen | `nc_screen_refresh()` callback path | Present, bridge-free path needs regression coverage |
| Refresh active window | `nc_screen_refresh_window()` callback path | Present, bridge-free path needs regression coverage |
| Scroll | `nc_playlist_screen_scroll()` | Present |
| Switch to screen | `native_c_screen_playlist_switch_to()` | Present |
| Resize | Native screen resize callback | Present |
| Window timeout | `NC_SCREEN_DEFAULT_WINDOW_TIMEOUT` | Present |
| Screen title | `native_playlist_refresh_stats()` | Present |
| Periodic update | `native_playlist_update()` | Present only as a no-op |
| Disable highlight after configured delay | No native update implementation | Missing |
| Mouse handling | `nc_playlist_screen_mouse_button_pressed()` | Present |
| Lockable | Native callback returns `true` | Present |
| Mergable | Native callback returns `true` | Present |
| Screen type and native screen | `NcScreen` and native screen accessors | Present |
| Search allowed | `current_screen_allows_search()` | Present |
| Persistent search constraint | `search_constraint` through `screen_actions.c` | Present |
| Clear search constraint | `current_screen_clear_search_constraint()` | Present |
| Forward/backward search | `native_playlist_screen_search()` | Present |
| Wrapped search and skip-current | `native_playlist_screen_search()` | Present |
| Filter allowed | `current_screen_allows_filter()` | Present |
| Current filter string | `filter_constraint` through `screen_actions.c` | Present |
| Apply and clear filter | `native_playlist_screen_apply_filter()` and `clear_filter()` | Present |
| Item availability | `native_playlist_screen_empty()` | Present |
| Play current item | `nc_playlist_screen_activate_current()` | Present |
| Retrieve selected songs | `native_playlist_screen_selected_songs()` | Present |
| Retrieve now-playing song | `native_playlist_screen_now_playing_song()` | Present |
| Locate song by MPD position | `native_playlist_screen_locate_position()` | Present; filtered-out message is missing |
| Set selected priority | `native_playlist_screen_set_selected_priority()` | Present |
| Test song membership | No public native API | Missing |
| Register/unregister duplicate song references | Only C++ hash map and bridge callbacks | Missing as standalone C behavior |
| Reload total length | `native_playlist_screen_reload_total_length()` | Present |
| Reload remaining time | `native_playlist_screen_reload_remaining()` | Present |
| Full and incremental MPD reload | `native_playlist_screen_reload_from_mpd()` | Present |
| Preserve filtering during incremental updates | Native update code reapplies filters | Present; requires bridge-free tests |
| Preserve selection during incremental updates | No complete regression coverage | Verification required |

## Required native gaps discovered by the inventory

The following capabilities must be completed before deleting
`src/screens/playlist.h`:

1. Native song-membership lookup with duplicate-entry semantics.
2. Highlight timeout handling in the native update callback.
3. Column heading title generation and invalidation.
4. Filtered-out feedback when locating a queue position.
5. C runtime implementation of `MoveSelectedItemsTo`.
6. C runtime implementation of `SetSelectedItemsPriority`.
7. Removal of the playlist dependency from `JumpToPositionInSong`.
8. Legacy parity for confirmations, status messages, filtering restrictions,
   and immediate local-state updates in existing C actions.

## Completion criterion for the inventory step

This inventory step is complete when:

- The clean build and test baseline is recorded separately.
- Every direct include and `myPlaylist` call site is classified above.
- Every public `Playlist` behavior has a native mapping or an explicit gap.
- No production source behavior has been changed yet.
