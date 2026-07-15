# Native playlist migration validation

## Scope

This report records the final static validation for the migration of the main
playlist screen from the deleted C++ `Playlist` class to
`NativePlaylistScreen` in `src/screens/nc_playlist.c`.

## Formatting

The C files changed by the playlist migration were reviewed against:

- `c-style.md`
- `c-format.md`
- `c-guidelines.md`
- `style.sed`

Validation includes line length, indentation, trailing whitespace, direct
header dependencies, control-flow braces, declarations, and delimiter balance.
Standard-library calls retained in tests are limited to their test and POSIX
interface roles.

Static checks completed successfully for all 16 C files changed by the
playlist migration:

- Maximum 80-character lines
- Four-space indentation without tabs
- No trailing whitespace
- Balanced C delimiters and preprocessor conditionals
- Direct playlist migration symbol and include audit

## Build configurations and sanitizers

The following commands were intentionally not run in this environment, per the
request not to compile without the required development libraries:

```sh
make clean
make
make check
```

The same restriction applies to TagLib-enabled, TagLib-disabled,
AddressSanitizer, and UndefinedBehaviorSanitizer builds. These remain required
release checks in an environment with the project dependencies installed.

## Runtime exercise checklist

Interactive runtime exercise was not possible without building the program.
The corresponding source-level regression coverage is listed below so the
checks can be executed with `make check` before merging.

| Behavior | Regression coverage |
| --- | --- |
| Empty playlist | `test_native_mpd_updates_and_membership` |
| Large playlist loading | `test_native_large_playlist` |
| Duplicate songs | `test_native_mpd_updates_and_membership` |
| Filtered playlist | `test_native_load_filter_search_selection` |
| Classic display mode | `test_native_display_and_column_title` |
| Columns display mode | `test_native_display_and_column_title` |
| Titles enabled/disabled | `test_native_display_and_column_title` |
| Move selected entries | `test_move_selected_items_up_down` |
| Move range to a position | `test_move_selected_items_to` |
| Shuffle | `test_shuffle_confirmation` |
| Reverse | `test_reverse_selected_range` |
| Crop | `test_crop_confirmation_and_range` |
| Clear | `test_clear_confirmation_and_immediate_state` |
| Set priority | `test_priority_prompt` |
| Current-song centering | `test_autocenter_locates_playing_song` |
| Incremental MPD updates | `test_native_mpd_updates_and_membership` |
| Screen switching and resize | `test_native_only_registration` |
| Current-song formatting | `test_native_now_playing_formatting` |
| Enter does not append | `test_play_item_does_not_append_playlist_song` |

Large-playlist loading and navigation have source-level regression
coverage. Responsiveness remains an interactive release test because timing
and terminal redraw behavior depend on ncurses and the local MPD queue.

## Migration boundary

`NCM_SCREEN_TYPE_PLAYLIST` is classified as C-only. The 101 playlist actions
are an explicit whitelist evaluated before the generic C-safe action list.
Actions that are globally implemented in C but belong to another screen, such
as `reset_search_engine`, sort-dialog movement, and media-library mode toggles,
are rejected on the playlist.

`tests/c_playlist_binding_dispatch_test.c` verifies that:

- Whitelisted playlist bindings execute through `ncm_action_runtime_run()`.
- Direct whitelisted playlist actions execute through the native runtime.
- Neither path invokes the legacy action or binding executor.
- A globally C-safe action absent from the playlist whitelist is blocked and
  is not forwarded to the legacy runtime.

## Remaining C++ inventory

The remaining C++ translation units are:

```text
./src/actions_legacy.cpp
./src/mpdpp.cpp
./src/settings_legacy.cpp
./tests/settings_legacy_sync_test.cpp
```

The inventory contains four C++ translation units and 31 C++-detected
headers. The original tree contained 32 C++-detected headers; the removed item
is `src/screens/playlist.h`. No replacement playlist compatibility header was
introduced.
