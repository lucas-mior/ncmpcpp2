#include <cstdlib>
#include <cstring>
#include <iostream>
#include <regex>
#include <string>

#include "c/ncm_base.h"
#include "settings.h"
#include "settings_legacy.h"
#include "settings_legacy_runtime.h"
#include "song.h"

#undef Config

namespace {

void check(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << "FAIL: " << message << "\n";
		std::exit(EXIT_FAILURE);
	}
}

void setString(char **data, int32 *length, int32 *capacity,
               const char *value)
{
	int32 value_length = static_cast<int32>(std::strlen(value));
	if (*data != nullptr)
		ncm_free(*data, *capacity);
	*capacity = value_length + 1;
	*data = static_cast<char *>(ncm_malloc(*capacity));
	std::memcpy(*data, value, static_cast<size_t>(value_length + 1));
	*length = value_length;
}

void setFormat(NcmFormatAst *format, const char *value)
{
	NcmError error;
	ncm_error_clear(&error);
	check(ncm_format_parse(format, const_cast<char *>(value),
	                       static_cast<int32>(std::strlen(value)),
	                       NCM_FORMAT_FLAG_ALL, &error),
	      "format fixture parsing");
}

void setBuffer(NcBuffer *buffer, const char *value, NcColor color)
{
	nc_buffer_clear(buffer);
	nc_buffer_append_cstring(buffer, const_cast<char *>(value));
	nc_buffer_add_color(buffer, 0, color, 17);
	nc_buffer_add_format(buffer, 1, NC_FORMAT_BOLD, 23);
}

void setFormattedColor(NcFormattedColor *color, NcColor base,
                       enum NcFormat format)
{
	nc_formatted_color_destroy(color);
	nc_formatted_color_init_color(color, base);
	nc_formatted_color_add_format(color, format);
}

void appendRatio(NcmInt32Array *array, int32 value)
{
	int32 *item = ncm_int32_array_append(array);
	check(item != nullptr, "ratio append");
	*item = value;
}

