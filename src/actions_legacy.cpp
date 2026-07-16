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
#include "macro_utilities.h"
#include "screens/native_c_screens.h"
#include "utility/string.h"
#include "utility/string_format.h"
#include "utility/utf8.h"
#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "c/ncm_type_conversions.h"
#include "title_legacy.h"
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

bool nativeMediaLibraryItemAvailable()
{
	if (!native_c_screen_media_library_is_current())
		return false;
	return native_media_library_screen_item_available(
	    native_c_screen_media_library());
}

bool addNativeMediaLibraryItemToPlaylist(bool play)
{
	NcmError error;
	bool success;

	ncm_error_clear(&error);
	success = native_media_library_screen_add_item_to_playlist(
	    native_c_screen_media_library(), play, &error);
	if (!success && ncm_error_is_set(&error))
		Statusbar::print(error.message);
	return success;
}


void nativeLyricsPrintFetcher()
{
	NcmLyricsFetcherDef *fetcher;

	fetcher = native_lyrics_screen_toggle_fetcher(
		native_c_screen_lyrics(), &Config.lyrics_fetchers);
	if (fetcher != nullptr)
		Statusbar::printf("Using lyrics fetcher: %s",
		                  ncm_lyrics_fetcher_name(fetcher));
	else
		Statusbar::print("Using all lyrics fetchers");
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

bool scrollTagCanBeRun(NC::List *&list, const SongList *&songs);
void scrollTagUpRun(NC::List *list, const SongList *songs, enum NcmSongGetter getter);
void scrollTagDownRun(NC::List *list, const SongList *songs, enum NcmSongGetter getter);

void listsChangeFinisher();

bool actionHasNoLegacyImplementation(NcmActionType type)
{
	return type == NCM_ACTION_START_SEARCHING;
}

bool legacyCanRunAction(NcmActionType type, void *)
{
	if (app_binding_migration_action_is_c_safe(type))
		return ncm_action_runtime_can_run(nullptr, type);

	Actions::BaseAction *action = Actions::runtimeAction(type);
	if (action == nullptr)
		return false;
	return action->canBeRun();
}

bool legacyRunAction(NcmActionType type, void *)
{
	if (app_binding_migration_action_is_c_safe(type))
		return ncm_action_runtime_run(nullptr, type);

	Actions::BaseAction *action = Actions::runtimeAction(type);
	if (action == nullptr)
		return false;
	return action->execute();
}

bool legacyCurrentScreenIs(ScreenType screen_type, void *)
{
	BaseScreen *current;

	current = screenLegacyCurrent();
	if (current == nullptr)
		return false;
	return current->type() == screen_type;
}

void legacyPushKey(NcKey key, void *)
{
	nc_window_push_key(ui_state_footer_window(), key);
}

bool legacyRunExternalCommand(char *command, int32 command_len, void *)
{
	std::string text;

	text.assign(command, static_cast<size_t>(command_len));
	runExternalCommand(text, true);
	return true;
}

bool legacyRunExternalConsoleCommand(char *command, int32 command_len, void *)
{
	std::string text;

	text.assign(command, static_cast<size_t>(command_len));
	runExternalConsoleCommand(text);
	return true;
}

NcmBindingRuntime *bindingsLegacyRuntime()
{
	static NcmBindingRuntime runtime = {
		legacyCanRunAction,
		legacyRunAction,
		legacyCurrentScreenIs,
		legacyPushKey,
		legacyRunExternalCommand,
		legacyRunExternalConsoleCommand,
		nullptr,
	};

	return &runtime;
}

bool bindingsLegacyExecute(NcmBinding *binding)
{
	return ncm_binding_execute_runtime(binding, bindingsLegacyRuntime());
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

void ScrollUp::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_UP);
	listsChangeFinisher();
}

void ScrollDown::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_DOWN);
	listsChangeFinisher();
}

bool ScrollUpArtist::canBeRun()
{
	return scrollTagCanBeRun(m_list, m_songs);
}

void ScrollUpArtist::run()
{
	scrollTagUpRun(m_list, m_songs, NCM_SONG_GETTER_ARTIST);
}

bool ScrollUpAlbum::canBeRun()
{
	return scrollTagCanBeRun(m_list, m_songs);
}

void ScrollUpAlbum::run()
{
	scrollTagUpRun(m_list, m_songs, NCM_SONG_GETTER_ALBUM);
}

bool ScrollDownArtist::canBeRun()
{
	return scrollTagCanBeRun(m_list, m_songs);
}

