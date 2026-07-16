#include <cassert>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <string>

#include "actions_legacy.h"
#include "actions_legacy_runtime.h"
#include "app_binding_migration.h"
#include "app_legacy_bridge.h"
#include "charset.h"
#include "config.h"
#include "app_controller.h"
#include "global.h"
#include "ui_state.h"
#include "screens/screen_cpp_legacy.h"
#include "mpdpp.h"
#include "helpers_legacy.h"
#include "statusbar.h"
#include "status.h"
#include "utility/conversion.h"

#include "cbase/base_macros.h"
#include "curses/menu_impl.h"
#include "screens/native_c_screens.h"
#include "utility/string.h"
#include "utility/string_format.h"
#include "utility/utf8.h"
#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "c/ncm_type_conversions.h"
#include "tags.h"

#ifdef HAVE_TAGLIB_H
# include "c/ncm_taglib.h"
#endif // HAVE_TAGLIB_H


namespace ph = std::placeholders;

namespace {

bool currentSongFromNative(MPD::Song &song)
{
	NcmSong native_song;
	bool success;

	ncm_song_init(&native_song);
	success = ncm_action_current_song(&native_song);
	if (success)
	{
		try
		{
			song = MPD::Song(&native_song);
		}
		catch (...)
		{
			success = false;
		}
	}
	ncm_song_destroy(&native_song);
	return success;
}

bool currentMediaLibraryArtistTag(std::string &artist)
{
	NativeMediaLibraryScreen *library;
	char *tag;
	int32 tag_len;

	if (!native_c_screen_media_library_is_current())
		return false;
	library = native_c_screen_media_library();
	if (native_media_library_screen_active_column(library)
	    != NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)
		return false;
	if (Config.media_lib_primary_tag != MPD_TAG_ARTIST)
		return false;
	if (!native_media_library_screen_current_primary_tag_value(
	        library, &tag, &tag_len))
		return false;

	artist.assign(tag, static_cast<size_t>(tag_len));
	return true;
}

bool currentMediaLibraryTag(std::string &tag)
{
	NativeMediaLibraryScreen *library;
	char *value;
	int32 value_len;

	if (!native_c_screen_media_library_is_current())
		return false;
	library = native_c_screen_media_library();
	if (native_media_library_screen_active_column(library)
	    != NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)
		return false;
	if (!native_media_library_screen_current_primary_tag_value(
	        library, &value, &value_len))
		return false;
	tag.assign(value, static_cast<size_t>(value_len));
	return true;
}

bool currentMediaLibraryAlbum(std::string &album)
{
	NativeMediaLibraryScreen *library;
	char *value;
	int32 value_len;

	if (!native_c_screen_media_library_is_current())
		return false;
	library = native_c_screen_media_library();
	if (native_media_library_screen_active_column(library)
	    != NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS)
		return false;
	if (!native_media_library_screen_current_album_value(
	        library, &value, &value_len))
		return false;
	album.assign(value, static_cast<size_t>(value_len));
	return true;
}



std::vector<std::shared_ptr<Actions::BaseAction>> AvailableActions;

template <typename Helper>
bool promptHook(char *text, void *data)
{
	Helper *helper = static_cast<Helper *>(data);
	return (*helper)(text);
}

int32 promptTextLen(char *text)
{
	if (text == nullptr)
		return 0;
	return static_cast<int32>(std::strlen(text));
}

void handleClientError(MPD::ClientError &e)
{
	ncm_status_handle_client_error_value(
	    &global_mpd, const_cast<char *>(e.what()), -1, e.clearable());
}

void handleServerError(MPD::ServerError &e)
{
	ncm_status_handle_server_error_value(
	    &global_mpd, static_cast<int32>(e.code()),
	    const_cast<char *>(e.what()), -1);
}

bool highlightPlaylistMpdPosition(int32 position)
{
	if (position < 0)
		return false;

	if (!native_playlist_screen_locate_position(
	        native_c_screen_playlist(), static_cast<uint32>(position)))
	{
		Statusbar::print("Song is filtered out");
		return false;
	}
	return true;
}

bool statusbarMainPromptHook(char *text, void *)
{
	return ncm_statusbar_main_hook(text, promptTextLen(text));
}

bool promptString(std::string &result,
                  const std::string &initial = std::string(),
                  NcPromptHook hook = nullptr,
                  void *hook_user_data = nullptr,
                  bool print_aborted = true)
{
	char *input = nullptr;
	NcPrompt prompt = {};

	prompt.initial_text = const_cast<char *>(initial.c_str());
	prompt.width = -1;
	if (hook == nullptr)
	{
		prompt.hook = statusbarMainPromptHook;
		prompt.hook_user_data = nullptr;
	}
	else
	{
		prompt.hook = hook;
		prompt.hook_user_data = hook_user_data;
	}
	prompt.encrypted = false;
	prompt.remember = true;

	if (nc_window_prompt(ui_state_footer_window(), &prompt, &input)
	    != NC_PROMPT_ACCEPTED)
	{
		nc_window_prompt_result_destroy(input);
		if (print_aborted)
			Statusbar::printf("Action aborted");
		return false;
	}

	if (input != nullptr)
		result = input;
	else
		result.clear();
	nc_window_prompt_result_destroy(input);
	return true;
}

bool promptOneOf(char *values, int32 values_len, char &result,
                 bool print_aborted = true)
{
	if (ncm_statusbar_prompt_return_one_of(
	        ui_state_footer_window(), values, values_len, &result))
		return true;
	if (print_aborted)
		Statusbar::printf("Action aborted");
	return false;
}

char *itemTypeName(MPD::Item::Type type)
{
	switch (type)
	{
		case MPD::Item::Type::Directory:
			return ncm_item_type_name(NCM_ITEM_DIRECTORY);
		case MPD::Item::Type::Song:
			return ncm_item_type_name(NCM_ITEM_SONG);
		case MPD::Item::Type::Playlist:
			return ncm_item_type_name(NCM_ITEM_PLAYLIST);
	}
	return ncm_item_type_name(NCM_ITEM_UNKNOWN);
}

void populateActions();


bool actionHasNoLegacyImplementation(NcmActionType type)
{
	switch (type)
	{
		case NCM_ACTION_SCROLL_UP_ARTIST:
		case NCM_ACTION_SCROLL_UP_ALBUM:
		case NCM_ACTION_SCROLL_DOWN_ARTIST:
		case NCM_ACTION_SCROLL_DOWN_ALBUM:
		case NCM_ACTION_PREVIOUS_COLUMN:
		case NCM_ACTION_NEXT_COLUMN:
		case NCM_ACTION_ADD_ITEM_TO_PLAYLIST:
		case NCM_ACTION_PLAY_ITEM:
		case NCM_ACTION_START_SEARCHING:
		case NCM_ACTION_SELECT_ITEM:
		case NCM_ACTION_SELECT_RANGE:
		case NCM_ACTION_REVERSE_SELECTION:
		case NCM_ACTION_REMOVE_SELECTION:
		case NCM_ACTION_SELECT_ALBUM:
		case NCM_ACTION_SELECT_FOUND_ITEMS:
			return true;
		default:
			return false;
	}
}


template <typename Iterator>
Iterator nextScreenTypeInSequence(Iterator first, Iterator last, ScreenType type)
{
	auto it = std::find(first, last, type);
	if (it == last)
		return first;
	else
	{
		++it;
		if (it == last)
			return first;
		else
			return it;
	}
}

}

