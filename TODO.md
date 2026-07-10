# ncmpcpp2

Main goal: remove C++ from the project while preserving all current behavior.

## Project-wide migration rules

- Treat the existing C++ implementation as the behavioral specification until
  equivalent C code and tests exist.
- Use the native C screen, menu, MPD, settings, formatting, buffer, and error
  APIs already present in `src/` and `src/c/`; do not add a second compatibility
  framework.
- Make ownership explicit for every string, song, list, window, callback, file
  descriptor, and external-library resource introduced by a conversion.
- A C++ file may be deleted only after its C replacement is used by the
  application, its former callers no longer include its C++ header, and its
  behavior is covered by focused tests plus `make all` and `make check`.
- The repository currently contains additional `.cpp` files not listed below:
  `src/bindings_legacy.cpp`, `src/helpers_legacy.cpp`,
  `src/screens/lastfm.cpp`, `src/screens/lyrics.cpp`,
  `src/status_legacy.cpp`, `src/statusbar_legacy.cpp`, and
  `src/title_legacy.cpp`. They require separate
  removal work before the whole source tree is C-only.

# C++ files to remove or convert to C

## `./src/screens/media_library.cpp`

1. **Replace the placeholder native contract with the complete C state model.**
   In `nc_media_library.h`, replace the ambiguous `columns` integer with an
   explicit three-column, two-column, and album-only mode enum while keeping
   tags, albums, and songs as distinct active-column values. Add the legacy
   250-ms fetch-delay/window-timeout state, per-column filter and search state,
   title buffers, and a C-only hook table whose production implementation uses
   `NcmMpdClient` and whose test implementation can return deterministic lists
   and errors. Expose typed accessors for the current tag, current album, mode,
   visible song rows, update timer, sort toggle, mode toggle, and locate/add
   operations needed by callers that currently dereference `myLibrary`.
2. **Implement exact mode, geometry, menu, and rendering behavior.** Configure
   all three C menus with cyclic scrolling, centered cursor, selected
   prefix/suffix, and active versus inactive-column highlight prefixes. Make
   the mode cycle exactly three-column -> two-column -> album-only ->
   three-column, hide the tags window in both two-column modes, and calculate
   widths from `media_library_column_width_ratio_three` or
   `media_library_column_width_ratio_two` after obtaining locked-screen-aware
   resize parameters. Add row draw callbacks for locale-converted tags,
   `Config.empty_tag`, the exact album/date/primary-tag text, “All tracks,” and
   `Config.song_library_format`; refresh every visible menu, draw the proper
   separators, maintain the legacy dynamic titles, and render “No albums
   found.” when appropriate.
3. **Build and test the native grouping and ordering primitives before MPD I/O.**
   Add pure C helpers that populate tag, album, and song arrays from
   `NcmMpdStringList`/`NcmMpdSongList`, enumerate every value of the configured
   primary tag, split albums by date only when configured, create the separator
   and all-tracks row only for multiple albums, and preserve empty tag/album
   display cases. Implement locale-aware tag/album comparators honoring
   `Config.ignore_leading_the`, descending mtime sorting, and song comparison by
   date, album, disc, track number, then rendered library text. Match the
   legacy duplicate-mtime rules explicitly—maximum mtime in three-column
   aggregation and last encountered mtime in the two-column recursive
   aggregation—unless that behavior is changed separately with its own test.
4. **Port the exact MPD query pipeline for every mode.** In three-column name
   sort, load primary tags with `ncm_mpd_client_get_list()`; in three-column
   mtime sort, recursively load the database and aggregate all primary-tag
   values; and load albums by an exact search for the highlighted tag. In
   two-column and album-only modes, recursively load the database and aggregate
   `(tag, album, date)` or `(empty tag, album, date)` keys, preserving the
   legacy rule that album-only aggregation still iterates songs through their
   primary-tag values. Load songs with exact primary-tag, album, and optional
   date constraints, omitting album/date for the all-tracks row and omitting
   the primary-tag constraint only in album-only mode. Convert the C++
   exception fallbacks into explicit C error paths: failed recursive two-column
   loading advances to the next column mode, failed mtime tag loading switches
   back to name sorting, request flags remain pending, and the original MPD
   error is reported without retaining partial rows.
