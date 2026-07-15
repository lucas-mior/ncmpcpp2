#ifndef NCMPCPP_SETTINGS_LEGACY_H
#define NCMPCPP_SETTINGS_LEGACY_H

#include <string>
#include <vector>
#include <mpd/client.h>

#include "curses/formatted_color.h"
#include "curses/strbuffer.h"
#include "c/ncm_enums.h"
#include "format.h"
#include "lyrics_fetcher.h"
#include "screens/screen_type.h"
#include "utility/regex.h"

struct LegacyColumn
{
	LegacyColumn() : stretch_limit(-1), right_alignment(0), display_empty_tag(1) { }

	std::string name;
	std::string type;
	int width;
	int stretch_limit;
	NC::Color color;
	bool fixed;
	bool right_alignment;
	bool display_empty_tag;
};

struct LegacyConfiguration
{
	LegacyConfiguration()
	{
		ncm_lyrics_fetcher_registry_init(&lyrics_fetchers);
	}

	~LegacyConfiguration()
	{
		ncm_lyrics_fetcher_registry_destroy(&lyrics_fetchers);
	}

	LegacyConfiguration(const LegacyConfiguration &) = delete;
	LegacyConfiguration &operator=(const LegacyConfiguration &) = delete;

	std::string ncmpcpp_directory;

	std::string mpd_music_dir;
	std::string empty_tag;

	Format::AST<char> song_list_format;
	Format::AST<char> song_columns_mode_format;
	Format::AST<char> browser_sort_format;

	std::string lastfm_preferred_language;

	std::string pattern;

	std::vector<size_t> playlist_editor_column_width_ratio;

	std::vector<LegacyColumn> columns;

	DisplayMode playlist_display_mode;
	DisplayMode browser_display_mode;
	DisplayMode playlist_editor_display_mode;

	NC::Buffer browser_playlist_prefix;
	NC::Buffer selected_item_prefix;
	NC::Buffer selected_item_suffix;
	NC::Buffer now_playing_prefix;
	NC::Buffer now_playing_suffix;
	NC::Buffer modified_item_prefix;
	NC::Buffer current_item_prefix;
	NC::Buffer current_item_suffix;
	NC::Buffer current_item_inactive_column_prefix;
	NC::Buffer current_item_inactive_column_suffix;

	NC::Color main_color;

	NC::FormattedColor color2;
	NC::FormattedColor empty_tags_color;
	NC::FormattedColor volume_color;
	NC::FormattedColor alternative_ui_separator_color;

	NC::Border window_border;
	NC::Border active_window_border;

	Design design;

	SpaceAddMode space_add_mode;

	mpd_tag_type media_lib_primary_tag;

	bool playlist_separate_albums;
	bool set_window_title;
	bool header_visibility;
	bool header_text_scrolling;
	bool statusbar_visibility;
	bool titles_visibility;
	bool centered_cursor;
	bool screen_switcher_previous;
	bool wrapped_search;
	bool now_playing_lyrics;
	bool fetch_lyrics_in_background;
	bool local_browser_show_hidden_files;
	bool display_bitrate;
	bool ignore_leading_the;
	bool use_cyclic_scrolling;
	bool ask_before_clearing_playlists;
	bool mouse_support;
	bool mouse_list_scroll_whole_page;
	bool data_fetching_delay;
	bool tag_editor_extended_numeration;
	bool discard_colors_if_item_is_selected;
	bool generate_win32_compatible_filenames;
	bool ask_for_locked_screen_width_part;
	bool allow_for_physical_item_deletion;

	unsigned crossfade_time;
	unsigned volume_change_step;
	unsigned lines_scrolled;

	Regex::Flags regex_type;

	double locked_screen_width_part;

	size_t selected_item_prefix_length;
	size_t selected_item_suffix_length;
	size_t now_playing_prefix_length;
	size_t now_playing_suffix_length;
	size_t current_item_prefix_length;
	size_t current_item_suffix_length;
	size_t current_item_inactive_column_prefix_length;
	size_t current_item_inactive_column_suffix_length;

	std::vector<ScreenType> screen_sequence;

	std::string random_exclude_pattern;
	SortMode browser_sort_mode;

	NcmLyricsFetcherRegistry lyrics_fetchers;
};

struct Configuration;

void settings_legacy_sync_from_c(const Configuration *source);

extern LegacyConfiguration ConfigLegacy;

#define Config ConfigLegacy

#endif // NCMPCPP_SETTINGS_LEGACY_H
