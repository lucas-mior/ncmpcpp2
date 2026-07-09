# ncmpcpp2
- goal is to convert the entire project to plain C (getting rid of C++)
- general deletion rule: do not remove a `.cpp` file until the matching C implementation is registered through the native screen/action/runtime path, the old C++ globals are no longer referenced, and `make check` still passes.

# .cpp files to convert

## ./src/screens/lyrics.cpp
1. ~~Keep `src/lyrics_fetcher.c` and `src/lyrics_fetcher.h` as the authoritative C lyrics fetch/parser layer. They already cover fetcher registration, URL construction, HTML cleanup, optional tag lyrics, and fake curl/escape hooks for deterministic tests; do not move provider parsing back into the screen.~~
2. ~~Treat `src/screens/nc_lyrics.c` and `src/screens/nc_lyrics.h` as the replacement screen. They already own `NativeLyricsScreen`, the C scrollpad/window/display buffers, current song/filename/result storage, selected fetcher state, background queue state, consumer-message storage, job queue, and native registration wrappers through `native_c_screens.c`.~~
3. ~~Remove the C++ facade from legacy initialization only after the native screen is registered in the same place as the old facade: stop including `screens/lyrics.h`, stop allocating `myLyrics`, stop calling `myLyrics->registerNativeScreen()`, stop setting `myLyrics->hasToBeResized`, and use `native_c_screen_lyrics_init/register/set_resize/switch_to` instead. Make `native_c_screen_lyrics_switch_to()` preserve the old toggle/back behavior: when Lyrics is already current, switch to the previous registered screen; otherwise register if needed and switch to Lyrics.~~
4. ~~Rewrite `ShowLyrics::{canBeRun,run}` to call the native screen while preserving legacy behavior. `canBeRun()` must return true when the native Lyrics screen is current, otherwise require the current song; `run()` should fetch only when a new song is available, then switch/toggle the native screen. The native fetch path must build the legacy filename, clear/reset the display for a changed song, load an existing lyrics file first, and only start a download when no file exists.~~
5. ~~Finish foreground fetch parity in `nc_lyrics.c`: use the screen's active fetcher pointer when set, otherwise try all configured fetchers in order; show the same `Fetching lyrics from <fetcher>...` progress/error text where practical; on successful completion, replace the display with the fetched lyrics and save the exact fetched text to the computed filename; on failure, append `\nLyrics were not found.\n`; ignore stale completed jobs whose song/filename no longer match the current screen song.~~
6. ~~Preserve lyrics fetcher selection semantics before moving `TOGGLE_LYRICS_FETCHER` out of the forced legacy path. The old action cycles `screen->fetcher` through each configured fetcher and then back to “all lyrics fetchers”, with statusbar messages naming the selected fetcher; either keep `NativeLyricsScreen.fetcher` as the source of truth or explicitly synchronize it with `Config.lyrics_db`, but do not leave the current mismatch where the C action changes `Config.lyrics_db` and the native fetch path ignores it.~~
7. ~~Port background fetching through the native queue with the same safeguards as `Lyrics::fetchInBackground()`: queue selected songs, skip streams, build the lyrics filename for each song, skip songs whose lyrics file already exists, start the worker only on demand, save successful downloads to disk, and expose the same consumer notification flow so `UpdateEnvironment` can print `Fetching lyrics for "<song>"...` using `native_lyrics_screen_try_take_consumer_message()`.~~
8. Port `RefetchLyrics` and `EditLyrics` without depending on `myLyrics`. Refetch should remove the current lyrics file, tolerate `ENOENT`, clear pending foreground state, and force a new native fetch. Edit should require `Config.external_editor`, build and shell-escape the native lyrics filename, run the console or external editor exactly like the legacy path, reload/refetch after the console editor returns, and surface file/editor errors through the statusbar.
9. Finish screen UI/input parity in `nc_lyrics.c`: implement the dynamic title `Lyrics: <artist - title/file>` with the same cyclic scrolling width calculation, keep `isLockable()`/`isMergable()` true, preserve keyboard scrolling through `nc_scrollpad_scroll`, implement BUTTON4/BUTTON5 mouse-wheel scrolling with `Config.lines_scrolled`, resize with `nc_screen_switcher_get_resize_params()`, and keep the header refresh cadence for Lyrics in `UpdateEnvironment` using the native-current check.
10. Add/update tests for filename construction, load/save behavior, fetcher parsing with fake I/O, foreground success/failure rendering and save behavior, stale-result suppression, fetcher toggling, background queue skip/save/notification behavior, dynamic title storage, mouse scrolling, and native registration/toggle switching. After those pass, move lyrics actions out of `action_forces_legacy_binding()`, remove `#include "screens/lyrics.h"`, remove the `#include "screens/lyrics.cpp"` block from `actions_legacy.cpp`, confirm no `myLyrics`/`Lyrics::`/`screens/lyrics.h` references remain, then delete `src/screens/lyrics.cpp` and delete `src/screens/lyrics.h` if unused.