5. **Port the delayed update and dependent-column state machine.** Make
   `native_library_update()` process only the requested or due level, rebuild
   menus through temporary arrays, swap them in only after successful fetches,
   preserve highlights by tag/album/song identity, and reapply each column’s
   existing filter after replacement. When a tag highlight changes, clear
   albums and songs and restart the timer; when an album highlight changes,
   clear songs and restart the timer. Fetch empty dependent columns only after
   250 ms when `Config.data_fetching_delay` is enabled and immediately
   otherwise, return a 250-ms window timeout while albums or songs are pending,
   and clear each update flag only after that level succeeds. Move this
   list-change finalization into a native API/callback so it no longer depends
   on `actions_legacy.cpp::listsChangeFinisher()`.
6. **Finish navigation, filtering, searching, mouse handling, and song actions.**
   Make previous/next-column availability depend on the actual visible mode and
   a nonempty destination menu rather than enum arithmetic; preserve active and
   inactive highlighting while moving between columns. Implement the native
   mouse callback with the legacy left-click selection, right-click add/play,
   column switching, scrolling, and dependent-list clearing behavior. Replace
   the current URI-only song matching and literal filter regex with
   `Config.regex_type` matching against the same rendered tag/album/song text
   used on screen; keep separators and “All tracks” during filtering but skip
   them during search. Expand selected-song extraction and add/play so tags,
   selected albums, all-tracks, and selected song rows issue the same searches,
   ordering, playlist insertion, playback, and status messages as the C++
   implementation.
7. **Port robust locate-song behavior and migrate every external consumer.**
   Reimplement `locateSong` in C with the database-song and missing-primary-tag
   checks, screen switch and progress message, filter clearing, on-demand
   fetches, insertion of tags/albums omitted by limited MPD servers, date-less
   retry for Spotify-style entries, invalid-row cleanup, exact song selection,
   and fallback to the previous nonempty column. Then replace all direct
   `myLibrary` uses in `actions_legacy.cpp` and `status_legacy.cpp`: screen
   construction/registration and resize flags, database update observers,
   inactive song-column refresh, artist lookup, edit-library-tag and
   edit-library-album inputs/song iteration, jump/show/playing-song actions,
   tag-type/sort/mode actions, and list-change cleanup. Register
   `native_c_screen_media_library()` directly and remove the duplicate legacy
   observer once the C status path is authoritative.
8. **Prove parity, cut over the build, and delete the facade.** Expand
   `tests/c_media_library_screen_test.c` with deterministic hook fixtures for
   all three modes, exact generated MPD queries, multi-valued and empty tags,
   duplicate mtime behavior, date splitting, all-tracks separators, every sort,
   delayed fetches, request/error fallback, highlight/filter preservation,
   rendered-string search, mouse transitions, selected songs from every
   column, add/play, missing-server entries, and locate-song empty-result
   recovery; add a curses smoke test for geometry, titles, separators, and
   active/inactive highlights. After the native screen is the registered
   implementation and no caller references `MediaLibrary`, `myLibrary`, or its
   nested row types, remove the textual `#include "screens/media_library.cpp"`
   from `actions_legacy.cpp`, delete `media_library.cpp` and
   `media_library.h`, scan for stale symbols/includes, and require `make all`
   plus `make check` to pass.

## `./src/screens/sel_items_adder.cpp`

1. **Define the complete C workflow and ownership contract.** Document the
   legacy paths from the source screen to current, new, existing, and cancelled
   playlist targets; identify which screen owns the copied `NcmSongArray`, the
   previous-screen reference, prompt text, playlist name, and final status; and
   extend `NativeSelectedItemsAdderAction` where that information is missing.
2. **Finish dialog lifecycle and presentation parity.** On switch, copy selected
   songs from the previous native `HasSongs` implementation, reject an empty
   selection with the legacy status message, remember the previous screen,
   center and resize both menus with the 60%, 66%, 35-column, and 11-row limits,
   draw the previous screen behind the dialog, preserve its title, and match
   left/right mouse activation and cancellation behavior.
