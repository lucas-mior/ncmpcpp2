# Configuration option migration inventory

This is the step-1 baseline for the configuration-subsystem refactor.
It records current behavior; it is deliberately not an authoritative
production definition and should not be included by production code.

The step-1 source contained 135 `OPT(...)` invocations but 134 effective
option names: `visualizer_type` has two mutually exclusive default entries
under `#if defined(HAVE_FFTW3_H)`.

As of step 5, `src/configuration_options.def` is the authoritative production
definition for all 78 primitive options: `XX_BOOL`, `XX_INT_RANGE`,
`XX_DOUBLE_RANGE`, `XX_STRING`, `XX_PATH`, and `XX_DIR`.  It now generates
their `Configuration` fields as well as their apply wrappers and option-table
entries.  String, path, and directory entries generate both the owned pointer
and its `_len` companion.  The remaining 56 effective options are still
declared by the handwritten `SettingsOption` table.  This document remains the
behavioral migration inventory rather than a production include file.  Existing
primitive side effects and transforms are preserved by temporary generated-
wrapper pre/post handling until the later validation/runtime-application steps
separate them from parsing.

## Proposed X-macro type inventory

| Proposed type | Count |
| --- | ---: |
| `XX_BOOL` | 47 |
| `XX_ENUM` | 13 |
| `XX_FORMATTED_COLOR` | 11 |
| `XX_INT_RANGE` | 11 |
| `XX_BUFFER_WIDTH` | 8 |
| `XX_STRING` | 8 |
| `XX_FORMAT` | 7 |
| `XX_PATH` | 5 |
| `XX_DOUBLE_RANGE` | 4 |
| `XX_COLOR` | 3 |
| `XX_DIR` | 3 |
| `XX_RATIO` | 3 |
| `XX_BORDER` | 2 |
| `XX_BUFFER` | 2 |
| `XX_LOOK` | 2 |
| `XX_COLUMNS` | 1 |
| `XX_FORMATTED_COLOR_LIST` | 1 |
| `XX_LYRICS_FETCHERS` | 1 |
| `XX_OPTIONAL_ENUM` | 1 |
| `XX_SCREEN_LIST` | 1 |
| **Total effective options** | **134** |

All integer settings are classified as `XX_INT_RANGE`, and all floating
settings as `XX_DOUBLE_RANGE`; the current behavior column below records
what the code enforces today, including cases with no explicit bounds.

## Per-option baseline