void populateSource(Configuration *source)
{
	setString(&source->ncmpcpp_directory, &source->ncmpcpp_directory_len,
	          &source->ncmpcpp_directory_cap, "ncmpcpp_directory-value");
	setString(&source->mpd_music_dir, &source->mpd_music_dir_len,
	          &source->mpd_music_dir_cap, "mpd_music_dir-value");
	setString(&source->empty_tag, &source->empty_tag_len,
	          &source->empty_tag_cap, "empty_tag-value");
	setString(&source->lastfm_preferred_language, &source->lastfm_preferred_language_len,
	          &source->lastfm_preferred_language_cap, "lastfm_preferred_language-value");
	setString(&source->pattern, &source->pattern_len,
	          &source->pattern_cap, "pattern-value");
	setString(&source->random_exclude_pattern, &source->random_exclude_pattern_len,
	          &source->random_exclude_pattern_cap, "random_exclude_pattern-value");
	setString(&source->tags_separator, &source->tags_separator_len,
	          &source->tags_separator_cap, "tags_separator-value");


	setFormat(&source->song_list_format, "format-0");
	setFormat(&source->song_columns_mode_format, "format-3");
	setFormat(&source->browser_sort_format, "format-4");

	appendRatio(&source->playlist_editor_column_width_ratio, 3);
	appendRatio(&source->playlist_editor_column_width_ratio, 5);

	Column *column = column_array_append(&source->columns);
	check(column != nullptr, "column append");
	setString(&column->name, &column->name_len, &column->name_cap,
	          "column-name");
	setString(&column->type, &column->type_len, &column->type_cap,
	          "at");
	column->width = 31;
	column->stretch_limit = 37;
	column->color = nc_color_make(COLOR_CYAN, COLOR_BLUE, false, false);
	column->fixed = true;
	column->right_alignment = true;
	column->display_empty_tag = false;

	source->playlist_display_mode = NCM_DISPLAY_MODE_COLUMNS;
	source->browser_display_mode = NCM_DISPLAY_MODE_COLUMNS;
	source->playlist_editor_display_mode = NCM_DISPLAY_MODE_COLUMNS;
	source->design = NCM_DESIGN_ALTERNATIVE;
	source->space_add_mode = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
	source->media_lib_primary_tag = MPD_TAG_ALBUM;
	source->browser_sort_mode = NCM_SORT_MODE_NAME;

	setBuffer(&source->browser_playlist_prefix, "buffer-0",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));
	setBuffer(&source->selected_item_prefix, "buffer-1",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));
	setBuffer(&source->selected_item_suffix, "buffer-2",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));
	setBuffer(&source->now_playing_prefix, "buffer-3",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));
	setBuffer(&source->now_playing_suffix, "buffer-4",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));
	setBuffer(&source->modified_item_prefix, "buffer-5",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));
	setBuffer(&source->current_item_prefix, "buffer-6",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));
	setBuffer(&source->current_item_suffix, "buffer-7",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));
	setBuffer(&source->current_item_inactive_column_prefix, "buffer-8",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));
	setBuffer(&source->current_item_inactive_column_suffix, "buffer-9",
	          nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false));

	source->main_color = nc_color_make(COLOR_GREEN, COLOR_BLUE, false, false);

	setFormattedColor(&source->color2,
	                  nc_color_make(COLOR_GREEN, COLOR_BLACK, false, false),
	                  NC_FORMAT_UNDERLINE);
	setFormattedColor(&source->empty_tags_color,
	                  nc_color_make(COLOR_YELLOW, COLOR_BLACK, false, false),
	                  NC_FORMAT_BOLD);
	setFormattedColor(&source->volume_color,
	                  nc_color_make(COLOR_BLUE, COLOR_BLACK, false, false),
	                  NC_FORMAT_UNDERLINE);
	setFormattedColor(&source->alternative_ui_separator_color,
	                  nc_color_make(COLOR_BLUE, COLOR_BLACK, false, false),
	                  NC_FORMAT_BOLD);

	source->window_border = nc_border_make(
		nc_color_make(COLOR_BLUE, COLOR_BLACK, false, false));
	source->active_window_border = nc_border_make(
		nc_color_make(COLOR_RED, COLOR_BLACK, false, false));

	source->playlist_separate_albums = true;
	source->set_window_title = true;
	source->header_visibility = true;
	source->header_text_scrolling = true;
	source->statusbar_visibility = true;
	source->titles_visibility = true;
	source->centered_cursor = true;
	source->screen_switcher_previous = true;
	source->wrapped_search = true;
	source->now_playing_lyrics = true;
	source->fetch_lyrics_in_background = true;
	source->local_browser_show_hidden_files = true;
	source->display_bitrate = true;
	source->ignore_leading_the = true;
	source->use_cyclic_scrolling = true;
	source->ask_before_clearing_playlists = true;
	source->mouse_support = true;
	source->mouse_list_scroll_whole_page = true;
	source->data_fetching_delay = true;
	source->tag_editor_extended_numeration = true;
	source->discard_colors_if_item_is_selected = true;
	source->generate_win32_compatible_filenames = true;
	source->ask_for_locked_screen_width_part = true;
	source->allow_for_physical_item_deletion = true;
	source->show_duplicate_tags = false;

	source->crossfade_time = 42;
	source->volume_change_step = 44;
	source->lines_scrolled = 47;
	source->locked_screen_width_part = 4.25;

	source->selected_item_prefix_length = 2;
	source->selected_item_suffix_length = 3;
	source->now_playing_prefix_length = 4;
	source->now_playing_suffix_length = 5;
	source->current_item_prefix_length = 6;
	source->current_item_suffix_length = 7;
	source->current_item_inactive_column_prefix_length = 8;
	source->current_item_inactive_column_suffix_length = 9;
	source->regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;

	ScreenType *screen = screen_type_array_append(&source->screen_sequence);
	check(screen != nullptr, "screen append");
	*screen = NCM_SCREEN_TYPE_PLAYLIST;
	screen = screen_type_array_append(&source->screen_sequence);
	check(screen != nullptr, "screen append");
	*screen = NCM_SCREEN_TYPE_BROWSER;

	check(ncm_lyrics_fetcher_registry_append_name(
	          &source->lyrics_fetchers,
	          const_cast<char *>("internet"), 8),
	      "lyrics fetcher append");
}