3. **Complete playlist-target population.** Fetch stored playlists with
   `ncm_mpd_client_get_playlists()`, copy their paths into owned C action rows,
   locale-sort them with `Config.ignore_leading_the`, add separators in the
   legacy positions, and enforce local-browser restrictions by omitting new and
   stored playlist targets for songs outside the MPD database.
4. **Implement new and stored playlist execution.** Add a C statusbar prompt for
   “Save playlist as:”, handle cancellation and empty names, and use one MPD
   command list to append every selected song to the requested stored
   playlist. Ensure partial setup failures destroy owned strings/actions,
   report the exact MPD error, and return to the previous screen only after a
   completed or explicitly cancelled operation.
5. **Implement all current-queue insertion positions.** Calculate positions for
   end, beginning, after the current song, after the complete current album,
   and after the highlighted song using native playlist/status APIs. Preserve
   song order when inserting multiple items, reject stopped/no-current-song
   states where the legacy screen does, and route execution through the common
   C add-mode helper rather than duplicating MPD commands.
6. **Wire dialog actions into the C action runtime.** Make `NCM_ACTION_RUN_ACTION`
   execute the highlighted target/position row, make `NCM_ACTION_ADD` behave
   correctly while either menu is active, switch between selector menus for
   “Current playlist” and “Cancel”, emit success/error status messages, refresh
   affected playlist screens, and restore the previous screen deterministically.
7. **Add complete dialog tests.** Extend
   `tests/c_playlist_management_dialogs_test.c` to cover song transfer, local
   browser restrictions, playlist sorting, owned-name cleanup, new-playlist
   prompt success/cancel, all five insertion positions, stopped playback,
   missing highlights, MPD command-list failures, search, mouse behavior,
   resize, and previous-screen restoration.
8. **Remove the C++ facade.** Replace `SelectedItemsAdder`,
   `mySelectedItemsAdder`, `sel_items_adder.h`, and all interface casts with
   `NativeSelectedItemsAdderScreen`; remove its textual include from
   `actions_legacy.cpp`; then delete the C++ source and obsolete header after
   all callers and tests use only C types.

## `./src/screens/sort_playlist.cpp`

1. **Extract an exact sorting specification.** Preserve the eleven configurable
   sort keys and terminator row, row-reordering rules, selected-range behavior,
   locale comparison, `Config.ignore_leading_the`, per-key tag lookup, and the
   original queue position as the final deterministic tie-breaker.
2. **Add a C sort-operation API separate from the dialog UI.** Define a request
   containing the selected queue range and ordered `NcmSongGetter` keys, and a
   result containing the target song order or swap operations. Obtain the range
   and a copied `NcmSongArray` from `NativePlaylistScreen` without exposing menu
   internals or retaining borrowed song pointers across MPD operations.
3. **Implement the comparator and stable in-memory order.** Use the project’s C
   locale/string comparison helpers and `ncm_song_getter_buffer()` for
   every selected key, stop at `NCM_SONG_GETTER_NONE`, and compare original
   queue positions last. Use a deterministic C sorting algorithm whose result
   can be tested independently; do not port the recursive C++ lambda or depend
   on random pivots.
4. **Translate the target order into valid MPD queue operations.** Build a
   position map for the selected range, generate swaps or moves that transform
   the current range without disturbing songs outside it, update the local map
   after every operation, and verify that duplicate/equal songs are identified
   by queue identity or original position rather than tag equality.
5. **Execute the sort atomically through the C MPD client.** Start a command
   list, queue each `ncm_mpd_client_swap()`/move operation with absolute queue
   positions, commit it, surface the first `NcmError`, and request a playlist
   refresh after success or failure so the UI cannot retain a stale local
   order. Preserve “Sorting...” and “Range sorted” status behavior.
6. **Connect the native dialog to the operation.** Make the Sort row execute the
   C sorting API, make Cancel return without MPD traffic, make
   `NCM_ACTION_RUN_ACTION`, `NCM_ACTION_MOVE_SORT_ORDER_UP`, and
   `NCM_ACTION_MOVE_SORT_ORDER_DOWN` operate on the native dialog, and restore
   the previous playlist screen after either terminal action.