| Option | Default | Proposed type | Configuration storage | Current parser | Current validation / transform / side effect | Lifecycle |
| --- | --- | --- | --- | --- | --- | --- |
| `ncmpcpp_directory` | `~/.config/ncmpcpp/` | `XX_DIR` | `ncmpcpp_directory`, `ncmpcpp_directory_len` | `APPLY_STRING_DIR` | Expand leading `~`; ensure a trailing `/`. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `lyrics_directory` | `~/.lyrics/` | `XX_DIR` | `lyrics_directory`, `lyrics_directory_len` | `APPLY_STRING_DIR` | Expand leading `~`; ensure a trailing `/`. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `mpd_host` | `localhost` | `XX_PATH` | `mpd_host`, `mpd_host_len` | `apply_mpd_host` custom body | Expand leading `~`; store string; immediately update `global_mpd`; the MPD setter also interprets embedded `password@host`. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `mpd_port` | `6600` | `XX_INT_RANGE` | `mpd_port` | `ncm_parse_int32` + option-specific checks | Parse `int32`; reject only values > 65535; negatives currently pass and are cast to `uint16`; immediately update `global_mpd`. | trivial scalar |
| `mpd_password` | `` | `XX_STRING` | `mpd_password`, `mpd_password_len` | `apply_mpd_password` custom body | Copy string; empty clears Configuration but does not call the MPD password setter; non-empty values immediately update `global_mpd`. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `mpd_music_dir` | `~/music` | `XX_DIR` | `mpd_music_dir`, `mpd_music_dir_len` | `APPLY_STRING_DIR` | Expand leading `~`; ensure a trailing `/`. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `mpd_connection_timeout` | `5` | `XX_INT_RANGE` | `mpd_connection_timeout` | `ncm_parse_int32` + option-specific checks | Parse unbounded `int32`; immediately set MPD timeout to value * 1000 ms. | trivial scalar |
| `mpd_crossfade_time` | `5` | `XX_INT_RANGE` | `mpd_crossfade_time` | `ncm_parse_int32` + option-specific checks | Parse `int32`; no explicit bounds today. | trivial scalar |
| `random_exclude_pattern` | `` | `XX_STRING` | `random_exclude_pattern`, `random_exclude_pattern_len` | `APPLY_STRING` | Copy value bytes verbatim. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `visualizer_data_source` | `/tmp/mpd.fifo` | `XX_PATH` | `visualizer_data_source`, `visualizer_data_source_len` | `APPLY_STRING_PATH` | Expand leading `~`; otherwise copy bytes. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `visualizer_output_name` | `Visualizer feed` | `XX_STRING` | `visualizer_output_name`, `visualizer_output_name_len` | `APPLY_STRING` | Copy value bytes verbatim. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `visualizer_in_stereo` | `yes` | `XX_BOOL` | `visualizer_in_stereo` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `visualizer_type` | `spectrum if `HAVE_FFTW3_H`, otherwise ellipse` | `XX_ENUM` | `visualizer_type` | `ncm_visualizer_type_parse` | Parse named value into enum-like storage. | trivial scalar |
| `visualizer_look` | `●▮` | `XX_LOOK` | `visualizer_look` | `apply_visualizer_look` custom body | Require exactly 2 UTF-8 characters. | `StrBuilder`; zero-init; `sb_free` on destroy |
| `visualizer_fps` | `60` | `XX_INT_RANGE` | `visualizer_fps` | `ncm_parse_int32` + option-specific checks | Parse `int32`; require 30..1000. | trivial scalar |
| `visualizer_autoscale` | `no` | `XX_BOOL` | `visualizer_autoscale` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `visualizer_spectrum_smooth_look` | `yes` | `XX_BOOL` | `visualizer_spectrum_smooth_look` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `visualizer_spectrum_smooth_look_legacy_chars` | `yes` | `XX_BOOL` | `visualizer_spectrum_smooth_look_legacy_chars` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `visualizer_spectrum_dft_size` | `2` | `XX_INT_RANGE` | `visualizer_spectrum_dft_size` | `ncm_parse_int32` + option-specific checks | Parse `int32`; require 1..5. | trivial scalar |
| `visualizer_spectrum_gain` | `10` | `XX_DOUBLE_RANGE` | `visualizer_spectrum_gain` | `ncm_parse_double` + option-specific checks | Parse `double`; require 0..100. | trivial scalar |
| `visualizer_spectrum_hz_min` | `20` | `XX_DOUBLE_RANGE` | `visualizer_spectrum_hz_min` | `ncm_parse_double` + option-specific checks | Parse `double`; require >= 1. | trivial scalar |
| `visualizer_spectrum_hz_max` | `20000` | `XX_DOUBLE_RANGE` | `visualizer_spectrum_hz_max` | `ncm_parse_double` + option-specific checks | Parse `double`; require >= current `visualizer_spectrum_hz_min + 1` (order-dependent cross-field validation). | trivial scalar |
| `visualizer_spectrum_log_scale_x` | `yes` | `XX_BOOL` | `visualizer_spectrum_log_scale_x` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `visualizer_spectrum_log_scale_y` | `yes` | `XX_BOOL` | `visualizer_spectrum_log_scale_y` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `visualizer_color` | `blue, cyan, green, yellow, magenta, red` | `XX_FORMATTED_COLOR_LIST` | `visualizer_color` | `apply_visualizer_color` custom body | Comma-separated formatted colors; ignore empty items; at least one item required. | array of owned `NcFormattedColor`; clear elements and free storage |
| `system_encoding` | `` | `XX_STRING` | `system_encoding`, `system_encoding_len` | `apply_system_encoding` custom body | Compatibility/no-op today: discard input and clear stored string. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `playlist_disable_highlight_delay` | `5` | `XX_INT_RANGE` | `playlist_disable_highlight_delay` | `ncm_parse_int32` + option-specific checks | Parse `int32`; no explicit bounds today. | trivial scalar |
| `message_delay_time` | `5` | `XX_INT_RANGE` | `message_delay_time` | `APPLY_UINT` | Parse `int32`; no explicit bounds today. | trivial scalar |
| `song_list_format` | `{%a - }{%t}\|{$8%f$9}$R{$3%l$9}` | `XX_FORMAT` | `song_list_format` | `settings_parse_format` | Parse format expression with `NCM_FORMAT_FLAG_ALL`. | `NcmFormatAst`; zero-init; destroy AST |
| `song_status_format` | `{{%a{ "%b"{ (%y)}} - }{%t}}\|{%f}` | `XX_FORMAT` | `song_status_format` | `settings_parse_format` | Parse format expression with `NCM_FORMAT_FLAG_ALL ^ NCM_FORMAT_FLAG_OUTPUT_SWITCH`. | `NcmFormatAst`; zero-init; destroy AST |
| `song_library_format` | `{%n - }{%t}\|{%f}` | `XX_FORMAT` | `song_library_format` | `settings_parse_format` | Parse format expression with `NCM_FORMAT_FLAG_ALL`. | `NcmFormatAst`; zero-init; destroy AST |
| `alternative_header_first_line_format` | `$b$1$aqqu$/a$9 {%t}\|{%f} $1$atqq$/a$9$/b` | `XX_FORMAT` | `alternative_header_first_line_format` | `settings_parse_format` | Parse format expression with `NCM_FORMAT_FLAG_ALL ^ NCM_FORMAT_FLAG_OUTPUT_SWITCH`. | `NcmFormatAst`; zero-init; destroy AST |
| `alternative_header_second_line_format` | `{{$4$b%a$/b$9}{ - $7%b$9}{ ($4%y$9)}}\|{%D}` | `XX_FORMAT` | `alternative_header_second_line_format` | `settings_parse_format` | Parse format expression with `NCM_FORMAT_FLAG_ALL ^ NCM_FORMAT_FLAG_OUTPUT_SWITCH`. | `NcmFormatAst`; zero-init; destroy AST |
| `current_item_prefix` | `$(yellow)$r` | `XX_BUFFER_WIDTH` | `current_item_prefix`, `current_item_prefix_length` | `settings_copy_nc_buffer` | Parse color/format markup and cache width; `keep_existing=true`. | `NcBuffer` + cached width; zero-init; destroy buffer |
| `current_item_suffix` | `$/r$(end)` | `XX_BUFFER_WIDTH` | `current_item_suffix`, `current_item_suffix_length` | `settings_copy_nc_buffer` | Parse color/format markup and cache width; `keep_existing=true`. | `NcBuffer` + cached width; zero-init; destroy buffer |
| `current_item_inactive_column_prefix` | `$(white)$r` | `XX_BUFFER_WIDTH` | `current_item_inactive_column_prefix`, `current_item_inactive_column_prefix_length` | `settings_copy_nc_buffer` | Parse color/format markup and cache width; `keep_existing=true`. | `NcBuffer` + cached width; zero-init; destroy buffer |
| `current_item_inactive_column_suffix` | `$/r$(end)` | `XX_BUFFER_WIDTH` | `current_item_inactive_column_suffix`, `current_item_inactive_column_suffix_length` | `settings_copy_nc_buffer` | Parse color/format markup and cache width; `keep_existing=true`. | `NcBuffer` + cached width; zero-init; destroy buffer |
| `now_playing_prefix` | `$b` | `XX_BUFFER_WIDTH` | `now_playing_prefix`, `now_playing_prefix_length` | `settings_copy_nc_buffer` | Parse color/format markup and cache width; replace existing value. | `NcBuffer` + cached width; zero-init; destroy buffer |
| `now_playing_suffix` | `$/b` | `XX_BUFFER_WIDTH` | `now_playing_suffix`, `now_playing_suffix_length` | `settings_copy_nc_buffer` | Parse color/format markup and cache width; replace existing value. | `NcBuffer` + cached width; zero-init; destroy buffer |
| `browser_playlist_prefix` | `$2playlist$9 ` | `XX_BUFFER` | `browser_playlist_prefix` | `settings_copy_nc_buffer` | Parse color/format markup; replace existing value; no cached width. | `NcBuffer`; zero-init; destroy buffer |
| `selected_item_prefix` | `$6` | `XX_BUFFER_WIDTH` | `selected_item_prefix`, `selected_item_prefix_length` | `settings_copy_nc_buffer` | Parse color/format markup and cache width; replace existing value. | `NcBuffer` + cached width; zero-init; destroy buffer |
| `selected_item_suffix` | `$9` | `XX_BUFFER_WIDTH` | `selected_item_suffix`, `selected_item_suffix_length` | `settings_copy_nc_buffer` | Parse color/format markup and cache width; replace existing value. | `NcBuffer` + cached width; zero-init; destroy buffer |
| `modified_item_prefix` | `$3>$9 ` | `XX_BUFFER` | `modified_item_prefix` | `settings_copy_nc_buffer` | Parse color/format markup; replace existing value; no cached width. | `NcBuffer`; zero-init; destroy buffer |
| `song_window_title_format` | `{%a - }{%t}\|{%f}` | `XX_FORMAT` | `song_window_title_format` | `settings_parse_format` | Parse format expression with `NCM_FORMAT_FLAG_TAG`. | `NcmFormatAst`; zero-init; destroy AST |
| `browser_sort_mode` | `type` | `XX_ENUM` | `browser_sort_mode` | `ncm_sort_mode_parse` plus `noop` -> `none` alias | Parse sort mode; compatibility alias `noop` is rewritten to `none`. | trivial scalar |
| `browser_sort_format` | `{%a - }{%t}\|{%f} {%l}` | `XX_FORMAT` | `browser_sort_format` | `settings_parse_format` | Parse format expression with `NCM_FORMAT_FLAG_TAG`. | `NcmFormatAst`; zero-init; destroy AST |
| `song_columns_list_format` | `(20)[]{a} (6f)[green]{NE} (50)[white]{t\|f:Title} (20)[cyan]{b} (7f)[magenta]{l}` | `XX_COLUMNS` | `song_columns_list_format`, derived `song_columns_mode_format` | `apply_song_columns_list_format` custom body | Custom `(width)[color]{tags}` grammar; rebuilds `ColumnArray`, stretch metadata, and derived `song_columns_mode_format` AST. | `ColumnArray` + derived format AST; clear/free both |
| `execute_on_song_change` | `` | `XX_PATH` | `execute_on_song_change`, `execute_on_song_change_len` | `APPLY_STRING_PATH` | Expand leading `~`; otherwise copy bytes. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `execute_on_player_state_change` | `` | `XX_PATH` | `execute_on_player_state_change`, `execute_on_player_state_change_len` | `APPLY_STRING_PATH` | Expand leading `~`; otherwise copy bytes. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `playlist_show_mpd_host` | `no` | `XX_BOOL` | `playlist_show_mpd_host` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `playlist_show_remaining_time` | `no` | `XX_BOOL` | `playlist_show_remaining_time` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `playlist_shorten_total_times` | `no` | `XX_BOOL` | `playlist_shorten_total_times` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `playlist_separate_albums` | `no` | `XX_BOOL` | `playlist_separate_albums` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `playlist_display_mode` | `columns` | `XX_ENUM` | `playlist_display_mode` | `ncm_display_mode_parse` | Parse named value into enum-like storage. | trivial scalar |
| `browser_display_mode` | `classic` | `XX_ENUM` | `browser_display_mode` | `ncm_display_mode_parse` | Parse named value into enum-like storage. | trivial scalar |
| `search_engine_display_mode` | `classic` | `XX_ENUM` | `search_engine_display_mode` | `ncm_display_mode_parse` | Parse named value into enum-like storage. | trivial scalar |
| `playlist_editor_display_mode` | `classic` | `XX_ENUM` | `playlist_editor_display_mode` | `ncm_display_mode_parse` | Parse named value into enum-like storage. | trivial scalar |
| `discard_colors_if_item_is_selected` | `yes` | `XX_BOOL` | `discard_colors_if_item_is_selected` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `show_duplicate_tags` | `yes` | `XX_BOOL` | `show_duplicate_tags` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `incremental_seeking` | `yes` | `XX_BOOL` | `incremental_seeking` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `seek_time` | `1` | `XX_INT_RANGE` | `seek_time` | `APPLY_UINT` | Parse `int32`; no explicit bounds today. | trivial scalar |
| `volume_change_step` | `2` | `XX_INT_RANGE` | `volume_change_step` | `APPLY_UINT` | Parse `int32`; no explicit bounds today. | trivial scalar |
| `autocenter_mode` | `no` | `XX_BOOL` | `autocenter_mode` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `centered_cursor` | `no` | `XX_BOOL` | `centered_cursor` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `progressbar_look` | `=>` | `XX_LOOK` | `progressbar_look` | `apply_progressbar_look` custom body | Require 2..3 UTF-8 characters; if 2, append an extra NUL byte. | `StrBuilder`; zero-init; `sb_free` on destroy |
| `default_place_to_search_in` | `database` | `XX_ENUM` | `default_place_to_search_in` | manual database/playlist -> bool | Named two-state value stored as bool: `database` -> true, `playlist` -> false. | trivial scalar |
| `user_interface` | `classic` | `XX_ENUM` | `user_interface` | `ncm_design_parse` | Parse named value into enum-like storage. | trivial scalar |
| `data_fetching_delay` | `yes` | `XX_BOOL` | `data_fetching_delay` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `media_library_hide_album_dates` | `no` | `XX_BOOL` | `media_library_hide_album_dates` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `media_library_primary_tag` | `artist` | `XX_ENUM` | `media_library_primary_tag` | manual MPD tag mapping | Manual enum mapping: artist, album_artist, date, genre, composer, performer. | trivial scalar |
| `media_library_albums_split_by_date` | `yes` | `XX_BOOL` | `media_library_albums_split_by_date` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `default_find_mode` | `wrapped` | `XX_ENUM` | `default_find_mode` | manual wrapped/normal -> bool | Named two-state value stored as bool: `wrapped` -> true, `normal` -> false. | trivial scalar |
| `default_tag_editor_pattern` | `%n - %t` | `XX_STRING` | `default_tag_editor_pattern`, `default_tag_editor_pattern_len` | `APPLY_STRING` | Copy value bytes verbatim. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `header_visibility` | `yes` | `XX_BOOL` | `header_visibility` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `statusbar_visibility` | `yes` | `XX_BOOL` | `statusbar_visibility` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `connected_message_on_startup` | `yes` | `XX_BOOL` | `connected_message_on_startup` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `titles_visibility` | `yes` | `XX_BOOL` | `titles_visibility` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `header_text_scrolling` | `yes` | `XX_BOOL` | `header_text_scrolling` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `cyclic_scrolling` | `no` | `XX_BOOL` | `cyclic_scrolling` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `lyrics_fetchers` | `azlyrics, genius, letras, musixmatch, tekstowo, vagalume, internet` | `XX_LYRICS_FETCHERS` | `lyrics_fetchers` | `apply_lyrics_fetchers` custom body | Comma-separated registered fetcher names; unknown names fail; at least one item required. | `NcmLyricsFetcherRegistry`; zero-init; registry destroy |
| `follow_now_playing_lyrics` | `no` | `XX_BOOL` | `follow_now_playing_lyrics` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `fetch_lyrics_for_current_song_in_background` | `no` | `XX_BOOL` | `fetch_lyrics_for_current_song_in_background` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `store_lyrics_in_song_dir` | `no` | `XX_BOOL` | `store_lyrics_in_song_dir` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `generate_win32_compatible_filenames` | `yes` | `XX_BOOL` | `generate_win32_compatible_filenames` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `allow_for_physical_item_deletion` | `no` | `XX_BOOL` | `allow_for_physical_item_deletion` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `lastfm_preferred_language` | `en` | `XX_STRING` | `lastfm_preferred_language`, `lastfm_preferred_language_len` | `APPLY_STRING` | Copy value bytes verbatim. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `space_add_mode` | `add_remove` | `XX_ENUM` | `space_add_mode` | `ncm_space_add_mode_parse` | Parse named value into enum-like storage. | trivial scalar |
| `show_hidden_files_in_local_browser` | `no` | `XX_BOOL` | `show_hidden_files_in_local_browser` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `screen_switcher_mode` | `playlist, browser` | `XX_SCREEN_LIST` | `screen_switcher_mode`, `screen_switcher_previous` | `apply_screen_switcher_mode` custom body | Special `previous` sets a companion bool; otherwise parse a non-empty comma-separated list of startup-valid screens. | `ScreenTypeArray` + companion bool; free array storage |
| `startup_screen` | `playlist` | `XX_ENUM` | `startup_screen` | `screen_type_parse_startup` | Parse only screens allowed at startup. | trivial scalar |
| `startup_slave_screen` | `` | `XX_OPTIONAL_ENUM` | `startup_slave_screen`, `has_startup_slave_screen_type` | `apply_startup_slave_screen` custom body | Empty means unset; otherwise parse startup-valid screen and set presence flag. | enum + presence bool; trivial scalars |
| `startup_slave_screen_focus` | `no` | `XX_BOOL` | `startup_slave_screen_focus` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `locked_screen_width_part` | `50` | `XX_DOUBLE_RANGE` | `locked_screen_width_part` | `ncm_parse_double` + option-specific checks | Parse unbounded `double`, then divide by 100 before storage. | trivial scalar |
| `ask_for_locked_screen_width_part` | `yes` | `XX_BOOL` | `ask_for_locked_screen_width_part` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `media_library_column_width_ratio_two` | `1:1` | `XX_RATIO` | `media_library_column_width_ratio_two` | `settings_parse_ratio` | Parse exactly 2 colon-separated ints; sum must be nonzero. | `NcmInt32Array`; zero-init; array destroy |
| `media_library_column_width_ratio_three` | `1:1:1` | `XX_RATIO` | `media_library_column_width_ratio_three` | `settings_parse_ratio` | Parse exactly 3 colon-separated ints; sum must be nonzero. | `NcmInt32Array`; zero-init; array destroy |
| `playlist_editor_column_width_ratio` | `1:2` | `XX_RATIO` | `playlist_editor_column_width_ratio` | `settings_parse_ratio` | Parse exactly 2 colon-separated ints; sum must be nonzero. | `NcmInt32Array`; zero-init; array destroy |
| `jump_to_now_playing_song_at_start` | `yes` | `XX_BOOL` | `jump_to_now_playing_song_at_start` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `ask_before_clearing_playlists` | `yes` | `XX_BOOL` | `ask_before_clearing_playlists` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `ask_before_shuffling_playlists` | `yes` | `XX_BOOL` | `ask_before_shuffling_playlists` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `display_volume_level` | `yes` | `XX_BOOL` | `display_volume_level` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `display_bitrate` | `no` | `XX_BOOL` | `display_bitrate` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `display_remaining_time` | `no` | `XX_BOOL` | `display_remaining_time` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `regular_expressions` | `extended` | `XX_ENUM` | `regular_expressions` | manual regex-flag mapping | Manual mapping: none/basic/extended to case-insensitive regex flags. | trivial scalar |
| `ignore_leading_the` | `no` | `XX_BOOL` | `ignore_leading_the` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `block_search_constraints_change_if_items_found` | `yes` | `XX_BOOL` | `block_search_constraints_change_if_items_found` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `mouse_support` | `yes` | `XX_BOOL` | `mouse_support` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `mouse_list_scroll_whole_page` | `no` | `XX_BOOL` | `mouse_list_scroll_whole_page` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `lines_scrolled` | `5` | `XX_INT_RANGE` | `lines_scrolled` | `APPLY_UINT` | Parse `int32`; no explicit bounds today. | trivial scalar |
| `empty_tag_marker` | `<empty>` | `XX_STRING` | `empty_tag_marker`, `empty_tag_marker_len` | `APPLY_STRING` | Copy value bytes verbatim. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `empty_tag_color` | `cyan` | `XX_FORMATTED_COLOR` | `empty_tag_color` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `tags_separator` | ` \| ` | `XX_STRING` | `tags_separator`, `tags_separator_len` | `APPLY_STRING` | Copy value bytes verbatim. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `tag_editor_extended_numeration` | `no` | `XX_BOOL` | `tag_editor_extended_numeration` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `media_library_sort_by_mtime` | `no` | `XX_BOOL` | `media_library_sort_by_mtime` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `enable_window_title` | `yes` | `XX_BOOL` | `enable_window_title` | `apply_enable_window_title` custom body | If TERM is missing, contains `linux`, or begins `eterm`, force false and optionally warn; otherwise parse `yes`/`no`. | trivial scalar |
| `search_engine_default_search_mode` | `1` | `XX_INT_RANGE` | `search_engine_default_search_mode` | `ncm_parse_int32` + option-specific checks | Parse `int32`; require 1..3, then store value - 1. | trivial scalar |
| `external_editor` | `nano` | `XX_PATH` | `external_editor`, `external_editor_len` | `APPLY_STRING_PATH` | Expand leading `~`; otherwise copy bytes. | owned string + `_len`; init NULL/0; `free2` on destroy |
| `use_console_editor` | `yes` | `XX_BOOL` | `use_console_editor` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `colors_enabled` | `yes` | `XX_BOOL` | `colors_enabled` | `APPLY_BOOL` | Parse `yes`/`no`. | trivial scalar |
| `header_window_color` | `default` | `XX_COLOR` | `header_window_color` | `settings_parse_color` | Parse foreground/background color syntax into `NcColor`. | trivial `NcColor` |
| `volume_color` | `default` | `XX_FORMATTED_COLOR` | `volume_color` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `state_line_color` | `default` | `XX_FORMATTED_COLOR` | `state_line_color` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `state_flags_color` | `default:b` | `XX_FORMATTED_COLOR` | `state_flags_color` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `main_window_color` | `yellow` | `XX_COLOR` | `main_window_color` | `settings_parse_color` | Parse foreground/background color syntax into `NcColor`. | trivial `NcColor` |
| `color1` | `white` | `XX_FORMATTED_COLOR` | `color1` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `color2` | `green` | `XX_FORMATTED_COLOR` | `color2` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `progressbar_color` | `black:b` | `XX_FORMATTED_COLOR` | `progressbar_color` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `progressbar_elapsed_color` | `green:b` | `XX_FORMATTED_COLOR` | `progressbar_elapsed_color` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `statusbar_color` | `default` | `XX_COLOR` | `statusbar_color` | `settings_parse_color` | Parse foreground/background color syntax into `NcColor`. | trivial `NcColor` |
| `statusbar_time_color` | `default:b` | `XX_FORMATTED_COLOR` | `statusbar_time_color` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `player_state_color` | `default:b` | `XX_FORMATTED_COLOR` | `player_state_color` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `alternative_ui_separator_color` | `black:b` | `XX_FORMATTED_COLOR` | `alternative_ui_separator_color` | `settings_parse_formatted_color` | Parse color plus formatting attributes. | `NcFormattedColor`; explicit init/destroy |
| `window_border_color` | `green` | `XX_BORDER` | `window_border_color` | `settings_parse_color` + `nc_border_make` | Parse color and wrap with `nc_border_make`. | trivial `NcBorder` |
| `active_window_border` | `red` | `XX_BORDER` | `active_window_border` | `settings_parse_color` + `nc_border_make` | Parse color and wrap with `nc_border_make`. | trivial `NcBorder` |

