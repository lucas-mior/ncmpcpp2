# C migration inventory

This file tracks the remaining C++ surface used by the root `Makefile`.  During
migration, the root `Makefile` is the supported build path.  The generated
autotools Makefiles under subdirectories are migration artifacts only.

All replacement work must follow `c-migration-patterns.txt`: public APIs are
plain C, ownership is explicit, resource lifetimes use `init`/`destroy` or
`create`/`free`, and recoverable failures use explicit return values or
`NcmError`.

## Current C++ count

Collected with `make cxx-count` after Stage 3.1 changes:

```text
built_cpp=21
total_cpp=38
```

`make no-cxx` is expected to fail until the later stages remove every entry in
this inventory.  Stage 2 removes `src/ncmpcpp.cpp` from the root build but
keeps it on disk for comparison until final cleanup.  Stage 3.1 keeps the
remaining C++ screen files built, but screen lookup, startup switching, current
screen type checks, locking, and resize requests now resolve through the native
`NcScreen` registry API instead of direct `BaseScreen *` startup paths.  The
target now uses `tools/no-cxx-scan.py`, which ignores comments and literals
before checking banned C++ tokens.

## C++ source inventory

| Path | Built by root Makefile | Group | Final action |
| --- | --- | --- | --- |
| `extras/artist_to_albumartist.cpp` | no | unsupported extra | Delete during final C cleanup unless a separate C/taglib_c extra is requested. |
| `src/actions.cpp` | no | already replaced source | Delete; keep `src/actions.c` as the C action owner. |
| `src/actions_legacy.cpp` | yes | legacy bridge | Translate remaining callbacks into `src/actions.c`, then delete. |
| `src/bindings.cpp` | no | already replaced source | Delete; keep `src/bindings.c` as the C binding owner. |
| `src/bindings_legacy.cpp` | yes | legacy bridge | Remove after C bindings execute actions without C++ adapters. |
| `src/configuration.cpp` | no | already replaced source | Delete; keep `src/configuration.c` as the C configuration owner. |
| `src/configuration_legacy.cpp` | yes | legacy bridge | Remove after all configuration callers use the C API directly. |
| `src/curl_handle.cpp` | no | already replaced source | Delete; keep `src/curl_handle.c`. |
| `src/helpers.cpp` | no | already replaced source | Delete; keep `src/helpers.c` and `src/c/ncm_*` helpers. |
| `src/helpers_legacy.cpp` | yes | legacy bridge | Replace remaining callers with C helpers, then delete. |
| `src/lastfm_service.cpp` | no | already replaced source | Delete; keep `src/lastfm_service.c`. |
| `src/lyrics_fetcher.cpp` | no | already replaced source | Delete; keep `src/lyrics_fetcher.c`. |
| `src/mpdpp.cpp` | yes | MPD wrapper/model bridge | Replace with `NcmMpdClient`, `NcmSong`, `NcmMutableSong`, `NcmMpdItem`, and `NcmPlaylist`, then delete. |
| `src/ncmpcpp.cpp` | no | application entry/runtime | Replaced in the root build by `src/ncmpcpp.c`; delete during final C cleanup once later stages remove the legacy bridge. |
| `src/screens/browser.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_browser.c`, then delete. |
| `src/screens/help_bridge.cpp` | no | already replaced bridge | Delete after native C screen switching no longer needs C++ bridge glue. |
| `src/screens/lastfm.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_lastfm.c`, then delete. |
| `src/screens/lyrics.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_lyrics.c`, then delete. |
| `src/screens/media_library.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_media_library.c`, then delete. |
| `src/screens/outputs_bridge.cpp` | no | already replaced bridge | Delete after native C screen switching no longer needs C++ bridge glue. |
| `src/screens/playlist.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_playlist.c`, then delete. |
| `src/screens/playlist_editor.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_playlist_editor.c`, then delete. |
| `src/screens/search_engine.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_search_engine.c`, then delete. |
| `src/screens/sel_items_adder.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_sel_items_adder.c`, then delete. |
| `src/screens/server_info_bridge.cpp` | no | already replaced bridge | Delete after native C screen switching no longer needs C++ bridge glue. |
| `src/screens/song_info_bridge.cpp` | no | already replaced bridge | Delete after native C screen switching no longer needs C++ bridge glue. |
| `src/screens/sort_playlist.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_sort_playlist.c`, then delete. |
| `src/screens/tag_editor.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_tag_editor.c`, then delete. |
| `src/screens/tiny_tag_editor.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_tiny_tag_editor.c`, then delete. |
| `src/screens/visualizer.cpp` | yes | C++ screen class | Move missing behavior to `src/screens/nc_visualizer.c`, then delete. |
| `src/settings.cpp` | no | already replaced source | Delete; keep `src/settings.c` and `src/settings_types.c`. |
| `src/settings_legacy.cpp` | yes | legacy bridge | Remove after C settings callers no longer need C++ adapters. |
| `src/status.cpp` | no | already replaced source | Delete; keep `src/status.c`. |
| `src/status_legacy.cpp` | yes | legacy bridge | Remove after all status callers use the C API directly. |
| `src/statusbar.cpp` | no | already replaced source | Delete; keep `src/statusbar.c`. |
| `src/statusbar_legacy.cpp` | yes | legacy bridge | Remove after the app shell and actions use C statusbar helpers. |
| `src/title.cpp` | no | already replaced source | Delete; keep `src/title.c`. |
| `src/title_legacy.cpp` | yes | legacy bridge | Remove after the app shell and actions use C title helpers. |