7. **Test both ordering and MPD commands.** Extend
   `tests/c_playlist_management_dialogs_test.c` with every key, reordered keys,
   stable ties, leading-article handling, full and selected ranges, empty and
   one-song ranges, duplicates, absolute offsets, exact generated command
   sequences, command-list errors, cancel behavior, and screen restoration.
8. **Remove the C++ facade.** Replace `SortPlaylistDialog`,
   `mySortPlaylistDialog`, and `sort_playlist.h` callers with
   `NativeSortPlaylistDialog`, remove the textual source include from
   `actions_legacy.cpp`, and delete the C++ source/header only after no
   `sort_requested` flag is left without a consumer and the real queue-sort
   path is tested end to end.

## `./src/screens/visualizer.cpp`

1. **Create a visualizer parity matrix and expand native state.** Account for
   FIFO and UDP sources, source/output identifiers, reset behavior, mono and
   stereo samples, all four visualization types, timing, autoscale, adaptive
   sample consumption, configured glyphs/colors, frequency settings, FFTW
   allocations, lock/merge capability, and every cleanup path. Correct the
   native screen’s lockability to match the legacy screen.
2. **Implement a C data-source layer.** Parse `visualizer_data_source` and the
   deprecated FIFO setting, distinguish filesystem paths from `host:port`,
   create/open a nonblocking FIFO without losing an existing valid FIFO, resolve
   and bind UDP sockets, expose read/close operations, and close file
   descriptors on every failed setup, screen teardown, and source change.
3. **Port MPD output discovery and reset behavior.** Fetch outputs through
   `ncm_mpd_client_get_outputs()`, match `Config.visualizer_output_name`, retain
   the output id only for FIFO-backed sources, report a missing named output,
   and reproduce disable/enable reset behavior when reopening the source or
   recovering from a stalled stream.
4. **Complete sample ingestion and timing.** Read nonblocking PCM data into the
   incoming buffer, retain partial samples, split stereo channels correctly,
   calculate required and rendered sample counts from 44.1 kHz, FPS, width, and
   visualization type, adapt the consumption rate to backlog, apply autoscale
   growth/decay and clipping, clear stale frames, and return the exact
   `1000 / visualizer_fps` timeout.
5. **Port non-spectrum renderers to `NcWindow`.** Implement wave, filled wave,
   mono ellipse, and stereo ellipse drawing with the configured UTF-8 glyphs,
   color cycling, center lines, dimensions, channel split, and out-of-range
   protections. Make resize and type changes recompute sample requirements and
   clear the native window before the next frame.
6. **Port the FFT spectrum with explicit FFTW lifecycle.** Allocate input,
   complex output, plan, magnitudes, frequency-space, and bar-height arrays from
   the C screen state; apply the legacy window function; support linear/log X
   spacing, optional log Y scaling, gain/dynamic range, cubic/linear
   interpolation, smooth and legacy smooth glyphs, stereo rendering, and
   destroy every FFTW/resource allocation exactly once.
7. **Integrate and test the complete screen.** Make switch/update/resize/toggle
   actions initialize and operate the source, draw frames, reset output when
   required, and report recoverable I/O/MPD errors without exceptions. Extend
   `tests/c_lastfm_lyrics_visualizer_test.c` with fake FIFO/UDP reads, partial
   frames, source failures, output matching/reset, autoscale/rate adaptation,
   each renderer, FFT frequency fixtures, mono/stereo, resize, type cycling,
   timeout, lockability, and cleanup; add a manual real-MPD smoke test.
8. **Remove the C++ facade.** Replace `Visualizer`, `myVisualizer`,
   `visualizer.h`, and `actions_legacy_runtime_visualizer_setup_datasource()`
   with native C APIs and status hooks; remove the textual source include from
   `actions_legacy.cpp`; then delete the C++ source/header after valgrind or
   sanitizer runs show no descriptor, buffer, or FFTW leaks.