void ScrollDownArtist::run()
{
	scrollTagDownRun(m_list, m_songs, NCM_SONG_GETTER_ARTIST);
}

bool ScrollDownAlbum::canBeRun()
{
	return scrollTagCanBeRun(m_list, m_songs);
}

void ScrollDownAlbum::run()
{
	scrollTagDownRun(m_list, m_songs, NCM_SONG_GETTER_ALBUM);
}

void PageUp::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_PAGE_UP);
	listsChangeFinisher();
}

void PageDown::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_PAGE_DOWN);
	listsChangeFinisher();
}

void MoveHome::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_HOME);
	listsChangeFinisher();
}

void MoveEnd::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_END);
	listsChangeFinisher();
}

void ToggleInterface::run()
{
	switch (Config.design)
	{
		case NCM_DESIGN_CLASSIC:
			Config.design = NCM_DESIGN_ALTERNATIVE;
			Config.statusbar_visibility = false;
			break;
		case NCM_DESIGN_ALTERNATIVE:
			Config.design = NCM_DESIGN_CLASSIC;
			Config.statusbar_visibility =
			    ui_state_statusbar_visibility_baseline();
			break;
	}
	ncmpcpp_legacy_resize_screen(false);
	// unlock progressbar
	Progressbar::ScopedLock();
	ncm_status_changes_mixer();
	ncm_status_changes_elapsed_time(false);
	Statusbar::printf("User interface: %1%", Config.design);
}

bool JumpToParentDirectory::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_JUMP_TO_PARENT_DIRECTORY);
}

void JumpToParentDirectory::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_JUMP_TO_PARENT_DIRECTORY);
}

bool PreviousColumn::canBeRun()
{
	if (native_c_screen_media_library_is_current())
		return native_media_library_screen_previous_column_available(
		    native_c_screen_media_library());

	m_hc = dynamic_cast<HasColumns *>(screenLegacyCurrent());
	return m_hc != nullptr
		&& m_hc->previousColumnAvailable();
}

void PreviousColumn::run()
{
	if (native_c_screen_media_library_is_current())
	{
		native_media_library_screen_previous_column(
		    native_c_screen_media_library());
		return;
	}
	m_hc->previousColumn();
}

bool NextColumn::canBeRun()
{
	if (native_c_screen_media_library_is_current())
		return native_media_library_screen_next_column_available(
		    native_c_screen_media_library());

	m_hc = dynamic_cast<HasColumns *>(screenLegacyCurrent());
	return m_hc != nullptr
		&& m_hc->nextColumnAvailable();
}

void NextColumn::run()
{
	if (native_c_screen_media_library_is_current())
	{
		native_media_library_screen_next_column(
		    native_c_screen_media_library());
		return;
	}
	m_hc->nextColumn();
}

bool MasterScreen::canBeRun()
{
	return app_controller_can_show_locked_screen();
}

void MasterScreen::run()
{
	if (app_controller_show_locked_screen())
	{
		drawHeader();
	}
}

bool SlaveScreen::canBeRun()
{
	return app_controller_can_show_inactive_screen();
}

void SlaveScreen::run()
{
	if (app_controller_show_inactive_screen())
	{
		drawHeader();
	}
}

bool VolumeUp::canBeRun()
{
	return ncm_status_state_volume() >= 0;
}

void VolumeUp::run()
{
	Mpd.ChangeVolume(static_cast<int>(Config.volume_change_step));
}

bool VolumeDown::canBeRun()
{
	return ncm_status_state_volume() >= 0;
}

void VolumeDown::run()
{
	Mpd.ChangeVolume(-static_cast<int>(Config.volume_change_step));
}

bool AddItemToPlaylist::canBeRun()
{
	if (native_c_screen_media_library_is_current())
		return nativeMediaLibraryItemAvailable();

	m_hs = dynamic_cast<HasSongs *>(screenLegacyCurrent());
	return m_hs != nullptr && m_hs->itemAvailable();
}

void AddItemToPlaylist::run()
{
	bool success;

	if (native_c_screen_media_library_is_current())
		success = addNativeMediaLibraryItemToPlaylist(false);
	else
		success = m_hs->addItemToPlaylist(false);

	if (success)
	{
		app_controller_scroll_current_screen(NC_SCROLL_DOWN);
		listsChangeFinisher();
	}
}

bool PlayItem::canBeRun()
{
	if (native_c_screen_media_library_is_current())
		return nativeMediaLibraryItemAvailable();

	m_hs = dynamic_cast<HasSongs *>(screenLegacyCurrent());
	return m_hs != nullptr && m_hs->itemAvailable();
}