## C++ header inventory

These headers are part of the C++ surface even when the root Makefile only
counts `.cpp` files.

### Legacy bridge headers

Final action: delete with the matching bridge once all callers use C APIs.

- `src/actions_legacy.h`
- `src/bindings_legacy.h`
- `src/configuration_legacy.h`
- `src/helpers_legacy.h`
- `src/ncm_playlist_legacy.h`
- `src/settings_legacy.h`
- `src/status_legacy.h`
- `src/statusbar_legacy.h`
- `src/title_legacy.h`
- `src/ui_state_legacy.h`

### MPD and model headers

Final action: replace with the C model/client modules and delete.

- `src/mpdpp.h` -> `src/c/ncm_mpd_client.*`
- `src/mutable_song.h` -> `src/c/ncm_mutable_song.*`
- `src/song.h` -> `src/c/ncm_song.*`
- `src/song_list.h` -> `src/c/ncm_song_list.*`
- `src/tags.h` -> `src/c/ncm_tags.*`

### Curses C++ wrapper headers

Final action: replace with `src/curses/nc_*` modules and delete.

- `src/curses/formatted_color.h`
- `src/curses/formatted_color_compat_impl.h`
- `src/curses/menu.h`
- `src/curses/menu_impl.h`
- `src/curses/scrollpad.h`
- `src/curses/scrollpad_compat_impl.h`
- `src/curses/strbuffer.h`
- `src/curses/window.h`
- `src/curses/window_compat_impl.h`

### Screen C++ headers

Final action: move any missing behavior into the matching `nc_*` screen and
delete the C++ header.

- `src/screens/browser.h`
- `src/screens/lastfm.h`
- `src/screens/lyrics.h`
- `src/screens/media_library.h`
- `src/screens/playlist.h`
- `src/screens/playlist_editor.h`
- `src/screens/search_engine.h`
- `src/screens/sel_items_adder.h`
- `src/screens/sort_playlist.h`
- `src/screens/tag_editor.h`
- `src/screens/tiny_tag_editor.h`
- `src/screens/visualizer.h`
- `src/screens/screen_cpp_compat.h`
- `src/screens/screen_cpp_legacy.h`
- `src/screens/screen_cpp_switcher.h`

### Utility C++ headers

Final action: replace with the C modules named by `c-migration-patterns.txt` and
delete.

- `src/utility/any_iterator.h` -> `src/c/ncm_any_iterator.h`
- `src/utility/conversion.h` -> `src/c/ncm_conversion.*`
- `src/utility/functional.h` -> `src/c/ncm_functional.*`
- `src/utility/regex.h` -> `src/c/ncm_regex.*`
- `src/utility/scoped_value.h` -> `src/c/ncm_scoped_value.*`
- `src/utility/shared_resource.h` -> `src/c/ncm_shared_resource.*`
- `src/utility/string_format.h` -> `src/c/ncm_string_format.*`
- `src/utility/transform_iterator.h` -> `src/c/ncm_transform_iterator.h`
- `src/utility/wide_string.h` -> `src/c/ncm_wide_string.*`

### Remaining C++-tainted public headers

Final action: audit during the corresponding stage, replace C++ declarations
with plain C declarations, or delete if the C replacement already owns the API.

- `src/charset.h`
- `src/display.h`
- `src/format.h`
- `src/interfaces.h`
- `src/macro_utilities.h`
- `src/regex_filter.h`
- `src/runnable_item.h`
- `src/screens/screen_type.h`
- `src/utility/comparators.h`
- `src/utility/const.h`
- `src/utility/html.h`
- `src/utility/option_parser.h`
- `src/utility/sample_buffer.h`
- `src/utility/storage_kind.h`
- `src/utility/string.h`
- `src/utility/utf8.h`

## Stage handoff checks

Before starting a later stage, refresh the inventory with:

```sh
make cxx-report
make cxx-count
make no-cxx
```

`make no-cxx` should only become green at the end of the migration.  Until then,
its output is the authoritative list of C++ extensions, standard-library
includes, and banned C++ tokens still visible to the root migration build.