## `./src/configuration_legacy.cpp`

1. **Build a differential configuration specification.** Compare every legacy
   command-line option, default, validation rule, help/version output, XDG and
   `~/.ncmpcpp` path candidate, MPD environment variable, CLI override,
   directory-creation rule, current-song formatter option, test-fetcher mode,
   quiet mode, and exit condition against `configuration.c`.
2. **Add exhaustive C parser tests before changing startup.** Extend
   `tests/c_configuration_test.c` with every short and long option, combined
   arguments, missing/invalid values, duplicate path options, environment/CLI
   precedence, XDG unset/empty cases, legacy path fallback, `~` expansion,
   unreadable files, directory creation failures, exact diagnostics, and modes
   that intentionally print and exit.
3. **Remove the startup double-configuration pass.** Change
   `ncmpcpp_legacy_configure()`/`ncmpcpp.c` to call `configure()` exactly once,
   initialize and destroy `Config` and `Bindings` in one documented order, and
   remove the current sequence that first invokes
   `actions_legacy_runtime_configure()` and then resets bindings and runs the C
   parser again.
4. **Migrate every C++ path-expansion caller.** Replace the `std::string`
   overload in `configuration_legacy.h`, including calls from
   `settings_legacy.cpp` and `screens/browser.h`, with the owned C
   `expand_home(char **, int32 *, NcmError *)` API or a purpose-built C buffer
   helper. Handle allocation failure and preserve the caller’s previous value
   when expansion fails.
5. **Make C configuration the sole settings/bindings loader.** Route discovered
   config paths through `configuration_read()` and binding paths through
   `ncm_bindings_configuration_read_paths()`, preserve “first existing file”
   and explicit-file error semantics, and eliminate conversions into
   `ConfigurationLegacy`, C++ option-parser structures, or C++ vectors.
6. **Port remaining one-shot CLI behavior to direct C services.** Ensure
   current-song printing uses `NcmFormatAst`/`NcmSong`, fetcher tests use the C
   lyrics registry, MPD connection validation reports `NcmError`, and help,
   version, screen-selection, host, port, password, timeout, and quiet options
   produce the same observable behavior and exit status as the legacy path.
7. **Cut all bridge and build references.** Remove
   `actions_legacy_runtime_configure()`, the C++ `configure(int, char **)`
   declaration, `configuration_legacy.h` includes, and
   `src/configuration_legacy.cpp` from `APP_CXX_SRCS`. Run CLI integration tests
   in isolated HOME/XDG directories in addition to `make all` and `make check`.
8. **Delete the legacy implementation.** Delete
   `configuration_legacy.cpp` and `configuration_legacy.h`, scan for legacy
   symbols and duplicate configuration passes, and verify startup, all
   print-and-exit modes, config/bindings discovery, and MPD connection setup
   using only the C implementation.

## `./src/mpdpp.cpp`

1. **Create a method-by-method MPD parity table.** Map every `MPD::Connection`,
   `Statistics`, `Status`, `Directory`, `Playlist`, `Item`, and iterator method
   in `mpdpp.h/.cpp` to `NcmMpdClient`, `NcmMpdConnection`, `NcmMpdStats`,
   `NcmMpdStatus`, `NcmDirectory`, `NcmPlaylist`, `NcmMpdItem`, and typed C
   arrays/lists. Mark differences in return values, command-list state, error
   clearing, object ownership, and no-idle callback timing.
2. **Fill any missing low-level C functionality first.** Add focused C client
   or connection functions for every uncovered command/query rather than
   emulating them in screens, including queue/stored-playlist operations,
   searches and tag lists, directory variants, replay gain, random additions,
   outputs, URL handlers, tag types, idle/no-idle, password handling, and raw
   connection access required by polling.
3. **Replace C++ MPD value wrappers in all callers.** Convert `MPD::Song`,
   `MPD::Directory`, `MPD::Playlist`, and `MPD::Item` fields, parameters, return
   values, variants, comparisons, and display helpers to the native C structs.
   Define copy/move/destroy behavior explicitly and remove helpers such as
   `ncm_playlist_cpp_path()` once no C++ object needs them.