## ./src/screens/media_library.cpp
1. Treat `src/screens/nc_media_library.c` and `src/screens/nc_media_library.h` as the existing C shell for the media library, but keep `media_library.cpp` until the C screen performs real MPD population rather than only storing manually added rows.
2. Port the C++ update pipeline into C: fetch primary tags, albums, and songs with `ncm_mpd_client_get_list()`, search APIs, recursive directory APIs, and song-list helpers.
3. Preserve all three display modes: three-column library, two-column album/song library, and album-only mode, including the mode transitions from `toggleColumnsMode()`.
4. Preserve the configured column width ratios from `Config.media_library_column_width_ratio_three` and `Config.media_library_column_width_ratio_two`; replace the current equal-width layout and redraw separators/titles as the C++ screen did.
5. Port media-library sorting: locale-aware tag/album/song ordering, mtime sorting for albums, `ignore_leading_the`, album date hiding/splitting, and the `All tracks` synthetic row.
6. Extend C row data so tag and album selections can produce the same song sets as the C++ screen, not only the currently highlighted song row.
7. Wire add/play behavior from the media library: selected tag, album, all-tracks row, or song should add the same MPD songs to the playlist in the same order as the legacy implementation.
8. Port search/filter/locate-song parity: each active column must use the same predicates as the C++ screen, and `JUMP_TO_MEDIA_LIBRARY` must populate missing tag/album/song rows before highlighting the current song.
9. Move media-library actions out of the legacy binding list only after the native screen handles show, jump, column movement, add/play, search/filter, sort-mode toggle, and columns-mode toggle.
10. Delete the `#include "screens/media_library.cpp"` line from `src/actions_legacy.cpp`, remove `screens/media_library.h` references, and then delete `src/screens/media_library.cpp`.

## ./src/screens/sel_items_adder.cpp
1. Treat `src/screens/nc_sel_items_adder.c` and `src/screens/nc_sel_items_adder.h` as the existing C replacement for the selected-items adder dialog.
2. Wire the C dialog into `src/actions.c` so `ADD_SELECTED_ITEMS` gathers selected songs from the current native screen and calls `native_selected_items_adder_screen_set_songs()` before switching screens.
3. Preserve the local-browser exception from the C++ dialog: adding from a local browser should offer the current playlist path only when the old screen is local.
4. Populate the playlist selector from MPD through `ncm_mpd_client_get_playlists()`, include the current playlist and new-playlist rows, sort existing playlists locale-aware, and keep the cancel row last.
5. Implement the new-playlist prompt in C and route the result through the same add-to-playlist code path as existing playlists.
6. Consume `native_selected_items_adder_screen_take_action()` from the action/update loop and perform the requested operation instead of only storing `last_action`.
7. Port the position insertion behavior: add at end, add at beginning, add after current song, add after current album, and add after highlighted item using native playlist state and MPD command lists.
8. Preserve the dialog navigation rules: after choosing current playlist, show the position selector; after adding or cancelling, switch back to the previous screen; from the position selector, cancel returns to the playlist selector.
9. Port the legacy geometry calculation and resize behavior instead of leaving resize as a request-clear operation.
10. Delete the `#include "screens/sel_items_adder.cpp"` line from `src/actions_legacy.cpp`, remove `screens/sel_items_adder.h` references, and then delete `src/screens/sel_items_adder.cpp`.