## Configuration fields that are not independent options

These fields should not become standalone entries merely because they are
physical members of `Configuration`; most are companions or derived state.

| Field(s) | Role |
| --- | --- |
| `visualizer_fifo_path`, `visualizer_fifo_path_len` | no option entry; currently never populated by the settings parser |
| all string `*_len` fields | ownership/length companions of string-like options |
| `song_columns_mode_format` | derived from `song_columns_list_format` |
| `screen_switcher_previous` | companion state of `screen_switcher_mode` |
| `has_startup_slave_screen_type` | presence flag of `startup_slave_screen` |
| the eight prefix/suffix `*_length` fields | cached widths derived from the corresponding buffer options |
| `lyrics_db` | no option entry; currently only initialized/reset |

## Cross-cutting behavior to preserve during refactoring

- Duplicate options are rejected across all configuration files in one read.
- Options not explicitly set are applied from their descriptor default after
  all files have been processed.
- Invalid explicit values fail unless `ignore_errors` is enabled.
- Invalid defaults are reported as initialization errors.
- `configuration_read()` begins by clearing the target `Configuration`.
- Option descriptors are immutable; duplicate-option state is local to each
  `configuration_read()` invocation and is shared across all files in that read.
- MPD host/port/password/timeout parsing currently mutates `global_mpd`.
- `enable_window_title` currently depends on the process `TERM` environment.
- `visualizer_spectrum_hz_max` currently validates against the already parsed
  value of `visualizer_spectrum_hz_min`.

## Step-1 regression coverage

`tests/c_settings_baseline_test.c` locks down the following behavior before
the implementation is reorganized:

- effective descriptor count and unique option names;
- successful parsing of every declared default in descriptor order;
- representative post-default values, including transformed values;
- numeric boundary acceptance/rejection for currently bounded settings;
- duplicate-option rejection;
- destruction/reset of representative owned objects, including a second
  destroy to catch non-idempotent cleanup.