4. **Replace lazy C++ iterators with owned typed C results.** Migrate every
   `SongIterator`, `ItemIterator`, `DirectoryIterator`, and `StringIterator`
   loop to `NcmMpd*List` or project typed arrays, initialize the result before
   the request, destroy it on every exit path, and preserve response-finish and
   partial-response semantics that the iterator destructor previously handled.
5. **Replace exception-based errors with explicit results.** Convert
   `MPD::Error`, `ClientError`, and `ServerError` call sites to `bool` plus
   `NcmError`, centralize client/server statusbar formatting and recoverable
   connection clearing, and ensure each command either returns success or
   leaves a fully populated error without throwing across a C boundary.
6. **Remove the global `Mpd` facade and C++ command state.** Change all callers
   to use `global_mpd` and `ncm_mpd_client_*`, preserve command-list prechecks,
   nested-list rejection, idle cancellation, no-idle callback dispatch, MPD
   version checks, and reconnect behavior, and move any remaining raw
   `mpd_connection` use behind the C connection API.
7. **Add protocol-wrapper parity tests.** Use a fake or scripted MPD server to
   test every command and collection conversion, command-list success/failure,
   malformed and partial responses, client versus server errors, reconnect,
   password failure, idle/no-idle callbacks, copied object lifetime, list
   destruction, and all return values formerly supplied by C++ wrappers.
8. **Remove the C++ MPD layer.** Eliminate all `#include "mpdpp.h"`, `MPD::`,
   and `Mpd.` references—including dormant C++ screens/helpers not built as
   separate objects—then delete `mpdpp.cpp`, `mpdpp.h`, and obsolete C++ song
   adapters. Remove its Makefile object and verify no C++ runtime symbols are
   pulled into the final binary.

## `./src/settings_legacy.cpp`

1. **Establish field and option parity mechanically.** Compare every member of
   `ConfigurationLegacy` with `Configuration`, and every one of the 134 option
   names already present in both readers, including defaults, aliases,
   deprecated options, duplicate handling, conversion functions, validation,
   feature guards, and post-processing. Store this mapping in a test fixture or
   generated audit so future changes cannot silently diverge.
2. **Add golden tests for the complete settings surface.** For each option,
   test its default and at least one valid non-default value; add invalid,
   duplicate, unknown, deprecated, missing-feature, enum, color, format,
   column-list, ratio, regex, path, lyrics-fetcher, screen-sequence, and range
   cases. Verify exact final C fields and errors rather than only checking that
   parsing succeeds.
3. **Migrate all consumers to the C `Config` object.** Replace
   `settings_legacy.h`, `ConfigurationLegacy`, and `ConfigLegacy` use in
   display, helpers, browser, playlist, search, tag editing, lyrics, status,
   statusbar, title, comparators, and the four legacy screen facades with
   `settings.h` fields or narrow C accessor functions.
4. **Replace C++-only setting types.** Convert `std::string`, `std::vector`,
   `std::optional`, C++ regex flags, `Format::AST`, C++ colors/buffers, columns,
   screen sequences, width ratios, visualizer colors, and lyrics fetcher
   containers to the existing owned strings, typed arrays, `NcmFormatAst`,
   `NcColor`/`NcBuffer`, enums, and explicit optional-state fields in C.
5. **Make initialization and mutation single-source.** Ensure defaults are
   initialized once by `settings_initialize_defaults()`, files mutate only
   `Config`, runtime actions toggle only C fields, dependent derived data is
   rebuilt in one place, and `configuration_destroy()` frees every allocation
   once. Remove synchronization or macro aliases between legacy and native
   globals.
6. **Port all custom apply/post-processing behavior.** Preserve home expansion,
   directory normalization, MPD host/password parsing, format compilation,
   column parsing, ratio validation, startup/locked screen validation, regex
   selection, visualizer source compatibility, lyrics fetcher lookup, ignored
   error behavior, and feature-dependent defaults using C helpers and
   `NcmError`.