void PlayItem::run()
{
	bool success;

	if (native_c_screen_media_library_is_current())
		success = addNativeMediaLibraryItemToPlaylist(true);
	else
		success = m_hs->addItemToPlaylist(true);

	if (success)
		listsChangeFinisher();
}

bool DeletePlaylistItems::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_DELETE_PLAYLIST_ITEMS);
}

void DeletePlaylistItems::run()
{
	Statusbar::print("Deleting items...");
	if (ncm_action_delete_playlist_items())
		Statusbar::print("Item(s) deleted");
}

bool DeleteBrowserItems::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_DELETE_BROWSER_ITEMS);
}

void DeleteBrowserItems::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_DELETE_BROWSER_ITEMS);
}

bool DeleteStoredPlaylist::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr,
	                                  NCM_ACTION_DELETE_STORED_PLAYLIST);
}

void DeleteStoredPlaylist::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_DELETE_STORED_PLAYLIST);
}

void SavePlaylist::run()
{

	std::string playlist_name;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Save playlist as: ";
		if (!promptString(playlist_name))
				return;
	}
	try
	{
		Mpd.SavePlaylist(playlist_name);
		Statusbar::printf("Playlist saved as \"%1%\"", playlist_name);
	}
	catch (MPD::ServerError &e)
	{
		if (e.code() == MPD_SERVER_ERROR_EXIST)
		{
			if (!confirmAction(
				stringFormat("Playlist \"%1%\" already exists, overwrite?", playlist_name)
			))
				return;
			Mpd.DeletePlaylist(playlist_name);
			Mpd.SavePlaylist(playlist_name);
			Statusbar::print("Playlist overwritten");
		}
		else
		{
			Statusbar::printf("Error while saving playlist: %1%", e.what());
			return;
		}
	}
}

bool MoveSelectedItemsUp::canBeRun()
{
	if (native_c_screen_playlist_is_current())
		return ncm_action_runtime_can_run(
		    nullptr, NCM_ACTION_MOVE_SELECTED_ITEMS_UP);
	return false;
}

void MoveSelectedItemsUp::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_MOVE_SELECTED_ITEMS_UP);
}

bool MoveSelectedItemsDown::canBeRun()
{
	if (native_c_screen_playlist_is_current())
		return ncm_action_runtime_can_run(
		    nullptr, NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN);
	return false;
}

void MoveSelectedItemsDown::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN);
}

bool MoveSelectedItemsTo::canBeRun()
{
	if (native_c_screen_playlist_is_current())
		return ncm_action_runtime_can_run(
		    nullptr, NCM_ACTION_MOVE_SELECTED_ITEMS_TO);
	return false;
}

void MoveSelectedItemsTo::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_MOVE_SELECTED_ITEMS_TO);
}

bool Add::canBeRun()
{
	return true;
}

void Add::run()
{

	std::string path;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Add: ";
		if (!promptString(path))
				return;
	}

	// confirm when one wants to add the whole database
	if (path.empty())
		if (!confirmAction("Are you sure you want to add the whole database?"))
			return;

	Statusbar::put() << "Adding...";
	nc_window_refresh(ui_state_footer_window());
	try
	{
		Mpd.Add(path);
	}
	catch (MPD::ServerError &err)
	{
		// If a path is not a file or directory, assume it is a playlist.
		if (err.code() == MPD_SERVER_ERROR_NO_EXIST)
			Mpd.LoadPlaylist(path);
		else
		{
			Statusbar::printf("Error while adding item: %1%", err.what());
			return;
		}
	}
}

bool Load::canBeRun()
{
	return true;
}

void Load::run()
{

	std::string path;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Load playlist: ";
		if (!promptString(path))
				return;
	}

	Statusbar::put() << "Loading...";
	nc_window_refresh(ui_state_footer_window());
	Mpd.LoadPlaylist(path);
}

bool ToggleDisplayMode::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_TOGGLE_DISPLAY_MODE);
}

void ToggleDisplayMode::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_TOGGLE_DISPLAY_MODE);
}

bool ToggleSeparatorsBetweenAlbums::canBeRun()
{
	return true;
}

void ToggleSeparatorsBetweenAlbums::run()
{
	Config.playlist_separate_albums = !Config.playlist_separate_albums;
	Statusbar::printf("Separators between albums: %1%",
		Config.playlist_separate_albums ? "on" : "off"
	);
}

bool ToggleLyricsUpdateOnSongChange::canBeRun()
{
	return native_c_screen_lyrics_is_current();
}

