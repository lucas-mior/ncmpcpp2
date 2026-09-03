# Prompt

For the first of the problems below (functions that are called only once),
identify instances of it in the first file in the list below it. Then fix those
instances and remove the file from the list.

## Functions that are called only once
For functions that are defined inside a file and only used inside the same file,
if they are called only once: inline their body and then delete them. If there
aren't any functions that fit this pattern, simply delete the file from the
list below, and go straight to the next file without asking me. If the next file
also doesn't have any function that fit this pattern, continue deleting the file
form the list below and searching on the next until you find.

- src/screens/nc_tag_editor.c
- src/screens/nc_server_info.c
- src/screens/nc_playlist_editor.c
- src/screens/nc_help.c
- src/screens/nc_song_info.c
- src/screens/app_screens.c
- src/screens/nc_search_engine.c
- src/screens/nc_browser.c
- src/screens/nc_lastfm.c
- src/screens/nc_scrollpad_screen.c
- src/screens/nc_sort_playlist.c
- src/screens/nc_screen.c
- src/screens/nc_tiny_tag_editor.c
- src/screens/nc_playlist.c
- src/screens/nc_screens.c
- src/screens/nc_visualizer.c
- src/screens/nc_sel_items_adder.c
- src/screens/nc_outputs.c
- src/screens/nc_media_library.c
- src/screens/nc_screen_switcher.c
- src/screens/screen_type.c
- src/screens/nc_lyrics.c
- src/configuration.c
- src/c/ncm_taglib.c
- src/curses/nc_app_menus.c
- src/c/ncm_song.c
- src/helpers.c
- src/curses/nc_menu.c
- src/app_state.c
- src/c/ncm_path.c
- src/c/ncm_enums.c
- src/ui_state.c
- src/app_controller.c
- src/c/ncm_mutable_song.c
- src/title.c
- src/c/ncm_conversion.c
- src/c/ncm_mpd_connection.c
- src/c/ncm_c.c
- src/c/ncm_string.c
- src/settings.c
- src/main.c
- src/lastfm_service.c
- src/statusbar.c
- src/c/ncm_job.c
- src/c/ncm_app_arrays.c
- src/curses/nc_formatted_color.c
- src/app_legacy_bridge.c
- src/c/ncm_search_prompt.c
- src/screen_actions.c
- src/lyrics_fetcher.c
- src/c/ncm_playlist_sort.c
- src/status.c
- src/c/ncm_comparators.c
- src/c/ncm_option_parser.c
- src/c/ncm_type_conversions.c
- src/c/ncm_lrc.c
- src/c/ncm_mpd_item.c
- src/c/ncm_html.c
- src/settings_types.c
- src/c/ncm_mpd_client.c
- src/curses/nc_curses.c
- src/c/ncm_directory.c
- src/c/ncm_macro_utilities.c
- src/curl_handle.c
- src/c/ncm_sample_buffer.c
- src/c/ncm_playlist.c
- src/c/ncm_format.c
- src/curses/nc_window.c
- src/c/ncm_error.c
- src/curses/nc_buffer.c
- src/global.c
- src/bindings.c
- src/curses/nc_scrollpad.c
- src/c/ncm_fs.c
- src/c/ncm_display.c
- src/c/ncm_regex.c
- src/c/ncm_tags.c
- src/curses/nc_cyclic_buffer.c

## Excessive NULL checking
The source code in src/ is at paranoia levels of NULL pointer checking.  For
instance, lots of functions check if (screen == NULL), but they are called
internally, *after* the external API of its module has already validated input.
Identify instances of the anti-pattern of looking for NULL in internal
functions.  Remove all the instances of this anti-pattern. You can leave an
ASSERT(pointer != NULL) in 20% of the cases, just as an occasional sanity check.
But most internal functions can simply assume that they have valid input because
the external API of the module has already validated it.

- src/c/ncm_conversion.c
- src/screens/nc_help.c
- src/c/ncm_tags.c
- src/c/ncm_type_conversions.c
- src/c/ncm_sample_buffer.c
- src/screen_actions.c
- src/curl_handle.c
- src/curses/nc_buffer.c
- src/screens/app_screens.c
- src/c/ncm_lrc.c
- src/screens/nc_server_info.c
- src/screens/nc_outputs.c
- src/curses/nc_window.c
- src/c/ncm_taglib.c
- src/screens/nc_playlist_editor.c
- src/global.c
- src/lastfm_service.c
- src/screens/nc_screen.c
- src/c/ncm_song.c
- src/screens/nc_browser.c
- src/helpers.c
- src/c/ncm_mpd_connection.c
- src/curses/nc_cyclic_buffer.c
- src/app_controller.c
- src/screens/nc_search_engine.c
- src/screens/nc_tiny_tag_editor.c
- src/curses/nc_scrollpad.c
- src/c/ncm_playlist.c
- src/c/ncm_fs.c
- src/c/ncm_search_prompt.c
- src/settings.c
- src/ui_state.c
- src/app_state.c
- src/c/ncm_job.c
- src/bindings.c
- src/screens/nc_sel_items_adder.c
- src/c/ncm_directory.c
- src/c/ncm_comparators.c
- src/c/ncm_regex.c
- src/curses/nc_menu.c
- src/screens/nc_screen_switcher.c
- src/statusbar.c
- src/c/ncm_string.c
- src/curses/nc_formatted_color.c
- src/c/ncm_path.c
- src/c/ncm_mpd_item.c
- src/screens/screen_type.c
- src/screens/nc_screens.c
- src/c/ncm_playlist_sort.c
- src/app_legacy_bridge.c
- src/screens/nc_sort_playlist.c
- src/settings_types.c
- src/screens/nc_tag_editor.c
- src/screens/nc_playlist.c
- src/screens/nc_lyrics.c
- src/screens/nc_lastfm.c
- src/c/ncm_html.c
- src/screens/nc_song_info.c
- src/c/ncm_display.c
- src/screens/nc_scrollpad_screen.c
- src/c/ncm_macro_utilities.c
- src/c/ncm_enums.c
- src/status.c
- src/configuration.c
- src/c/ncm_app_arrays.c
- src/main.c
- src/title.c
- src/lyrics_fetcher.c
- src/c/ncm_mpd_client.c
- src/c/ncm_c.c
- src/c/ncm_option_parser.c
- src/screens/nc_visualizer.c
- src/curses/nc_curses.c
- src/curses/nc_app_menus.c
- src/actions.c
- src/screens/nc_media_library.c
- src/c/ncm_mutable_song.c
- src/c/ncm_format.c
- src/c/ncm_error.c

## Functions that are never called (dead code)
## Excessive error checking
## Utility function creep
Functions that do the same thing are redefined in different

## some unnecessary static function declarations at the top of the files
## functions definitions could be reordered to not need declarations at the top

## Style: lines broken prematurely

## Style: checking return value after the call (call should be inside if().

## Style: breaking function calls before the first argument and not alining
This is bad:
```c
status = ncm_fs_rename(
    old_real_path.data, old_real_path.len,
    new_real_path.data, new_real_path.len, ncm_error);
```
Replace with:
```c
status = ncm_fs_rename(old_real_path.data, old_real_path.len,
                       new_real_path.data, new_real_path.len,
                       ncm_error);
```

## Strings unecessary conversion to and from StrBuilder
Investigate:
- ncm_conversion_copy_source()
- there are multiple confusing string types