7. **Prove lifecycle and runtime parity.** Run the expanded settings tests,
   startup/config integration tests, action tests that mutate settings, and
   address/undefined-behavior sanitizers to catch leaks, stale views, double
   frees, and use-after-reallocation in all owned strings and arrays.
8. **Remove the legacy settings layer.** Delete all
   `settings_legacy.h` includes and `ConfigLegacy` references, remove
   `src/settings_legacy.cpp` from `APP_CXX_SRCS`, delete the source/header, and
   verify documentation defaults, startup behavior, screen construction, and
   runtime setting toggles use only `Configuration Config`.

## `./src/actions_legacy.cpp`

1. **Build an exhaustive action migration table.** For every
   `NcmActionType`, compare the legacy class’s `canBeRun()` and `run()` behavior
   with `action_runtime_builtin_can_run()` and
   `action_runtime_builtin_run()`. Classify actions as complete, behaviorally
   different, hard-coded false, hybrid-only, or dependent on a legacy screen,
   and make this table the checklist for removing entries from
   `app_binding_migration.c`.
2. **Move application-runtime services out of the action translation unit.**
   Implement C ownership for readline initialization, header/footer windows,
   prompt hooks, screen initialization, resize, environment updates, playlist
   highlighting, browser extension loading, visualizer source setup, and exit
   state in `ncmpcpp.c`, `app_controller`, `status`, `ui_state`, and the
   relevant screen modules. Remove each corresponding
   `actions_legacy_runtime_*` entry after its direct C caller is active.
3. **Complete generic C screen capabilities.** Add or finish native interfaces
   for current item/song access, selected songs/ranges, run-current-row,
   columns, parent navigation, filters/search, selection operations, album
   selection, found-item navigation, edit targets, and mouse dispatch. Replace
   legacy `dynamic_cast`/screen-global decisions with `NcScreen` callbacks or
   explicit screen-type APIs.
4. **Port every missing domain action into its owning C module.** Implement the
   currently false or bridge-only prompt and mutation paths—including save
   playlist, browser deletion, move-to-position, start/find navigation,
   crossfade/volume prompts, song-position jump, tag/library/directory/playlist
   edits, album/found-item selection, random additions, priorities, and dialog
   run actions—using the C MPD, filesystem, tag, lyrics, playlist, and screen
   APIs rather than rebuilding a monolithic action file.
5. **Make bindings execute entirely in C.** Support normal actions,
   require-runnable, require-screen, pushed characters, external commands, and
   console commands through `NcmBindingRuntime`; preserve sequence short-circuit
   rules and prompt input behavior; then remove hybrid and forced-legacy lists
   from `app_binding_migration.c` one action at a time until
   `ncmpcpp_legacy_execute_binding()` is unnecessary.
6. **Replace all exception and C++ helper handling.** Convert conversion,
   bounds, prompt-abort, MPD client/server, filesystem, regex, and unexpected
   errors to explicit C return values and `NcmError`; centralize statusbar
   reporting; and remove dependencies on C++ helpers, `std::filesystem`,
   `std::regex`, streams, lambdas, and C++ screen globals.
7. **Add action and application integration coverage.** Extend
   `tests/c_actions_test.c` so every action has positive and negative
   `can_run` cases, successful execution, failure/error reporting, and expected
   MPD/screen/config side effects. Add binding-chain tests, prompt tests, every
   screen context, update/resize/reconnect paths, and an end-to-end terminal
   smoke test that starts, navigates, controls MPD, and exits without invoking
   a legacy symbol.
8. **Perform the final legacy-runtime cutover.** Make `ncmpcpp.c` call only C
   configuration, window, controller, status, binding, and action APIs; delete
   `app_legacy_bridge.*`, `actions_legacy_runtime.h`, the textual screen-source
   includes, `actions_legacy.cpp`, and its header/object. After the separately
   listed and additional C++ files are gone, remove `APP_CXX_SRCS`, `CXX`,
   `CXXFLAGS`, the C++20 tool check, and C++ linker use from the Makefile, then
   require a repository scan with no `.cpp`, C++ headers/symbols, or C++ runtime
   dependency before declaring the migration complete.