void ToggleLyricsUpdateOnSongChange::run()
{
	Config.now_playing_lyrics = !Config.now_playing_lyrics;
	Statusbar::printf("Update lyrics if song changes: %1%",
		Config.now_playing_lyrics ? "on" : "off"
	);
}

void ToggleLyricsFetcher::run()
{
	nativeLyricsPrintFetcher();
}

void ToggleFetchingLyricsInBackground::run()
{
	Config.fetch_lyrics_in_background = !Config.fetch_lyrics_in_background;
	Statusbar::printf("Fetching lyrics for playing songs in background: %1%",
	                  Config.fetch_lyrics_in_background ? "on" : "off");
}

void TogglePlayingSongCentering::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING);
}

bool JumpToPlayingSong::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_JUMP_TO_PLAYING_SONG);
}

void JumpToPlayingSong::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_JUMP_TO_PLAYING_SONG);
}

bool Shuffle::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_SHUFFLE);
}

void Shuffle::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_SHUFFLE);
}

bool SaveTagChanges::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_SAVE_TAG_CHANGES);
}

void SaveTagChanges::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_SAVE_TAG_CHANGES);
}

void SetCrossfade::run()
{

	Statusbar::ScopedLock slock;
	Statusbar::put() << "Set crossfade to: ";
	std::string input;
	if (!promptString(input))
		return;
	auto crossfade = fromString<unsigned>(input);
	lowerBoundCheck(crossfade, 0u);
	Config.crossfade_time = crossfade;
	Mpd.SetCrossfade(crossfade);
}

bool SetVolume::canBeRun()
{
	return ncm_status_state_volume() >= 0;
}

void SetVolume::run()
{

	unsigned volume;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Set volume to: ";
		std::string input;
		if (!promptString(input))
			return;
		volume = fromString<unsigned>(input);
		boundsCheck(volume, 0u, 100u);
		Mpd.SetVolume(volume);
	}
	Statusbar::printf("Volume set to %1%%%", volume);
}

bool EnterDirectory::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_ENTER_DIRECTORY);
}

void EnterDirectory::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_ENTER_DIRECTORY);
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

bool EditDirectoryName::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_EDIT_DIRECTORY_NAME);
}

void EditDirectoryName::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_EDIT_DIRECTORY_NAME);
}

bool EditPlaylistName::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_EDIT_PLAYLIST_NAME);
}

void EditPlaylistName::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_EDIT_PLAYLIST_NAME);
}

bool EditLyrics::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_EDIT_LYRICS);
}

void EditLyrics::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_EDIT_LYRICS);
}

bool JumpToBrowserAction::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_JUMP_TO_BROWSER);
}

void JumpToBrowserAction::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_JUMP_TO_BROWSER);
}

bool JumpToPlaylistEditor::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR);
}

void JumpToPlaylistEditor::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR);
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

bool JumpToTagEditor::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_JUMP_TO_TAG_EDITOR);
}

void JumpToTagEditor::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_JUMP_TO_TAG_EDITOR);
}

bool JumpToPositionInSong::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_JUMP_TO_POSITION_IN_SONG);
}

void JumpToPositionInSong::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_JUMP_TO_POSITION_IN_SONG);
}

bool SelectItem::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	return m_list != nullptr
	    && !m_list->empty()
	    && m_list->currentP()->isSelectable();
}

void SelectItem::run()
{
	auto current = m_list->currentP();
	current->setSelected(!current->isSelected());
}

bool SelectRange::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	if (m_list == nullptr)
		return false;
	m_begin = m_list->beginP();
	m_end = m_list->endP();
	return findRange(m_begin, m_end);
}

void SelectRange::run()
{
	for (; m_begin != m_end; ++m_begin)
		m_begin->setSelected(true);
	Statusbar::print("Range selected");
}

bool ReverseSelection::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	return m_list != nullptr;
}

void ReverseSelection::run()
{
	for (auto &p : *m_list)
		p.setSelected(!p.isSelected());
	Statusbar::print("Selection reversed");
}

bool RemoveSelection::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	return m_list != nullptr;
}

void RemoveSelection::run()
{
	for (auto &p : *m_list)
		p.setSelected(false);
	Statusbar::print("Selection removed");
}

bool SelectAlbum::canBeRun()
{
	auto *w = screenLegacyCurrent()->activeWindow();
	if (m_list != static_cast<void *>(w))
		m_list = dynamic_cast<NC::List *>(w);
	if (m_songs != static_cast<void *>(w))
		m_songs = dynamic_cast<SongList *>(w);
	return m_list != nullptr && !m_list->empty()
	    && m_songs != nullptr;
}

