# Prompt

For the first and second of the problems below
(Breaking function calls before the first argument and
 Related argument pairs not grouped together),
identify instances of them in the first file in the list below it. Then fix those
instances and remove the file from the list. If the first file in the list does
not have the problems, try the next one without asking first.

# List for the first 2 problems:

## breaking function calls before the first argument and not aligning
For instance, this is bad:
```c
status = ncm_fs_rename(
    old_real_path.data, old_real_path.len,
    new_real_path.data, new_real_path.len, ncm_error)
```
Replace with:
```c
status = ncm_fs_rename(old_real_path.data, old_real_path.len,
                       new_real_path.data, new_real_path.len,
                       ncm_error);
```
Only keep line break before the first argument on the very very rare occasion
that it does not fit in a single line (80 column rule). Remember to try to keep
associated pointer + len pairs together.

## Related argument pairs not grouped together
Make sure that related argument pairs in function headers and calls are together
(on the same line if they fit). For instance, width with height, x with y, array
with its length, callback function with its `void *user`, etc.
If both args do not fit in the same line, then break the
line *before* the first one, so that they at least do not mix with other
arguments in the call and are aligned.
Example
```c
// bad (note how width and height are unaligned)
int x = function_with_long_name_must_be_broken(long_argument, window_width,
                                               window_height);
// bad (avoid breaking before the first argument, even if that would make width
//      and height be together).
int x = function_with_long_name_must_be_broken(
    long_argument, window_width, window_height);

// good (don't break if all arguments fit)
int x = function_with_short_name(long_argument, window_width, window_height);

// good
int x = function_with_long_name_must_be_broken(long_argument,
                                               window_width, window_height);

// good
int x = function_with_long_name_must_be_broken_twice(long_argument,
                                                     window_width,
                                                     window_height);
```

## Utility function creep
Functions that do the (almost) the same thing are redefined in different places.

## unnecessary static function declarations at the top of the files
Reorder them and delete the standalone declaration on the top of the .c file.

## functions definitions could be reordered to not need declarations at the top

## Style: lines broken prematurely

## Style: checking return value after the call (call should be inside if().

## Strings unecessary conversion to and from StrBuilder
Investigate:
- ncm_conversion_copy_source()
- there are multiple confusing string types

## Excessive NULL checking and error checking
The source code in src/ is at paranoia levels of NULL pointer checking.  For
instance, lots of functions check if (screen == NULL), but they are called
internally, *after* the external API of its module has already validated input.
Another bad pattern comes in the form of errors. Functions that should never
return negative because the input was already validated are always checked,
which adds lots of unnecessary `if (status < 0)` in the code. As explained in
detail in cbase/c-guidelines.md, only the external API of each module does input
validation. If a utility function like `sb_set` might return negative because we
passed NULL pointers to it or pass zero or negative lengths, does not mean that
we need to check it, because the pointers and lengths should have been validated
at a higher level in the code. Also, some functions are made to return int32
only to conform because some lower level function that they call returns int32.
But again, this is only needed if an error is expected. Most internal functions
should never fail. Make them return void in that case (or the result if they
need to return some data). Also, some functions need to checked for errors in
*some* calls, while not in others. You need to know which calls have to be
checked by the context. It is okay to assume stuff sometimes (add assertions if
you are not 100% sure). Never cast functions calls to void.

Identify instances of the anti-patterns above (excessive NULL pointer checking
and excessive error checking).  Remove all the instances of the anti-pattern.
You can leave an ASSERT(pointer != NULL) or in 20% of the cases, just as an
occasional sanity check.  But most internal functions can simply assume that
they have valid input because the external API of the module has already
validated it. If you don't find any instance of the anti patterns in the first
file, remove it from the list below and search in the next without asking me.
If the next file also does not have the anti pattern, also remove it from the
list and try the next and so on. When you end up changing a .c file, send me the
result as a tar ball (but include all the changes of the previous prompts as
well).

- src/curses/nc_scrollpad.c
- src/statusbar.c
- src/app_legacy_bridge.c
- src/c/ncm_display.c
- src/c/ncm_tags.c
- src/c/ncm_fs.c
- src/c/ncm_playlist_sort.c
- src/screens/nc_help.c
- src/c/ncm_taglib.c
- src/screens/nc_server_info.c
- src/c/ncm_mpd_item.c
- src/screens/nc_song_info.c
- src/c/ncm_app_arrays.c
- src/c/ncm_html.c
- src/c/ncm_conversion.c
- src/c/ncm_regex.c
- src/c/ncm_string.c
- src/c/ncm_enums.c
- src/title.c
- src/app_controller.c
- src/helpers.c
- src/c/ncm_option_parser.c
- src/curl_handle.c
- src/c/ncm_sample_buffer.c
- src/c/ncm_path.c
- src/c/ncm_directory.c
- src/c/ncm_playlist.c
- src/screens/screen_type.c
- src/c/ncm_macro_utilities.c
- src/c/ncm_search_prompt.c
- src/ui_state.c
- src/curses/nc_formatted_color.c
- src/global.c
- src/screens/nc_screens.c
- src/curses/nc_curses.c

## Functions that are never called (dead code)
