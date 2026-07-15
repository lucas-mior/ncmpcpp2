#include <cstddef>
#include <exception>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "configuration.h"
#include "settings.h"
#include "settings_legacy.h"
#include "settings_legacy_runtime.h"
#include "song.h"

#undef Config

LegacyConfiguration ConfigLegacy;

namespace {

std::string stringFromC(const char *data, int32 length)
{
	if (data == nullptr || length <= 0)
		return std::string();
	return std::string(data, static_cast<size_t>(length));
}

Format::AST<char> copyFormat(const NcmFormatAst &source)
{
	Format::AST<char> result;
	if (!ncm_format_ast_copy(
	        result.cAst(), const_cast<NcmFormatAst *>(&source)))
		throw std::bad_alloc();
	return result;
}

NC::Buffer copyBuffer(const NcBuffer &source)
{
	NC::Buffer result;
	nc_buffer_destroy(result.cBuffer());
	nc_buffer_copy(result.cBuffer(), const_cast<NcBuffer *>(&source));
	return result;
}

std::vector<size_t> copyRatio(const NcmInt32Array &source)
{
	std::vector<size_t> result;
	result.reserve(static_cast<size_t>(source.len));
	for (int32 i = 0; i < source.len; i += 1)
	{
		if (source.items[i] < 0)
			throw std::logic_error("negative column width ratio");
		result.push_back(static_cast<size_t>(source.items[i]));
	}
	return result;
}

std::vector<LegacyColumn> copyColumns(const ColumnArray &source)
{
	std::vector<LegacyColumn> result;
	result.reserve(static_cast<size_t>(source.len));
	for (int32 i = 0; i < source.len; i += 1)
	{
		const Column &item = source.items[i];
		LegacyColumn column;
		column.name = stringFromC(item.name, item.name_len);
		column.type = stringFromC(item.type, item.type_len);
		column.width = item.width;
		column.stretch_limit = item.stretch_limit;
		column.color = NC::fromNcColor(item.color);
		column.fixed = item.fixed;
		column.right_alignment = item.right_alignment;
		column.display_empty_tag = item.display_empty_tag;
		result.push_back(std::move(column));
	}
	return result;
}

NC::Border copyBorder(const NcBorder &source)
{
	if (!source.enabled)
		return NC::Border();
	return NC::Border(NC::fromNcColor(source.color));
}

Regex::Flags copyRegexFlags(uint32 flags)
{
	switch (flags)
	{
		case NCM_REGEX_LITERAL_CASE_INSENSITIVE:
			return Regex::literalCaseInsensitive();
		case NCM_REGEX_BASIC_CASE_INSENSITIVE:
			return Regex::basicCaseInsensitive();
		case NCM_REGEX_EXTENDED_CASE_INSENSITIVE:
			return Regex::extendedCaseInsensitive();
		default:
			throw std::logic_error("unsupported regular expression flags");
	}
}

std::vector<ScreenType> copyScreenSequence(const ScreenTypeArray &source)
{
	if (source.len <= 0)
		return std::vector<ScreenType>();
	return std::vector<ScreenType>(
		source.items, source.items + source.len);
}

void copyLyricsFetchers(NcmLyricsFetcherRegistry &destination,
                        const NcmLyricsFetcherRegistry &source)
{
	NcmLyricsFetcherRegistry result;
	ncm_lyrics_fetcher_registry_init(&result);
	try
	{
		for (int32 i = 0; i < source.fetchers.len; i += 1)
		{
			auto item = ncm_lyrics_fetcher_registry_append(&result);
			if (item == nullptr)
				throw std::bad_alloc();
			if (!ncm_lyrics_fetcher_def_copy(
			        item, const_cast<NcmLyricsFetcherDef *>(
			                  &source.fetchers.items[i])))
			{
				result.fetchers.len -= 1;
				ncm_lyrics_fetcher_def_destroy(item);
				throw std::bad_alloc();
			}
		}
	}
	catch (...)
	{
		ncm_lyrics_fetcher_registry_destroy(&result);
		throw;
	}

	ncm_lyrics_fetcher_registry_destroy(&destination);
	destination = result;
}

}