## ./src/screens/sort_playlist.cpp
1. Treat `src/screens/nc_sort_playlist.c` and `src/screens/nc_sort_playlist.h` as the existing C replacement for the sort-playlist dialog.
2. Keep the C menu row order and movement behavior, but wire `MOVE_SORT_ORDER_UP` and `MOVE_SORT_ORDER_DOWN` only through the native dialog when that screen is active.
3. Consume `native_sort_playlist_dialog_take_sort_request()` and `native_sort_playlist_dialog_take_cancel_request()` from the action/update loop; cancel should switch back without changing MPD state.
4. Port `findSelectedRange()` to C using the native playlist menu selection helpers so sorting applies to the selected contiguous range, or to the full playlist when there is no selected range.
5. Build a sortable C array containing song identity, original playlist position, and the configured getter order returned by `native_sort_playlist_dialog_get_order()`.
6. Port the legacy comparator: compare by each selected tag getter, use locale-aware string comparison, respect `ignore_leading_the`, and fall back to original playlist position for stable ordering.
7. Replace the C++ quicksort/`Mpd.Swap` loop with C sorting plus `ncm_mpd_client_swap()` or equivalent MPD command-list operations.
8. After sorting, refresh native playlist state, clear the dialog request flags, and return to the previous screen.
9. Port the centered dialog geometry and resize behavior from the C++ screen.
10. Delete the `#include "screens/sort_playlist.cpp"` line from `src/actions_legacy.cpp`, remove `screens/sort_playlist.h` references, and then delete `src/screens/sort_playlist.cpp`.

## ./src/screens/visualizer.cpp
1. Treat `src/screens/nc_visualizer.c` and `src/screens/nc_visualizer.h` as the existing C visualizer state/screen shell, but keep `visualizer.cpp` until data-source I/O and drawing are fully ported.
2. Port FIFO and Unix-socket data-source handling into C: open, close, reconnect, nonblocking reads, sample-size handling, and errors for `Config.visualizer_data_source`/`Config.visualizer_fifo_path`.
3. Replace `actions_legacy_runtime_visualizer_setup_datasource()` with a C status/update hook that closes, opens, and re-identifies the visualizer data source when MPD output/state changes.
4. Port MPD output detection/reset behavior using `Config.visualizer_output_name` and the C MPD client output APIs so switching to the visualizer can reset the configured output when needed.
5. Port sample ingestion parity: channel count, sample consumption rate, requested sample count, buffer shifting, stereo splitting, and player-state-aware read timing.
6. Finish autoscale parity by updating the multiplier from incoming sample amplitude rather than only applying the current multiplier.
7. Port drawing functions for wave, filled wave, ellipse, frequency spectrum, and stereo variants using the C curses window API and `Config.visualizer_chars`/`Config.visualizer_colors`.
8. Port FFTW-backed spectrum support under `HAVE_FFTW3_H`, including DFT size, frequency min/max, log scaling, gain, smoothing, and legacy smooth-character mode.
9. Wire `SHOW_VISUALIZER` and `TOGGLE_VISUALIZATION_TYPE` through C only after the native screen renders correctly and updates its timeout based on source/player state.
10. Delete the `#include "screens/visualizer.cpp"` line from `src/actions_legacy.cpp`, remove `screens/visualizer.h` references, and then delete `src/screens/visualizer.cpp`.

## ./src/actions_legacy.cpp
1. Treat `src/actions.c`, `src/actions.h`, `src/screen_actions.c`, `src/app_binding_migration.c`, and `src/app_legacy_bridge.c` as the existing C action system.
2. Audit every action still listed in `action_forces_legacy_binding()` and classify it as already implemented in `actions.c`, implemented by a native screen but not wired, or still missing.
3. Move already-safe actions out of the forced-legacy list after verifying `action_runtime_builtin_can_run()` and `action_runtime_builtin_run()` agree for each action.
4. Replace legacy screen creation in `actions_legacy_runtime_initialize_screens()` with `native_c_screens_init_all()` plus native screen registration for all converted screens.
5. Replace legacy window/screen callbacks in `actions_legacy_runtime_window_*` with calls into `app_controller`, `screen_registry`, and the native screen base APIs.
6. Port the remaining action implementations that still return false in the C runtime: run action by name, delete browser items, save playlist, move selected items, search/find mode, crossfade/volume prompts, editor actions, select album/found items, random items, and selected item priority.
7. Replace all direct uses of C++ globals such as `myBrowser`, `myPlaylist`, `myLibrary`, `myLyrics`, `myVisualizer`, and dialog globals with native screen accessors or app-state accessors.
8. Move legacy status/title/update side effects into C modules so `status.c`, `title`, playlist updates, library updates, lyrics background fetches, browser extension refreshes, and visualizer data-source refreshes no longer require runtime stubs.
9. Remove the six `#include "screens/*.cpp"` inclusions from this file after each native screen plan above is complete.
10. Delete `src/actions_legacy.cpp` and `src/actions_legacy_runtime.h`, then remove `src/actions_legacy.cpp` from `APP_CXX_SRCS` and link the final binary with `$(CC)`.