void SelectAlbum::run()
{
	const auto front = m_songs->beginS(), current = m_songs->currentS(), end = m_songs->endS();
	if (current->song() == nullptr)
		return;
	enum NcmSongGetter getter = NCM_SONG_GETTER_ALBUM;
	const std::string tag = current->song()->getTags(getter);
	// go up
	for (auto it = current; it != front;)
	{
		--it;
		if (it->song() == nullptr || it->song()->getTags(getter) != tag)
			break;
		it->properties().setSelected(true);
	}
	// go down
	for (auto it = current;;)
	{
		it->properties().setSelected(true);
		if (++it == end)
			break;
		if (it->song() == nullptr || it->song()->getTags(getter) != tag)
			break;
	}
	Statusbar::print("Album around cursor position selected");
}

bool SelectFoundItems::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	if (m_list == nullptr || m_list->empty())
		return false;
	m_searchable = dynamic_cast<Searchable *>(screenLegacyCurrent());
	return m_searchable != nullptr && m_searchable->allowsSearching();
}

void SelectFoundItems::run()
{
	auto current_pos = m_list->choice();
	screenLegacyCurrent()->activeWindow()->scroll(NC_SCROLL_HOME);
	bool found = m_searchable->search(NCM_SEARCH_DIRECTION_FORWARD, false, false);
	if (found)
	{
		Statusbar::print("Searching for items...");
		m_list->currentP()->setSelected(true);
		while (m_searchable->search(NCM_SEARCH_DIRECTION_FORWARD, false, true))
			m_list->currentP()->setSelected(true);
		Statusbar::print("Found items selected");
	}
	m_list->highlight(current_pos);
}

void CropMainPlaylist::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_CROP_MAIN_PLAYLIST);
}

bool CropPlaylist::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_CROP_PLAYLIST);
}

void CropPlaylist::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_CROP_PLAYLIST);
}

void ClearMainPlaylist::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_CLEAR_MAIN_PLAYLIST);
}

bool ClearPlaylist::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_CLEAR_PLAYLIST);
}

void ClearPlaylist::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_CLEAR_PLAYLIST);
}

bool ReversePlaylist::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_REVERSE_PLAYLIST);
}

void ReversePlaylist::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_REVERSE_PLAYLIST);
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
	listsChangeFinisher();
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
	listsChangeFinisher();
}

void ToggleFindMode::run()
{
	Config.wrapped_search = !Config.wrapped_search;
	Statusbar::printf("Search mode: %1%",
		Config.wrapped_search ? "Wrapped" : "Normal"
	);
}

void ToggleReplayGainMode::run()
{

	char rgm = 0;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Replay gain mode? "
		<< "[" << NC_FORMAT_BOLD << 'o' << NC_FORMAT_NO_BOLD << "ff"
		<< "/" << NC_FORMAT_BOLD << 't' << NC_FORMAT_NO_BOLD << "rack"
		<< "/" << NC_FORMAT_BOLD << 'a' << NC_FORMAT_NO_BOLD << "lbum"
		<< "] ";
		char values[] = {'t', 'a', 'o'};
		if (!promptOneOf(values, static_cast<int32>(sizeof(values)), rgm))
			return;
	}
	switch (rgm)
	{
		case 't':
			Mpd.SetReplayGainMode(MPD::rgmTrack);
			break;
		case 'a':
			Mpd.SetReplayGainMode(MPD::rgmAlbum);
			break;
		case 'o':
			Mpd.SetReplayGainMode(MPD::rgmOff);
			break;
		default: // impossible
			Statusbar::printf("Internal error: invalid replay gain mode: %1%",
			                  rgm);
			return;
	}
	Statusbar::printf("Replay gain mode: %1%", Mpd.GetReplayGainMode());
}

void ToggleAddMode::run()
{
	std::string mode_desc;
	switch (Config.space_add_mode)
	{
		case NCM_SPACE_ADD_MODE_ADD_REMOVE:
			Config.space_add_mode = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
			mode_desc = "always add an item to playlist";
			break;
		case NCM_SPACE_ADD_MODE_ALWAYS_ADD:
			Config.space_add_mode = NCM_SPACE_ADD_MODE_ADD_REMOVE;
			mode_desc = "add an item to playlist or remove if already added";
			break;
	}
	Statusbar::printf("Add mode: %1%", mode_desc);
}