bool formatEqual(const NcmFormatExprList &left,
                 const NcmFormatExprList &right)
{
	if (left.len != right.len)
		return false;
	for (int32 i = 0; i < left.len; i += 1)
	{
		const NcmFormatExpr &a = left.items[i];
		const NcmFormatExpr &b = right.items[i];
		if (a.type != b.type)
			return false;
		switch (a.type)
		{
			case NCM_FORMAT_EXPR_TEXT:
				if (a.value.text.len != b.value.text.len
				    || std::memcmp(a.value.text.data, b.value.text.data,
				                   static_cast<size_t>(a.value.text.len)) != 0)
					return false;
				break;
			case NCM_FORMAT_EXPR_COLOR:
				if (!nc_color_equal(a.value.color, b.value.color))
					return false;
				break;
			case NCM_FORMAT_EXPR_FORMAT:
				if (a.value.format != b.value.format)
					return false;
				break;
			case NCM_FORMAT_EXPR_SONG_TAG:
				if (a.value.song_tag.getter != b.value.song_tag.getter
				    || a.value.song_tag.delimiter !=
				           b.value.song_tag.delimiter)
					return false;
				break;
			case NCM_FORMAT_EXPR_GROUP:
			case NCM_FORMAT_EXPR_FIRST_OF:
				if (!formatEqual(a.value.list, b.value.list))
					return false;
				break;
			case NCM_FORMAT_EXPR_OUTPUT_SWITCH:
				break;
		}
	}
	return true;
}

bool bufferEqual(const NC::Buffer &legacy, const NcBuffer &source)
{
	return nc_buffer_equal(
		const_cast<NcBuffer *>(legacy.cBuffer()),
		const_cast<NcBuffer *>(&source));
}

bool formattedColorEqual(const NC::FormattedColor &legacy,
                         const NcFormattedColor &source)
{
	return nc_formatted_color_equal(
		const_cast<NcFormattedColor *>(legacy.cFormattedColor()),
		const_cast<NcFormattedColor *>(&source));
}

bool regexFlagsEqual(const Regex::Flags &legacy, uint32 source)
{
	if (source == NCM_REGEX_LITERAL_CASE_INSENSITIVE)
		return legacy.literal()
		       && legacy.syntax() ==
			       (std::regex_constants::ECMAScript
			        |std::regex_constants::icase);
	if (source == NCM_REGEX_BASIC_CASE_INSENSITIVE)
		return !legacy.literal()
		       && legacy.syntax() ==
			       (std::regex_constants::basic
			        |std::regex_constants::icase);
	return !legacy.literal()
	       && legacy.syntax() ==
		       (std::regex_constants::extended
		        |std::regex_constants::icase);
}