namespace Actions {

bool ExitMainLoop = false;

bool confirmAction(const std::string &description)
{
	Statusbar::ScopedLock slock;
	Statusbar::put() << description
	<< " [" << NC_FORMAT_BOLD << 'y' << NC_FORMAT_NO_BOLD
	<< '/' << NC_FORMAT_BOLD << 'n' << NC_FORMAT_NO_BOLD
	<< "] ";
	char answer;
	char values[] = {'y', 'n'};
	if (!promptOneOf(values, static_cast<int32>(sizeof(values)), answer, false))
	{
		Statusbar::print("Action cancelled");
		return false;
	}
	if (answer == 'n')
	{
		Statusbar::print("Action cancelled");
		return false;
	}
	return true;
}

bool isMPDMusicDirSet()
{
	if (Config.mpd_music_dir.empty())
	{
		Statusbar::print("Proper mpd_music_dir variable has to be set in configuration file");
		return false;
	}
	return true;
}

BaseAction *runtimeAction(enum NcmActionType type)
{
	NcmActionDef *definition = ncm_action_get(type);
	if (definition == nullptr)
	{
		Statusbar::printf("Unknown action type: %1%", static_cast<int>(type));
		return nullptr;
	}

	if (AvailableActions.empty())
		populateActions();

	auto index = static_cast<size_t>(type);
	if (index >= AvailableActions.size() || AvailableActions[index] == nullptr)
	{
		if (app_binding_migration_action_is_c_safe(type)
		    || actionHasNoLegacyImplementation(type))
			return nullptr;

		std::cerr << "fatal: action not implemented: "
		          << definition->name << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return AvailableActions[index].get();
}

bool execute(enum NcmActionType type)
{
	if (app_binding_migration_action_is_c_safe(type))
		return ncm_action_runtime_run(nullptr, type);

	BaseAction *action = runtimeAction(type);
	return action != nullptr && action->execute();
}

bool MouseEvent::canBeRun()
{
	return Config.mouse_support;
}

void MouseEvent::run()
{

	m_old_mouse_event = m_mouse_event;
	if (NcWindow *footer_window = ui_state_footer_window())
		m_mouse_event = *nc_window_mouse_event(footer_window);

	//Statusbar::printf("(%1%, %2%, %3%)", m_mouse_event.bstate, m_mouse_event.x, m_mouse_event.y);

	if (m_mouse_event.bstate & BUTTON1_PRESSED
	&&  m_mouse_event.y == LINES-(Config.statusbar_visibility ? 2 : 1)
	   ) // progressbar
	{
		if (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP)
			return;
		Mpd.Seek(ncm_status_state_current_song_position(),
			ncm_status_state_total_time()*m_mouse_event.x/double(COLS));
	}
	else if (m_mouse_event.bstate & BUTTON1_PRESSED
	     &&  (Config.statusbar_visibility || Config.design == NCM_DESIGN_ALTERNATIVE)
	     &&  ncm_status_state_player() != NCM_STATUS_PLAYER_STOP
	     &&  m_mouse_event.y == (Config.design == NCM_DESIGN_ALTERNATIVE ? 1 : LINES-1)
			 &&  m_mouse_event.x < 9
		) // playing/paused
	{
		Mpd.Toggle();
	}
	else if ((m_mouse_event.bstate & BUTTON5_PRESSED || m_mouse_event.bstate & BUTTON4_PRESSED)
	     &&	 (Config.header_visibility || Config.design == NCM_DESIGN_ALTERNATIVE)
	     &&	 m_mouse_event.y == 0 && size_t(m_mouse_event.x) > COLS-global_volume_state_len()
	) // volume
	{
		if (m_mouse_event.bstate & BUTTON5_PRESSED)
			Actions::execute(NCM_ACTION_VOLUME_DOWN);
		else
			Actions::execute(NCM_ACTION_VOLUME_UP);
	}
	else if (m_mouse_event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED | BUTTON4_PRESSED | BUTTON5_PRESSED))
		app_controller_mouse_button_pressed_current(m_mouse_event);
}



bool EditSong::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	return !native_c_screen_lyrics_is_current()
	    && isMPDMusicDirSet()
	    && currentSongFromNative(m_song);
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditSong::run()
{
#	ifdef HAVE_TAGLIB_H
	(void)ncm_action_edit_song(m_song.cSong());
#	endif // HAVE_TAGLIB_H
}

bool EditLibraryTag::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	std::string tag;