## ./src/configuration_legacy.cpp
1. Treat `src/configuration.c` and `src/configuration.h` as the existing C replacement for command-line parsing, default path discovery, config reading, bindings reading, current-song output, and lyrics-fetcher testing.
2. Update every caller to include `configuration.h` instead of `configuration_legacy.h` and to call `configure(int32 argc, char **argv)` from C.
3. Verify `expand_home()` parity: `~`, `$HOME`, missing home, relative paths, and error reporting must match the behavior expected by settings and path parsing.
4. Verify default config/bindings path discovery for XDG paths, legacy `~/.ncmpcpp` paths, and explicit `--config`/`--bindings` arguments.
5. Verify MPD host/port/password/timeout application from command-line options and environment variables is done through the C MPD client.
6. Verify startup screen/slave-screen parsing is applied through the C settings and app-state path rather than legacy screen names.
7. Verify `--current-song`, `--test-lyrics-fetchers`, `--help`, `--version`, `--quiet`, and `--ignore-config-errors` behavior against the legacy implementation.
8. Move any remaining configuration-only helper that still lives in C++ into `configuration.c` or `settings.c`.
9. Remove `configuration_legacy.h` includes from browser/screen compatibility headers as those screens are converted or given C-facing wrappers.
10. Delete `src/configuration_legacy.cpp` and `src/configuration_legacy.h`, then remove `src/configuration_legacy.cpp` from `APP_CXX_SRCS`.

## ./src/helpers_legacy.cpp
1. Treat `src/helpers.c` and `src/helpers.h` as the existing C replacement for time formatting, timestamps, menu iteration, song-list iteration, wrapped search, and selection range helpers.
2. Replace C++ helper calls from converted screens/actions with C helpers: `ncm_helpers_time_format()`, `ncm_helpers_show_song_time()`, menu selection helpers, and wrapped search helpers.
3. Port any remaining `addSongsToPlaylist` behavior into C using `NcmSongList`, native menu iterators, and MPD command lists.
4. Port delete, crop, move, reverse, and shuffle selected playlist-item behavior into `actions.c`/`helpers.c` so it no longer depends on template helpers.
5. Replace `ShowTag`/legacy display helper behavior with C formatting callbacks using `ncm_format`, `ncm_song`, and configured empty-tag/color handling.
6. Replace legacy cyclic scroller usage with `NcBuffer`/C title helpers for lyrics, last.fm, and other converted screens.
7. Move highlight-fix behavior into native menu/screen code so playlist, browser, and library refreshes do not depend on C++ menu templates.
8. Remove `helpers_legacy.h` includes from converted screens, display helpers, tag utilities, playlist editor, search engine, tag editor, tiny tag editor, and status/title legacy code as those areas are converted.
9. Add focused tests for selection ranges, wrapped search, playlist command-list construction, and time/timestamp formatting.
10. Delete `src/helpers_legacy.cpp` and `src/helpers_legacy.h` after no remaining compiled file includes the legacy helper header.

## ./src/mpdpp.cpp
1. Treat `src/c/ncm_mpd_client.c`, `src/c/ncm_mpd_connection.c`, `src/c/ncm_mpd_item.c`, `src/c/ncm_song.c`, `src/c/ncm_directory.c`, and `src/c/ncm_playlist.c` as the existing C replacement for the C++ MPD wrapper.
2. Replace every `#include "mpdpp.h"` with the specific C headers needed by that file.
3. Replace the global `Mpd` object with `global_mpd` plus explicit `ncm_mpd_client_*` calls.
4. Replace C++ exceptions with `NcmError` returns and statusbar/app error reporting at every MPD call site.
5. Replace C++ `MPD::Song`, `MPD::Directory`, `MPD::Playlist`, and `MPD::Item` value usage with the matching C structs and explicit init/copy/move/destroy ownership rules.
6. Replace C++ iterator APIs with C list/array iteration helpers, preserving ownership and avoiding borrowed-pointer lifetime bugs.
7. Add any missing thin C wrappers that only exist through `mpdpp.cpp`, especially supported-extension and output helper calls used by browser/status/visualizer paths.
8. Convert search/list/directory/queue/playback/playlist command-list operations one group at a time and remove the matching `MPD::Connection` method dependencies after each group.
9. Verify all converted call sites compile without `mpdpp.h`, and add tests around MPD item conversion, command-list construction, and error propagation where fake-client coverage exists.
10. Delete `src/mpdpp.cpp` and `src/mpdpp.h`, then remove `src/mpdpp.cpp` from `APP_CXX_SRCS`.