void ToggleMouse::run()
{
	Config.mouse_support = !Config.mouse_support;
	if (Config.mouse_support)
		nc_mouse_enable();
	else
		nc_mouse_disable();
	Statusbar::printf("Mouse support %1%",
		Config.mouse_support ? "enabled" : "disabled"
	);
}

void ToggleBitrateVisibility::run()
{
	Config.display_bitrate = !Config.display_bitrate;
	Statusbar::printf("Bitrate visibility %1%",
		Config.display_bitrate ? "enabled" : "disabled"
	);
}

void AddRandomItems::run()
{
		char rnd_type = 0;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Add random? "
		<< "[" << NC_FORMAT_BOLD << 's' << NC_FORMAT_NO_BOLD << "ongs"
		<< "/" << NC_FORMAT_BOLD << 'a' << NC_FORMAT_NO_BOLD << "rtists"
		<< "/" << "album" << NC_FORMAT_BOLD << 'A' << NC_FORMAT_NO_BOLD << "rtists"
		<< "/" << "al" << NC_FORMAT_BOLD << 'b' << NC_FORMAT_NO_BOLD << "ums"
		<< "] ";
		char values[] = {'s', 'a', 'A', 'b'};
		if (!promptOneOf(values, static_cast<int32>(sizeof(values)), rnd_type))
			return;
	}

	mpd_tag_type tag_type = MPD_TAG_ARTIST;
	std::string tag_type_str ;
	if (rnd_type != 's')
	{
		tag_type = ncm_char_to_tag_type(rnd_type);
		tag_type_str = lowercaseAscii(ncm_tag_type_name(tag_type));
	}
	else
		tag_type_str = "song";

	unsigned number;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Number of random " << tag_type_str << "s: ";
		std::string input;
		if (!promptString(input))
			return;
		number = fromString<unsigned>(input);
	}
	if (number > 0)
	{
		bool success;
		if (rnd_type == 's')
			success = Mpd.AddRandomSongs(number, Config.random_exclude_pattern,
			                          &global_random);
		else
			success = Mpd.AddRandomTag(tag_type, number, &global_random);
		if (success)
			Statusbar::printf("%1% random %2%%3% added to playlist", number, tag_type_str, number == 1 ? "" : "s");
	}
}

bool ToggleBrowserSortMode::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_TOGGLE_BROWSER_SORT_MODE);
}

void ToggleBrowserSortMode::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_TOGGLE_BROWSER_SORT_MODE);
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

bool FetchLyricsInBackground::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr,
	                                  NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND);
}

void FetchLyricsInBackground::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND);
}

bool RefetchLyrics::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_REFETCH_LYRICS);
}

void RefetchLyrics::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_REFETCH_LYRICS);
}

bool SetSelectedItemsPriority::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY);
}

void SetSelectedItemsPriority::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY);
}

bool ToggleOutput::canBeRun()
{
#ifdef ENABLE_OUTPUTS
	return native_c_screen_outputs_is_current();
#else
	return false;
#endif // ENABLE_OUTPUTS
}

void ToggleOutput::run()
{
#ifdef ENABLE_OUTPUTS
	native_c_screen_outputs_toggle();
#endif // ENABLE_OUTPUTS
}

bool ToggleVisualizationType::canBeRun()
{
#	ifdef ENABLE_VISUALIZER
	return native_c_screen_visualizer_is_current();
#	else
	return false;
#	endif // ENABLE_VISUALIZER
}

void ToggleVisualizationType::run()
{
	(void)ncm_action_toggle_visualization_type();
}

void ShowSongInfo::run()
{
	native_c_screen_song_info_switch_to();
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

bool ShowLyrics::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_SHOW_LYRICS);
}

void ShowLyrics::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_SHOW_LYRICS);
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

bool ShowHelp::canBeRun()
{
	return !native_c_screen_help_is_current()
#	ifdef HAVE_TAGLIB_H
	    && !native_c_screen_tiny_tag_editor_is_current()
#	endif // HAVE_TAGLIB_H
	;
}

void ShowHelp::run()
{
	native_c_screen_help_switch_to();
}

bool ShowPlaylist::canBeRun()
{
	return !native_c_screen_playlist_is_current()
#	ifdef HAVE_TAGLIB_H
	    && !native_c_screen_tiny_tag_editor_is_current()
#	endif // HAVE_TAGLIB_H
	;
}

void ShowPlaylist::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_SHOW_PLAYLIST);
}

bool ShowBrowserAction::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_SHOW_BROWSER);
}

void ShowBrowserAction::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_SHOW_BROWSER);
}