void settings_legacy_sync_from_c(const Configuration *source)
{
	if (source == nullptr)
		throw std::invalid_argument("missing C configuration");

	ConfigLegacy.ncmpcpp_directory = stringFromC(source->ncmpcpp_directory, source->ncmpcpp_directory_len);
	ConfigLegacy.mpd_music_dir = stringFromC(source->mpd_music_dir, source->mpd_music_dir_len);
	ConfigLegacy.empty_tag = stringFromC(source->empty_tag, source->empty_tag_len);
	ConfigLegacy.lastfm_preferred_language = stringFromC(source->lastfm_preferred_language, source->lastfm_preferred_language_len);
	ConfigLegacy.pattern = stringFromC(source->pattern, source->pattern_len);
	ConfigLegacy.random_exclude_pattern = stringFromC(source->random_exclude_pattern, source->random_exclude_pattern_len);

	ConfigLegacy.song_list_format = copyFormat(source->song_list_format);
	ConfigLegacy.song_columns_mode_format = copyFormat(source->song_columns_mode_format);
	ConfigLegacy.browser_sort_format = copyFormat(source->browser_sort_format);

	ConfigLegacy.playlist_editor_column_width_ratio = copyRatio(source->playlist_editor_column_width_ratio);
	ConfigLegacy.columns = copyColumns(source->columns);

	ConfigLegacy.playlist_display_mode = source->playlist_display_mode;
	ConfigLegacy.browser_display_mode = source->browser_display_mode;
	ConfigLegacy.playlist_editor_display_mode = source->playlist_editor_display_mode;
	ConfigLegacy.design = source->design;
	ConfigLegacy.space_add_mode = source->space_add_mode;
	ConfigLegacy.media_lib_primary_tag = source->media_lib_primary_tag;
	ConfigLegacy.browser_sort_mode = source->browser_sort_mode;

	ConfigLegacy.browser_playlist_prefix = copyBuffer(source->browser_playlist_prefix);
	ConfigLegacy.selected_item_prefix = copyBuffer(source->selected_item_prefix);
	ConfigLegacy.selected_item_suffix = copyBuffer(source->selected_item_suffix);
	ConfigLegacy.now_playing_prefix = copyBuffer(source->now_playing_prefix);
	ConfigLegacy.now_playing_suffix = copyBuffer(source->now_playing_suffix);
	ConfigLegacy.modified_item_prefix = copyBuffer(source->modified_item_prefix);
	ConfigLegacy.current_item_prefix = copyBuffer(source->current_item_prefix);
	ConfigLegacy.current_item_suffix = copyBuffer(source->current_item_suffix);
	ConfigLegacy.current_item_inactive_column_prefix = copyBuffer(source->current_item_inactive_column_prefix);
	ConfigLegacy.current_item_inactive_column_suffix = copyBuffer(source->current_item_inactive_column_suffix);

	ConfigLegacy.main_color = NC::fromNcColor(source->main_color);

	ConfigLegacy.color2 = NC::FormattedColor(
		const_cast<NcFormattedColor *>(&source->color2));
	ConfigLegacy.empty_tags_color = NC::FormattedColor(
		const_cast<NcFormattedColor *>(&source->empty_tags_color));
	ConfigLegacy.volume_color = NC::FormattedColor(
		const_cast<NcFormattedColor *>(&source->volume_color));
	ConfigLegacy.alternative_ui_separator_color = NC::FormattedColor(
		const_cast<NcFormattedColor *>(&source->alternative_ui_separator_color));
	ConfigLegacy.window_border = copyBorder(source->window_border);
	ConfigLegacy.active_window_border = copyBorder(
		source->active_window_border);

	ConfigLegacy.playlist_separate_albums = source->playlist_separate_albums;
	ConfigLegacy.set_window_title = source->set_window_title;
	ConfigLegacy.header_visibility = source->header_visibility;
	ConfigLegacy.header_text_scrolling = source->header_text_scrolling;
	ConfigLegacy.statusbar_visibility = source->statusbar_visibility;
	ConfigLegacy.titles_visibility = source->titles_visibility;
	ConfigLegacy.centered_cursor = source->centered_cursor;
	ConfigLegacy.screen_switcher_previous = source->screen_switcher_previous;
	ConfigLegacy.wrapped_search = source->wrapped_search;
	ConfigLegacy.now_playing_lyrics = source->now_playing_lyrics;
	ConfigLegacy.fetch_lyrics_in_background = source->fetch_lyrics_in_background;
	ConfigLegacy.local_browser_show_hidden_files = source->local_browser_show_hidden_files;
	ConfigLegacy.display_bitrate = source->display_bitrate;
	ConfigLegacy.ignore_leading_the = source->ignore_leading_the;
	ConfigLegacy.use_cyclic_scrolling = source->use_cyclic_scrolling;
	ConfigLegacy.ask_before_clearing_playlists = source->ask_before_clearing_playlists;
	ConfigLegacy.mouse_support = source->mouse_support;
	ConfigLegacy.mouse_list_scroll_whole_page = source->mouse_list_scroll_whole_page;
	ConfigLegacy.data_fetching_delay = source->data_fetching_delay;
	ConfigLegacy.tag_editor_extended_numeration = source->tag_editor_extended_numeration;
	ConfigLegacy.discard_colors_if_item_is_selected = source->discard_colors_if_item_is_selected;
	ConfigLegacy.generate_win32_compatible_filenames = source->generate_win32_compatible_filenames;
	ConfigLegacy.ask_for_locked_screen_width_part = source->ask_for_locked_screen_width_part;
	ConfigLegacy.allow_for_physical_item_deletion = source->allow_for_physical_item_deletion;

	ConfigLegacy.crossfade_time = source->crossfade_time;
	ConfigLegacy.volume_change_step = source->volume_change_step;
	ConfigLegacy.lines_scrolled = source->lines_scrolled;

	ConfigLegacy.locked_screen_width_part = source->locked_screen_width_part;

	ConfigLegacy.selected_item_prefix_length = static_cast<size_t>(source->selected_item_prefix_length);
	ConfigLegacy.selected_item_suffix_length = static_cast<size_t>(source->selected_item_suffix_length);
	ConfigLegacy.now_playing_prefix_length = static_cast<size_t>(source->now_playing_prefix_length);
	ConfigLegacy.now_playing_suffix_length = static_cast<size_t>(source->now_playing_suffix_length);
	ConfigLegacy.current_item_prefix_length = static_cast<size_t>(source->current_item_prefix_length);
	ConfigLegacy.current_item_suffix_length = static_cast<size_t>(source->current_item_suffix_length);
	ConfigLegacy.current_item_inactive_column_prefix_length = static_cast<size_t>(source->current_item_inactive_column_prefix_length);
	ConfigLegacy.current_item_inactive_column_suffix_length = static_cast<size_t>(source->current_item_inactive_column_suffix_length);
	ConfigLegacy.regex_type = copyRegexFlags(source->regex_type);
	ConfigLegacy.screen_sequence = copyScreenSequence(
		source->screen_sequence);
	copyLyricsFetchers(ConfigLegacy.lyrics_fetchers,
	                  source->lyrics_fetchers);

	MPD::Song::TagsSeparator = stringFromC(
		source->tags_separator, source->tags_separator_len);
	MPD::Song::ShowDuplicateTags = source->show_duplicate_tags;
}

extern "C" bool settings_legacy_runtime_sync_configuration(void)
{
	try
	{
		if (configuration_is_quiet())
			std::clog.rdbuf(nullptr);
		settings_legacy_sync_from_c(&::Config);
	}
	catch (const std::exception &error)
	{
		std::cerr << "Error while synchronizing legacy configuration: "
		          << error.what() << "\n";
		return false;
	}
	catch (...)
	{
		std::cerr << "Error while synchronizing legacy configuration\n";
		return false;
	}
	return true;
}