	return currentMediaLibraryTag(tag) && isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryTag::run()
{
#	ifdef HAVE_TAGLIB_H

	std::string current_tag;
	std::string new_tag;
	if (!currentMediaLibraryTag(current_tag))
		return;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << NC_FORMAT_BOLD << ncm_tag_type_name(Config.media_lib_primary_tag) << NC_FORMAT_NO_BOLD << ": ";
		if (!promptString(new_tag, current_tag))
				return;
	}
	if (!new_tag.empty() && new_tag != current_tag)
	{
		Statusbar::print("Updating tags...");
		Mpd.StartSearch(true);
		Mpd.AddSearch(Config.media_lib_primary_tag, current_tag);
		enum NcmTagsField field = ncm_tags_field_from_tag_type(Config.media_lib_primary_tag);
		assert(field != NCM_TAGS_FIELD_LAST);
		bool success = true;
		std::string dir_to_update;
		for (MPD::SongIterator s = Mpd.CommitSearchSongs(), end; s != end; ++s)
		{
			MPD::MutableSong ms = std::move(*s);
			ms.setTags(field, new_tag);
			Statusbar::printf("Updating tags in \"%1%\"...", ms.getName());
			std::string path = Config.mpd_music_dir + ms.getURI();
			if (!ncm_tags_write_mutable_song(ms))
			{
				success = false;
				Statusbar::printf("Error while writing tags to \"%1%\": %2%",
				                  ms.getName(), strerror(errno));
				s.finish();
				break;
			}
			if (dir_to_update.empty())
				dir_to_update = ms.getURI();
			else
				dir_to_update = getSharedDirectory(dir_to_update, ms.getURI());
		};
		if (success)
		{
			Mpd.UpdateDirectory(dir_to_update);
			Statusbar::print("Tags updated successfully");
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditLibraryAlbum::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	std::string album;

	return currentMediaLibraryAlbum(album) && isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryAlbum::run()
{
#	ifdef HAVE_TAGLIB_H
		// FIXME: merge this and EditLibraryTag. also, prompt on failure if user wants to continue
	std::string current_album;
	std::string new_album;
	if (!currentMediaLibraryAlbum(current_album))
		return;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << NC_FORMAT_BOLD << "Album: " << NC_FORMAT_NO_BOLD;
		if (!promptString(new_album, current_album))
				return;
	}
	if (!new_album.empty() && new_album != current_album)
	{
		NcmError error;
		NcmSongArray songs;
		bool success;
		std::string shared_directory;

		ncm_song_array_init(&songs);
		ncm_error_clear(&error);
		success = native_media_library_screen_copy_visible_songs(
		    native_c_screen_media_library(), &songs, &error);
		if (!success)
		{
			if (ncm_error_is_set(&error))
				Statusbar::print(error.message);
			ncm_song_array_destroy(&songs);
			return;
		}
		Statusbar::print("Updating tags...");
		for (int32 i = 0; i < songs.len; i += 1)
		{
			MPD::Song song(&songs.items[i]);
			std::string directory = song.getDirectory();

			Statusbar::printf("Updating tags in \"%1%\"...", song.getName());
			std::string path = Config.mpd_music_dir + song.getURI();
			if (shared_directory.empty())
				shared_directory = directory;
			else
				shared_directory = getSharedDirectory(shared_directory, directory);
			NcmTaglibFile file;
			ncm_taglib_file_init(&file);
			if (!ncm_taglib_file_open(&file, const_cast<char *>(path.c_str())))
			{
				const char msg[] = "Error while opening file \"%1%\"";
				Statusbar::printf(msg, Utf8::shorten(song.getURI(), COLS-const_strlen(msg)));
				success = false;
				break;
			}
			ncm_taglib_clear_property(&file, const_cast<char *>("ALBUM"));
			ncm_taglib_append_property(&file, const_cast<char *>("ALBUM"),
			                           const_cast<char *>(new_album.c_str()));
			if (!ncm_taglib_file_save(&file))
			{
				const char msg[] = "Error while writing tags in \"%1%\"";
				Statusbar::printf(msg, Utf8::shorten(song.getURI(), COLS-const_strlen(msg)));
				ncm_taglib_file_close(&file);
				success = false;
				break;
			}
			ncm_taglib_file_close(&file);
		}
		if (success && !shared_directory.empty())
		{
			Mpd.UpdateDirectory(shared_directory);
			Statusbar::print("Tags updated successfully");
		}
		ncm_song_array_destroy(&songs);
	}
#	endif // HAVE_TAGLIB_H
}



void ToggleScreenLock::run()
{
		const char *msg_unlockable_screen = "Current screen can't be locked";
	if (screenLegacyLocked() != nullptr)
	{
		BaseScreen::unlock();
		native_c_screens_request_registered_resize();
		native_c_screen_lyrics_set_resize();
		app_controller_resize_current_screen();
		Statusbar::print("Screen unlocked");
	}
	else if (!screenLegacyCurrent()->isLockable())
	{
		Statusbar::print(msg_unlockable_screen);
	}
	else
	{
		unsigned part = Config.locked_screen_width_part*100;
		if (Config.ask_for_locked_screen_width_part)
		{
			Statusbar::ScopedLock slock;
			Statusbar::put() << "% of the locked screen's width to be reserved (20-80): ";
			std::string input;
			if (!promptString(input, std::to_string(part)))
				return;
			part = fromString<unsigned>(input);
		}
		boundsCheck(part, 20u, 80u);
		Config.locked_screen_width_part = part/100.0;
		if (screenLegacyCurrent()->lock())
			Statusbar::printf("Screen locked (with %1%%% width)", part);
		else
			Statusbar::print(msg_unlockable_screen);
	}
}



bool Find::canBeRun()
{
	return native_c_screen_help_is_current()
		|| native_c_screen_lastfm_is_current()
		|| native_c_screen_lyrics_is_current();
}

void Find::run()
{

	std::string token;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Find: ";
		if (!promptString(token))
				return;
	}

	Statusbar::print("Searching...");
	if (native_c_screen_lastfm_is_current())
	{
		NcmError error = {};
		bool found;

		found = native_lastfm_screen_find(
			native_c_screen_lastfm(),
			const_cast<char *>(token.data()),
			static_cast<int32>(token.size()),
			&error);
		if (token.empty() || found)
			Statusbar::print("Done");
		else
			Statusbar::print("No matching patterns found");
		return;
	}

	if (native_c_screen_lyrics_is_current())
	{
		NcmError error = {};
		bool found;

		found = native_lyrics_screen_find(
			native_c_screen_lyrics(),
			const_cast<char *>(token.data()),
			static_cast<int32>(token.size()),
			&error);
		if (token.empty() || found)
			Statusbar::print("Done");
		else
			Statusbar::print("No matching patterns found");
		return;
	}

	auto s = static_cast<Screen<NC::Scrollpad> *>(screenLegacyCurrent());
	s->main().removeProperties();
	if (token.empty() || s->main().setProperties(NC_FORMAT_REVERSE, token, NC_FORMAT_NO_REVERSE, Config.regex_type))
		Statusbar::print("Done");
	else
		Statusbar::print("No matching patterns found");
	s->main().flush();
}

bool NextFoundItem::canBeRun()
{
	return dynamic_cast<Searchable *>(screenLegacyCurrent());
}

void NextFoundItem::run()
{
	Searchable *w = dynamic_cast<Searchable *>(screenLegacyCurrent());
	assert(w != nullptr);
	w->search(NCM_SEARCH_DIRECTION_FORWARD, Config.wrapped_search, true);
	nc_screen_finish_list_change(app_controller_current_screen());
}

bool PreviousFoundItem::canBeRun()
{
	return dynamic_cast<Searchable *>(screenLegacyCurrent());
}

void PreviousFoundItem::run()
{
	Searchable *w = dynamic_cast<Searchable *>(screenLegacyCurrent());
	assert(w != nullptr);
	w->search(NCM_SEARCH_DIRECTION_BACKWARD, Config.wrapped_search, true);
	nc_screen_finish_list_change(app_controller_current_screen());
}


bool ToggleLibraryTagType::canBeRun()
{
	NativeMediaLibraryScreen *library;
	enum NativeMediaLibraryColumn column;

	if (!native_c_screen_media_library_is_current())
		return false;
	library = native_c_screen_media_library();
	column = native_media_library_screen_active_column(library);
	return column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS
	    || (native_media_library_screen_column_count(library) == 2
	        && column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
}

void ToggleLibraryTagType::run()
{

	char tag_type = 0;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Tag type? "
		<< "[" << NC_FORMAT_BOLD << 'a' << NC_FORMAT_NO_BOLD << "rtist"
		<< "/" << "album" << NC_FORMAT_BOLD << 'A' << NC_FORMAT_NO_BOLD << "rtist"
		<< "/" << NC_FORMAT_BOLD << 'y' << NC_FORMAT_NO_BOLD << "ear"
		<< "/" << NC_FORMAT_BOLD << 'g' << NC_FORMAT_NO_BOLD << "enre"
		<< "/" << NC_FORMAT_BOLD << 'c' << NC_FORMAT_NO_BOLD << "omposer"
		<< "/" << NC_FORMAT_BOLD << 'p' << NC_FORMAT_NO_BOLD << "erformer"
		<< "] ";
		char values[] = {'a', 'A', 'y', 'g', 'c', 'p'};
		if (!promptOneOf(values, static_cast<int32>(sizeof(values)), tag_type))
			return;
	}
	mpd_tag_type new_tagitem = ncm_char_to_tag_type(tag_type);
	if (new_tagitem != Config.media_lib_primary_tag)
	{
		native_media_library_screen_set_primary_tag_type(
		    native_c_screen_media_library(), new_tagitem);
		std::string item_type = ncm_tag_type_name(new_tagitem);
		item_type = lowercaseAscii(item_type);
		Statusbar::printf("Switched to the list of %1%s", item_type);
	}
}



bool ShowArtistInfo::canBeRun()
{
	MPD::Song song;
	std::string artist;

	if (native_c_screen_lastfm_is_current())
		return true;
	if (currentMediaLibraryArtistTag(artist))
		return true;

	return currentSongFromNative(song);
}

void ShowArtistInfo::run()
{
	std::string artist;

	if (native_c_screen_lastfm_is_current())
	{
		native_c_screen_lastfm_switch_to();
		return;
	}

	if (!currentMediaLibraryArtistTag(artist))
	{
		MPD::Song song;

		if (!currentSongFromNative(song))
			return;
		artist = song.getArtist();
	}

	if (!artist.empty())
	{
		NcmError error = {};

		(void)native_lastfm_screen_queue_artist_info(
			native_c_screen_lastfm(),
			const_cast<char *>(artist.data()),
			static_cast<int32>(artist.size()),
			const_cast<char *>(Config.lastfm_preferred_language.data()),
			static_cast<int32>(Config.lastfm_preferred_language.size()),
			&error);
		if (!app_controller_is_screen_visible(native_c_screen_lastfm_native()))
			native_c_screen_lastfm_switch_to();
	}
}



void NextScreen::run()
{
	if (Config.screen_switcher_previous)
	{
		BaseScreen *current = screenLegacyCurrent();

		if (auto tababble = dynamic_cast<Tabbable *>(current))
			tababble->switchToPreviousScreen();
		else if (current == nullptr)
			(void)app_controller_switch_to_screen(
				app_controller_previous_screen());
	}
	else if (!Config.screen_sequence.empty())
	{
		auto screen = nextScreenTypeInSequence(
			Config.screen_sequence.begin(),
			Config.screen_sequence.end(),
			native_c_screens_current_type());
		(void)native_c_screens_switch_to_type(*screen);
	}
}

void PreviousScreen::run()
{
	if (Config.screen_switcher_previous)
	{
		BaseScreen *current = screenLegacyCurrent();

		if (auto tababble = dynamic_cast<Tabbable *>(current))
			tababble->switchToPreviousScreen();
		else if (current == nullptr)
			(void)app_controller_switch_to_screen(
				app_controller_previous_screen());
	}
	else if (!Config.screen_sequence.empty())
	{
		auto screen = nextScreenTypeInSequence(
			Config.screen_sequence.rbegin(),
			Config.screen_sequence.rend(),
			native_c_screens_current_type());
		(void)native_c_screens_switch_to_type(*screen);
	}
}



}