bool ChangeBrowseMode::canBeRun()
{
	return ncm_action_runtime_can_run(
	    nullptr, NCM_ACTION_CHANGE_BROWSE_MODE);
}

void ChangeBrowseMode::run()
{
	(void)ncm_action_runtime_run(
	    nullptr, NCM_ACTION_CHANGE_BROWSE_MODE);
}

bool ShowPlaylistEditor::canBeRun()
{
	return !native_c_screen_playlist_editor_is_current()
#	ifdef HAVE_TAGLIB_H
	    && !native_c_screen_tiny_tag_editor_is_current()
#	endif // HAVE_TAGLIB_H
	;
}

void ShowPlaylistEditor::run()
{
	native_c_screen_playlist_editor_register();
	native_c_screen_playlist_editor_switch_to();
}

bool ShowTagEditor::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_SHOW_TAG_EDITOR);
}

void ShowTagEditor::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_SHOW_TAG_EDITOR);
}

bool ShowOutputs::canBeRun()
{
#	ifdef ENABLE_OUTPUTS
	return !native_c_screen_outputs_is_current()
#	ifdef HAVE_TAGLIB_H
	    && !native_c_screen_tiny_tag_editor_is_current()
#	endif // HAVE_TAGLIB_H
	;
#	else
	return false;
#	endif // ENABLE_OUTPUTS
}

void ShowOutputs::run()
{
#	ifdef ENABLE_OUTPUTS
	native_c_screen_outputs_switch_to();
#	endif // ENABLE_OUTPUTS
}

bool ShowVisualizer::canBeRun()
{
#	ifdef ENABLE_VISUALIZER
	return !native_c_screen_visualizer_is_current()
#	ifdef HAVE_TAGLIB_H
	    && !native_c_screen_tiny_tag_editor_is_current()
#	endif // HAVE_TAGLIB_H
	;
#	else
	return false;
#	endif // ENABLE_VISUALIZER
}

void ShowVisualizer::run()
{
	(void)ncm_action_show_visualizer();
}

#ifdef HAVE_TAGLIB_H
bool ShowServerInfo::canBeRun()
{
	return !native_c_screen_tiny_tag_editor_is_current();
}
#endif // HAVE_TAGLIB_H