void verifySync(const Configuration *source)
{
	check(ConfigLegacy.ncmpcpp_directory ==
	      std::string(source->ncmpcpp_directory, static_cast<size_t>(source->ncmpcpp_directory_len)),
	      "ncmpcpp_directory");
	check(ConfigLegacy.mpd_music_dir ==
	      std::string(source->mpd_music_dir, static_cast<size_t>(source->mpd_music_dir_len)),
	      "mpd_music_dir");
	check(ConfigLegacy.empty_tag ==
	      std::string(source->empty_tag, static_cast<size_t>(source->empty_tag_len)),
	      "empty_tag");
	check(ConfigLegacy.lastfm_preferred_language ==
	      std::string(source->lastfm_preferred_language, static_cast<size_t>(source->lastfm_preferred_language_len)),
	      "lastfm_preferred_language");
	check(ConfigLegacy.pattern ==
	      std::string(source->pattern, static_cast<size_t>(source->pattern_len)),
	      "pattern");
	check(ConfigLegacy.random_exclude_pattern ==
	      std::string(source->random_exclude_pattern, static_cast<size_t>(source->random_exclude_pattern_len)),
	      "random_exclude_pattern");

	check(formatEqual(ConfigLegacy.song_list_format.cAst()->root,
	                  source->song_list_format.root),
	      "song_list_format");
	check(formatEqual(ConfigLegacy.song_columns_mode_format.cAst()->root,
	                  source->song_columns_mode_format.root),
	      "song_columns_mode_format");
	check(formatEqual(ConfigLegacy.browser_sort_format.cAst()->root,
	                  source->browser_sort_format.root),
	      "browser_sort_format");

	check(ConfigLegacy.playlist_editor_column_width_ratio ==
	      std::vector<size_t>({3, 5}), "playlist editor ratio");
	check(ConfigLegacy.columns.size() == 1, "columns size");
	check(ConfigLegacy.columns[0].name == "column-name", "column name");
	check(ConfigLegacy.columns[0].type == "at", "column type");
	check(ConfigLegacy.columns[0].width == 31, "column width");
	check(ConfigLegacy.columns[0].stretch_limit == 37,
	      "column stretch limit");
	check(NC::toNcColor(ConfigLegacy.columns[0].color).foreground ==
	      source->columns.items[0].color.foreground, "column color");
	check(ConfigLegacy.columns[0].fixed, "column fixed");
	check(ConfigLegacy.columns[0].right_alignment,
	      "column right alignment");
	check(!ConfigLegacy.columns[0].display_empty_tag,
	      "column display empty tag");

	check(ConfigLegacy.playlist_display_mode == source->playlist_display_mode, "playlist_display_mode");
	check(ConfigLegacy.browser_display_mode == source->browser_display_mode, "browser_display_mode");
	check(ConfigLegacy.playlist_editor_display_mode == source->playlist_editor_display_mode, "playlist_editor_display_mode");
	check(ConfigLegacy.design == source->design, "design");
	check(ConfigLegacy.space_add_mode == source->space_add_mode, "space_add_mode");
	check(ConfigLegacy.media_lib_primary_tag == source->media_lib_primary_tag, "media_lib_primary_tag");
	check(ConfigLegacy.browser_sort_mode == source->browser_sort_mode, "browser_sort_mode");

	check(bufferEqual(ConfigLegacy.browser_playlist_prefix, source->browser_playlist_prefix),
	      "browser_playlist_prefix");
	check(bufferEqual(ConfigLegacy.selected_item_prefix, source->selected_item_prefix),
	      "selected_item_prefix");
	check(bufferEqual(ConfigLegacy.selected_item_suffix, source->selected_item_suffix),
	      "selected_item_suffix");
	check(bufferEqual(ConfigLegacy.now_playing_prefix, source->now_playing_prefix),
	      "now_playing_prefix");
	check(bufferEqual(ConfigLegacy.now_playing_suffix, source->now_playing_suffix),
	      "now_playing_suffix");
	check(bufferEqual(ConfigLegacy.modified_item_prefix, source->modified_item_prefix),
	      "modified_item_prefix");
	check(bufferEqual(ConfigLegacy.current_item_prefix, source->current_item_prefix),
	      "current_item_prefix");
	check(bufferEqual(ConfigLegacy.current_item_suffix, source->current_item_suffix),
	      "current_item_suffix");
	check(bufferEqual(ConfigLegacy.current_item_inactive_column_prefix, source->current_item_inactive_column_prefix),
	      "current_item_inactive_column_prefix");
	check(bufferEqual(ConfigLegacy.current_item_inactive_column_suffix, source->current_item_inactive_column_suffix),
	      "current_item_inactive_column_suffix");

	check(nc_color_equal(NC::toNcColor(ConfigLegacy.main_color),
	                     source->main_color), "main color");

	check(formattedColorEqual(ConfigLegacy.color2, source->color2),
	      "color2");
	check(formattedColorEqual(ConfigLegacy.empty_tags_color, source->empty_tags_color),
	      "empty_tags_color");
	check(formattedColorEqual(ConfigLegacy.volume_color, source->volume_color),
	      "volume_color");
	check(formattedColorEqual(ConfigLegacy.alternative_ui_separator_color, source->alternative_ui_separator_color),
	      "alternative_ui_separator_color");
	check(ConfigLegacy.window_border.has_value(), "window border");
	check(ConfigLegacy.active_window_border.has_value(),
	      "active window border");
	check(nc_color_equal(NC::toNcColor(*ConfigLegacy.window_border),
	                     source->window_border.color),
	      "window border color");
	check(nc_color_equal(NC::toNcColor(*ConfigLegacy.active_window_border),
	                     source->active_window_border.color),
	      "active window border color");

	check(ConfigLegacy.playlist_separate_albums == source->playlist_separate_albums, "playlist_separate_albums");
	check(ConfigLegacy.set_window_title == source->set_window_title, "set_window_title");
	check(ConfigLegacy.header_visibility == source->header_visibility, "header_visibility");
	check(ConfigLegacy.header_text_scrolling == source->header_text_scrolling, "header_text_scrolling");
	check(ConfigLegacy.statusbar_visibility == source->statusbar_visibility, "statusbar_visibility");
	check(ConfigLegacy.titles_visibility == source->titles_visibility, "titles_visibility");
	check(ConfigLegacy.centered_cursor == source->centered_cursor, "centered_cursor");
	check(ConfigLegacy.screen_switcher_previous == source->screen_switcher_previous, "screen_switcher_previous");
	check(ConfigLegacy.wrapped_search == source->wrapped_search, "wrapped_search");
	check(ConfigLegacy.now_playing_lyrics == source->now_playing_lyrics, "now_playing_lyrics");
	check(ConfigLegacy.fetch_lyrics_in_background == source->fetch_lyrics_in_background, "fetch_lyrics_in_background");
	check(ConfigLegacy.local_browser_show_hidden_files == source->local_browser_show_hidden_files, "local_browser_show_hidden_files");
	check(ConfigLegacy.display_bitrate == source->display_bitrate, "display_bitrate");
	check(ConfigLegacy.ignore_leading_the == source->ignore_leading_the, "ignore_leading_the");
	check(ConfigLegacy.use_cyclic_scrolling == source->use_cyclic_scrolling, "use_cyclic_scrolling");
	check(ConfigLegacy.ask_before_clearing_playlists == source->ask_before_clearing_playlists, "ask_before_clearing_playlists");
	check(ConfigLegacy.mouse_support == source->mouse_support, "mouse_support");
	check(ConfigLegacy.mouse_list_scroll_whole_page == source->mouse_list_scroll_whole_page, "mouse_list_scroll_whole_page");
	check(ConfigLegacy.data_fetching_delay == source->data_fetching_delay, "data_fetching_delay");
	check(ConfigLegacy.tag_editor_extended_numeration == source->tag_editor_extended_numeration, "tag_editor_extended_numeration");
	check(ConfigLegacy.discard_colors_if_item_is_selected == source->discard_colors_if_item_is_selected, "discard_colors_if_item_is_selected");
	check(ConfigLegacy.generate_win32_compatible_filenames == source->generate_win32_compatible_filenames, "generate_win32_compatible_filenames");
	check(ConfigLegacy.ask_for_locked_screen_width_part == source->ask_for_locked_screen_width_part, "ask_for_locked_screen_width_part");
	check(ConfigLegacy.allow_for_physical_item_deletion == source->allow_for_physical_item_deletion, "allow_for_physical_item_deletion");

	check(ConfigLegacy.crossfade_time == source->crossfade_time, "crossfade_time");
	check(ConfigLegacy.volume_change_step == source->volume_change_step, "volume_change_step");
	check(ConfigLegacy.lines_scrolled == source->lines_scrolled, "lines_scrolled");

	check(ConfigLegacy.locked_screen_width_part == source->locked_screen_width_part, "locked_screen_width_part");

	check(ConfigLegacy.selected_item_prefix_length == static_cast<size_t>(source->selected_item_prefix_length), "selected_item_prefix_length");
	check(ConfigLegacy.selected_item_suffix_length == static_cast<size_t>(source->selected_item_suffix_length), "selected_item_suffix_length");
	check(ConfigLegacy.now_playing_prefix_length == static_cast<size_t>(source->now_playing_prefix_length), "now_playing_prefix_length");
	check(ConfigLegacy.now_playing_suffix_length == static_cast<size_t>(source->now_playing_suffix_length), "now_playing_suffix_length");
	check(ConfigLegacy.current_item_prefix_length == static_cast<size_t>(source->current_item_prefix_length), "current_item_prefix_length");
	check(ConfigLegacy.current_item_suffix_length == static_cast<size_t>(source->current_item_suffix_length), "current_item_suffix_length");
	check(ConfigLegacy.current_item_inactive_column_prefix_length == static_cast<size_t>(source->current_item_inactive_column_prefix_length), "current_item_inactive_column_prefix_length");
	check(ConfigLegacy.current_item_inactive_column_suffix_length == static_cast<size_t>(source->current_item_inactive_column_suffix_length), "current_item_inactive_column_suffix_length");
	check(regexFlagsEqual(ConfigLegacy.regex_type, source->regex_type),
	      "regex type");
	check(ConfigLegacy.screen_sequence ==
	      std::vector<ScreenType>({NCM_SCREEN_TYPE_PLAYLIST,
	                               NCM_SCREEN_TYPE_BROWSER}),
	      "screen sequence");
	check(ConfigLegacy.lyrics_fetchers.fetchers.len == 1,
	      "lyrics fetcher count");
	check(std::string(
	          ConfigLegacy.lyrics_fetchers.fetchers.items[0].name,
	          static_cast<size_t>(
	              ConfigLegacy.lyrics_fetchers.fetchers.items[0].name_len))
	      == "the Internet", "lyrics fetcher deep copy");
	check(MPD::Song::TagsSeparator == "tags_separator-value",
	      "tags separator");
	check(!MPD::Song::ShowDuplicateTags, "show duplicate tags");
}