namespace {

void populateActions()
{
	AvailableActions.resize(static_cast<size_t>(Actions::Type::_numberOfActions));
	auto insert_action = [](Actions::BaseAction *a) {
		AvailableActions.at(static_cast<size_t>(a->type())).reset(a);
	};
	insert_action(new Actions::MouseEvent());
	insert_action(new Actions::EditSong());
	insert_action(new Actions::EditLibraryTag());
	insert_action(new Actions::EditLibraryAlbum());
	insert_action(new Actions::ToggleScreenLock());
	insert_action(new Actions::Find());
	insert_action(new Actions::NextFoundItem());
	insert_action(new Actions::PreviousFoundItem());
	insert_action(new Actions::ToggleLibraryTagType());
	insert_action(new Actions::ShowArtistInfo());
	insert_action(new Actions::NextScreen());
	insert_action(new Actions::PreviousScreen());
	NcmError action_error;
	ncm_error_clear(&action_error);
	if (!ncm_action_validate(&action_error))
	{
		std::cerr << "fatal: invalid action table: "
		          << action_error.message << std::endl;
		std::exit(EXIT_FAILURE);
	}

	for (int32 i = 0; i < NCM_ACTION_LAST; i += 1)
	{
		auto type = static_cast<NcmActionType>(i);

		if (AvailableActions[static_cast<size_t>(i)] == nullptr
		    && !app_binding_migration_action_is_c_safe(type)
		    && !actionHasNoLegacyImplementation(type))
		{
			std::cerr << "fatal: undefined action: "
			          << ncm_action_type_name(type)
			          << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
}


}


extern "C" {

bool actions_legacy_runtime_playlist_highlight_mpd_position(int32 position)
{
	return highlightPlaylistMpdPosition(position);
}

bool actions_legacy_runtime_search_current_screen(
    enum SearchDirection direction, char *pattern, int32 pattern_len,
    bool wrap, bool skip_current, bool *handled, NcmError *error)
{
	Searchable *searchable;

	if (handled == nullptr)
		return false;
	*handled = false;
	searchable = dynamic_cast<Searchable *>(screenLegacyCurrent());
	if (searchable == nullptr)
		return false;

	*handled = true;
	try
	{
		if (pattern == nullptr || pattern_len <= 0)
		{
			searchable->clearSearchConstraint();
			return false;
		}
		searchable->setSearchConstraint(std::string(
			pattern, static_cast<size_t>(pattern_len)));
		return searchable->search(direction, wrap, skip_current);
	}
	catch (std::exception &e)
	{
		ncm_error_set(error, EINVAL, const_cast<char *>(e.what()),
		              static_cast<int32>(std::strlen(e.what())));
	}
	return false;
}

void actions_legacy_runtime_clear_current_search(void)
{
	Searchable *searchable;

	searchable = dynamic_cast<Searchable *>(screenLegacyCurrent());
	if (searchable == nullptr)
		return;

	searchable->clearSearchConstraint();
}

bool actions_legacy_runtime_can_run_action(enum NcmActionType type)
{
	if (app_binding_migration_action_is_c_safe(type))
		return ncm_action_runtime_can_run(nullptr, type);

	try
	{
		Actions::BaseAction *action;

		action = Actions::runtimeAction(type);
		if (action == nullptr)
			return false;
		return action->canBeRun();
	}
	catch (ConversionError &e)
	{
		Statusbar::printf("Invalid value: %1%", e.value());
	}
	catch (OutOfBounds &e)
	{
		Statusbar::printf("Error: %1%", e.errorMessage());
	}
	catch (NC::PromptAborted &)
	{
		Statusbar::printf("Action aborted");
	}
	catch (MPD::ClientError &e)
	{
		handleClientError(e);
	}
	catch (MPD::ServerError &e)
	{
		handleServerError(e);
	}
	catch (std::exception &e)
	{
		Statusbar::printf("Unexpected error: %1%", e.what());
	}
	return false;
}

bool actions_legacy_runtime_execute_action(enum NcmActionType type)
{
	if (app_binding_migration_action_is_c_safe(type))
		return ncm_action_runtime_run(nullptr, type);

	try
	{
		return Actions::execute(type);
	}
	catch (MPD::ClientError &e)
	{
		handleClientError(e);
	}
	catch (MPD::ServerError &e)
	{
		handleServerError(e);
	}
	catch (std::exception &e)
	{
		Statusbar::printf("Unexpected error: %1%", e.what());
	}
	return false;
}

void actions_legacy_runtime_request_exit(void)
{
	Actions::ExitMainLoop = true;
}

bool actions_legacy_runtime_exit_requested(void)
{
	return Actions::ExitMainLoop;
}

}