void ShowServerInfo::run()
{
	native_c_screen_server_info_switch_to();
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
	insert_action(new Actions::ScrollUp());
	insert_action(new Actions::ScrollDown());
	insert_action(new Actions::ScrollUpArtist());
	insert_action(new Actions::ScrollUpAlbum());
	insert_action(new Actions::ScrollDownArtist());
	insert_action(new Actions::ScrollDownAlbum());
	insert_action(new Actions::PageUp());
	insert_action(new Actions::PageDown());
	insert_action(new Actions::MoveHome());
	insert_action(new Actions::MoveEnd());
	insert_action(new Actions::ToggleInterface());
	insert_action(new Actions::JumpToParentDirectory());
	insert_action(new Actions::SelectItem());
	insert_action(new Actions::SelectRange());
	insert_action(new Actions::PreviousColumn());
	insert_action(new Actions::NextColumn());
	insert_action(new Actions::MasterScreen());
	insert_action(new Actions::SlaveScreen());
	insert_action(new Actions::VolumeUp());
	insert_action(new Actions::VolumeDown());
	insert_action(new Actions::AddItemToPlaylist());
	insert_action(new Actions::DeletePlaylistItems());
	insert_action(new Actions::DeleteStoredPlaylist());
	insert_action(new Actions::DeleteBrowserItems());
	insert_action(new Actions::SavePlaylist());
	insert_action(new Actions::MoveSelectedItemsUp());
	insert_action(new Actions::MoveSelectedItemsDown());
	insert_action(new Actions::MoveSelectedItemsTo());
	insert_action(new Actions::Add());
	insert_action(new Actions::Load());
	insert_action(new Actions::PlayItem());
	insert_action(new Actions::ToggleDisplayMode());
	insert_action(new Actions::ToggleSeparatorsBetweenAlbums());
	insert_action(new Actions::ToggleLyricsUpdateOnSongChange());
	insert_action(new Actions::ToggleLyricsFetcher());
	insert_action(new Actions::ToggleFetchingLyricsInBackground());
	insert_action(new Actions::TogglePlayingSongCentering());
	insert_action(new Actions::JumpToPlayingSong());
	insert_action(new Actions::Shuffle());
	insert_action(new Actions::SaveTagChanges());
	insert_action(new Actions::SetCrossfade());
	insert_action(new Actions::SetVolume());
	insert_action(new Actions::EnterDirectory());
	insert_action(new Actions::EditSong());
	insert_action(new Actions::EditLibraryTag());
	insert_action(new Actions::EditLibraryAlbum());
	insert_action(new Actions::EditDirectoryName());
	insert_action(new Actions::EditPlaylistName());
	insert_action(new Actions::EditLyrics());
	insert_action(new Actions::JumpToBrowserAction());
	insert_action(new Actions::JumpToPlaylistEditor());
	insert_action(new Actions::ToggleScreenLock());
	insert_action(new Actions::JumpToTagEditor());
	insert_action(new Actions::JumpToPositionInSong());
	insert_action(new Actions::ReverseSelection());
	insert_action(new Actions::RemoveSelection());
	insert_action(new Actions::SelectAlbum());
	insert_action(new Actions::SelectFoundItems());
	insert_action(new Actions::CropMainPlaylist());
	insert_action(new Actions::CropPlaylist());
	insert_action(new Actions::ClearMainPlaylist());
	insert_action(new Actions::ClearPlaylist());
	insert_action(new Actions::ReversePlaylist());
	insert_action(new Actions::Find());
	insert_action(new Actions::NextFoundItem());
	insert_action(new Actions::PreviousFoundItem());
	insert_action(new Actions::ToggleFindMode());
	insert_action(new Actions::ToggleReplayGainMode());
	insert_action(new Actions::ToggleAddMode());
	insert_action(new Actions::ToggleMouse());
	insert_action(new Actions::ToggleBitrateVisibility());
	insert_action(new Actions::AddRandomItems());
	insert_action(new Actions::ToggleBrowserSortMode());
	insert_action(new Actions::ToggleLibraryTagType());
	insert_action(new Actions::FetchLyricsInBackground());
	insert_action(new Actions::RefetchLyrics());
	insert_action(new Actions::SetSelectedItemsPriority());
	insert_action(new Actions::ToggleOutput());
	insert_action(new Actions::ToggleVisualizationType());
	insert_action(new Actions::ShowSongInfo());
	insert_action(new Actions::ShowArtistInfo());
	insert_action(new Actions::ShowLyrics());
	insert_action(new Actions::NextScreen());
	insert_action(new Actions::PreviousScreen());
	insert_action(new Actions::ShowHelp());
	insert_action(new Actions::ShowPlaylist());
	insert_action(new Actions::ShowBrowserAction());
	insert_action(new Actions::ChangeBrowseMode());
	insert_action(new Actions::ShowPlaylistEditor());
	insert_action(new Actions::ShowTagEditor());
	insert_action(new Actions::ShowOutputs());
	insert_action(new Actions::ShowVisualizer());
	insert_action(new Actions::ShowServerInfo());
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

bool scrollTagCanBeRun(NC::List *&list, const SongList *&songs)
{
	auto w = screenLegacyCurrent()->activeWindow();
	if (list != static_cast<void *>(w))
		list = dynamic_cast<NC::List *>(w);
	if (songs != static_cast<void *>(w))
		songs = dynamic_cast<SongList *>(w);
	return list != nullptr && !list->empty()
	    && songs != nullptr;
}

void scrollTagUpRun(NC::List *list, const SongList *songs, enum NcmSongGetter getter)
{
	const auto front = songs->beginS();
	auto it = songs->currentS();
	if (it->song() != nullptr)
	{
		const std::string tag = it->song()->getTags(getter);
		while (it != front)
		{
			--it;
			if (it->song() == nullptr || it->song()->getTags(getter) != tag)
				break;
		}
		list->highlight(it-front);
	}
}

void scrollTagDownRun(NC::List *list, const SongList *songs, enum NcmSongGetter getter)
{
	const auto front = songs->beginS(), back = --songs->endS();
	auto it = songs->currentS();
	if (it->song() != nullptr)
	{
		const std::string tag = it->song()->getTags(getter);
		while (it != back)
		{
			++it;
			if (it->song() == nullptr || it->song()->getTags(getter) != tag)
				break;
		}
		list->highlight(it-front);
	}
}

void listsChangeFinisher()
{
	if (native_c_screen_media_library_is_current())
	{
		native_media_library_screen_finish_list_change(
		    native_c_screen_media_library());
		return;
	}

	if (native_c_screen_playlist_editor_is_current())
		return;

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

bool actions_legacy_runtime_execute_binding(NcmBinding *binding)
{
	try
	{
		return bindingsLegacyExecute(binding);
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