void verifyGlobalSyncWrapper()
{
	configuration_init(&::Config);
	setString(&::Config.ncmpcpp_directory,
	          &::Config.ncmpcpp_directory_len,
	          &::Config.ncmpcpp_directory_cap,
	          "global-ncmpcpp-directory");
	check(settings_legacy_runtime_sync_configuration(),
	      "global compatibility sync");
	check(ConfigLegacy.ncmpcpp_directory == "global-ncmpcpp-directory",
	      "global compatibility source");
	configuration_destroy(&::Config);
}

void verifyDeepCopies(Configuration *source)
{
	source->ncmpcpp_directory[0] = 'X';
	ncm_format_ast_clear(&source->song_list_format);
	nc_buffer_clear(&source->browser_playlist_prefix);
	source->playlist_editor_column_width_ratio.items[0] = 99;
	source->columns.items[0].name[0] = 'X';
	source->screen_sequence.items[0] = NCM_SCREEN_TYPE_HELP;
	ncm_lyrics_fetcher_registry_clear(&source->lyrics_fetchers);

	check(ConfigLegacy.ncmpcpp_directory == "ncmpcpp_directory-value",
	      "string copy independence");
	check(ConfigLegacy.song_list_format.cAst()->root.len > 0,
	      "format copy independence");
	check(!ConfigLegacy.browser_playlist_prefix.empty(),
	      "buffer copy independence");
	check(ConfigLegacy.playlist_editor_column_width_ratio[0] == 3,
	      "ratio copy independence");
	check(ConfigLegacy.columns[0].name == "column-name",
	      "column copy independence");
	check(ConfigLegacy.screen_sequence[0] == NCM_SCREEN_TYPE_PLAYLIST,
	      "screen sequence copy independence");
	check(ConfigLegacy.lyrics_fetchers.fetchers.len == 1,
	      "lyrics fetcher copy independence");
}

}

int main()
{
	Configuration source;
	configuration_init(&source);
	populateSource(&source);

	settings_legacy_sync_from_c(&source);
	verifySync(&source);
	verifyDeepCopies(&source);

	configuration_destroy(&source);
	verifyGlobalSyncWrapper();
	return EXIT_SUCCESS;
}
