#include <cassert>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <regex>
#include <string>

#include "actions_legacy.h"
#include "actions_legacy_runtime.h"
#include "app_binding_migration.h"
#include "app_legacy_bridge.h"
#include "charset.h"
#include "config.h"
#include "display.h"
#include "app_controller.h"
#include "global.h"
#include "ui_state.h"
#include "screens/screen_cpp_legacy.h"
#include "mpdpp.h"
#include "helpers_legacy.h"
#include "statusbar.h"
#include "status.h"
#include "utility/comparators.h"
#include "utility/conversion.h"

#include "cbase/base_macros.h"
#include "curses/menu_impl.h"
#include "macro_utilities.h"
#include "screens/browser.h"
#include "screens/native_c_screens.h"
#include "screens/playlist.h"
#include "screens/playlist_editor.h"
#include "utility/readline.h"
#include "utility/string.h"
#include "utility/string_format.h"
#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "c/ncm_type_conversions.h"
#include "screens/tag_editor.h"
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

bool locateNativeMediaLibrarySong(MPD::Song &song, bool switch_to)
{
	NcmError error;
	bool success;

	if (switch_to)
	{
		native_c_screen_media_library_register();
		native_c_screen_media_library_switch_to();
	}
	Statusbar::put() << "Jumping to song...";
	static_cast<NC::Window *>(ui_state_footer_legacy_window())->refresh();
	ncm_error_clear(&error);
	success = native_media_library_screen_locate_song(
	    native_c_screen_media_library(), song.cSong(), &error);
	if (!success && ncm_error_is_set(&error))
		Statusbar::print(error.message);
	return success;
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


void nativeLyricsPrintConsumerMessage()
{
	NcmBuffer message;

	native_lyrics_screen_dispatch_jobs(native_c_screen_lyrics());
	ncm_buffer_init(&message);
	if (native_lyrics_screen_try_take_consumer_message(
	        native_c_screen_lyrics(), &message))
		Statusbar::print(std::string(message.data, message.len));
	ncm_buffer_destroy(&message);
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

void requestMediaLibraryDatabaseUpdate(void *)
{
	native_media_library_screen_request_database_update(
	    native_c_screen_media_library());
}

void refreshPlaylistRelatedInactiveColumns(void *)
{
	if (app_controller_is_screen_visible(
	        native_c_screen_media_library_native()))
		native_media_library_screen_refresh_inactive_songs(
		    native_c_screen_media_library());

	if (myPlaylistEditor != nullptr
	    && isVisible(myPlaylistEditor)
	    && !myPlaylistEditor->isActiveWindow(myPlaylistEditor->Content))
		myPlaylistEditor->Content.refresh();
}

bool highlightPlaylistMpdPosition(int32 position)
{
	if (myPlaylist == nullptr || position < 0)
		return false;

	auto &menu = myPlaylist->main();
	if (!menu.isFiltered())
	{
		auto first = menu.begin();
		auto last = menu.end();
		auto it = std::find_if(first, last, [position](const auto &item) {
			return item.value().getPosition() == position;
		});
		if (it == last)
			return false;
		menu.highlight(it - first);
		return true;
	}

	auto first = menu.beginV();
	auto last = menu.endV();
	auto it = std::find_if(first, last, [position](const auto &song) {
		return song.getPosition() == position;
	});
	if (it == last)
	{
		Statusbar::print("Song is filtered out");
		return false;
	}
	menu.highlight(it - first);
	return true;
}

void traceStatus(bool update_timer, bool update_window_timeout)
{
	NcmError error = {};

	ncm_status_set_database_update_observer(
	    requestMediaLibraryDatabaseUpdate, nullptr);
	ncm_status_set_playlist_update_observer(
	    refreshPlaylistRelatedInactiveColumns, nullptr);
	ncm_error_clear(&error);
	ncm_status_trace(&global_mpd, update_timer, update_window_timeout, &error);
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

std::string storedPlaylistPath(const MPD::Playlist &playlist)
{
	return playlist.path();
}

std::string currentStoredPlaylistPath()
{
	return storedPlaylistPath(myPlaylistEditor->Playlists.current()->value());
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
	static_cast<NC::Window *>(ui_state_footer_legacy_window())->pushChar(key);
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
bool findSelectedRangeAndPrintInfoIfNot(Iterator &first, Iterator &last)
{
	bool success = findSelectedRange(first, last);
	if (!success)
		Statusbar::print("No range selected");
	return success;
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

void initializeScreens()
{
	app_controller_init();
	native_c_screens_init_all();

	myPlaylist = new Playlist;
	myBrowser = new Browser;
	ncm_status_set_database_update_observer(
	    requestMediaLibraryDatabaseUpdate, nullptr);
	myPlaylistEditor = new PlaylistEditor;
	ncm_status_set_playlist_update_observer(
	    refreshPlaylistRelatedInactiveColumns, nullptr);
#	ifdef HAVE_TAGLIB_H
	myTagEditor = new TagEditor;
#	endif // HAVE_TAGLIB_H

	native_c_screens_register_native_only();
	myPlaylist->registerNativeScreen();
	myBrowser->registerNativeScreen();
	myPlaylistEditor->registerNativeScreen();
	native_c_screen_lyrics_register();

#	ifdef HAVE_TAGLIB_H
	myTagEditor->registerNativeScreen();
#	endif // HAVE_TAGLIB_H
}

void setResizeFlags()
{
	native_c_screens_request_registered_resize();
	myPlaylist->hasToBeResized = 1;
	myBrowser->hasToBeResized = 1;
	myPlaylistEditor->hasToBeResized = 1;
	native_c_screen_lyrics_set_resize();

#	ifdef HAVE_TAGLIB_H
	myTagEditor->hasToBeResized = 1;
#	endif // HAVE_TAGLIB_H
}

void resizeScreen(bool reload_main_window)
{
	// update internal screen dimensions
	if (reload_main_window)
	{
		rl_resize_terminal();
		endwin();
		refresh();
		// Remove KEY_RESIZE from input queue, I'm not sure how these make it in.
		getch();
	}

	ncmpcpp_legacy_set_windows_dimensions();

	setResizeFlags();

	app_controller_resize_visible_screens();

	if (Config.header_visibility || Config.design == NCM_DESIGN_ALTERNATIVE)
	{
		static_cast<NC::Window *>(ui_state_header_legacy_window())->resize(
		    COLS, static_cast<size_t>(ncmpcpp_legacy_header_height()));
	}

	static_cast<NC::Window *>(ui_state_footer_legacy_window())->moveTo(
	    0, static_cast<size_t>(ncmpcpp_legacy_footer_start_y()));
	static_cast<NC::Window *>(ui_state_footer_legacy_window())->resize(
	    COLS, static_cast<size_t>(ncmpcpp_legacy_footer_height()));

	app_controller_refresh_visible_screens();

	ncm_status_changes_elapsed_time(false);
	ncm_status_changes_player_state();
	// Note: routines for drawing separator if alternative user
	// interface is active and header is hidden are placed in
	// NcmpcppStatusChanges.StatusFlags
	ncm_status_changes_flags();
	drawHeader();
	static_cast<NC::Window *>(ui_state_footer_legacy_window())->refresh();
	refresh();
}

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

UpdateEnvironment::UpdateEnvironment()
: BaseAction(Type::UpdateEnvironment)
, m_past()
{ }

void UpdateEnvironment::run(bool update_timer, bool refresh_window, bool mpd_sync)
{

	// update timer, status if necessary etc.
	traceStatus(update_timer, true);

	// show lyrics consumer notification if appropriate
	nativeLyricsPrintConsumerMessage();

	// header stuff
	if ((screenLegacyCurrent() == myPlaylist
	     || screenLegacyCurrent() == myBrowser
	     || native_c_screen_lyrics_is_current())
	&&  (global_timer_elapsed_ms(m_past) > 500)
	)
	{
		drawHeader();
		m_past = global_timer;
	}

	if (refresh_window)
		app_controller_refresh_current_window();

	// We want to synchronize with MPD during execution of an action chain.
	if (mpd_sync)
	{
		NcmError error;

		ncm_error_clear(&error);
		(void)ncm_status_update_from_noidle(&global_mpd, nullptr, &error);
	}
}

void UpdateEnvironment::run()
{
	run(true, true, true);
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
	ncmpcpp_legacy_set_windows_dimensions();
	resizeScreen(false);
	// unlock progressbar
	Progressbar::ScopedLock();
	ncm_status_changes_mixer();
	ncm_status_changes_elapsed_time(false);
	Statusbar::printf("User interface: %1%", Config.design);
}

bool JumpToParentDirectory::canBeRun()
{
	BaseScreen *current;

	current = screenLegacyCurrent();
	if (current == nullptr)
		return false;
	return (current == myBrowser)
#	ifdef HAVE_TAGLIB_H
	    || (current->activeWindow() == myTagEditor->Dirs)
#	endif // HAVE_TAGLIB_H
	;
}

void JumpToParentDirectory::run()
{
	if (screenLegacyCurrent() == myBrowser)
	{
		if (!myBrowser->inRootDirectory())
		{
			myBrowser->main().reset();
			myBrowser->enterDirectory();
		}
	}
#	ifdef HAVE_TAGLIB_H
	else if (screenLegacyCurrent() == myTagEditor)
	{
		if (myTagEditor->CurrentDir() != "/")
		{
			myTagEditor->Dirs->reset();
			myTagEditor->enterDirectory();
		}
	}
#	endif // HAVE_TAGLIB_H
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
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_DELETE_PLAYLIST_ITEMS)
	    || (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content)
	        && !myPlaylistEditor->Content.empty());
}

void DeletePlaylistItems::run()
{
	if (ncm_action_runtime_can_run(nullptr, NCM_ACTION_DELETE_PLAYLIST_ITEMS))
	{
		Statusbar::print("Deleting items...");
		if (ncm_action_delete_playlist_items())
			Statusbar::print("Item(s) deleted");
	}
	else if (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content))
	{
		std::string playlist = currentStoredPlaylistPath();
		auto delete_fun = [playlist](auto &, unsigned pos) {
			Mpd.PlaylistDelete(playlist, pos);
		};
		Statusbar::print("Deleting items...");
		deleteSelectedSongs(myPlaylistEditor->Content, delete_fun);
		Statusbar::print("Item(s) deleted");
	}
}

bool DeleteBrowserItems::canBeRun()
{
	auto check_if_deletion_allowed = []() {
		if (Config.allow_for_physical_item_deletion)
			return true;
		else
		{
			Statusbar::print("Flag \"allow_for_physical_item_deletion\" needs to be enabled in configuration file");
			return false;
		}
	};
	return screenLegacyCurrent() == myBrowser
	    && !myBrowser->main().empty()
	    && isMPDMusicDirSet()
	    && check_if_deletion_allowed();
}

void DeleteBrowserItems::run()
{
	auto get_name = [](const MPD::Item &item) -> std::string {
		std::string iname;
		switch (item.type())
		{
			case MPD::Item::Type::Directory:
				iname = getBasename(item.directory().path());
				break;
			case MPD::Item::Type::Song:
				iname = item.song().getName();
				break;
			case MPD::Item::Type::Playlist:
				iname = getBasename(item.playlist().path());
				break;
		}
		return iname;
	};

	std::string question;
	if (hasSelected(myBrowser->main().begin(), myBrowser->main().end()))
		question = "Delete selected items?";
	else
	{
		const auto &item = myBrowser->main().current()->value();
		// parent directories are not accepted (and they
		// can't be selected, so in other cases it's fine).
		if (myBrowser->isParentDirectory(item))
			return;
		const char msg[] = "Delete \"%1%\"?";
		question = stringFormat(msg,
			Utf8::shorten(get_name(item), COLS-const_strlen(msg)-5)
		);
	}
	if (!confirmAction(question))
		return;

	auto items = getSelectedOrCurrent(
		myBrowser->main().begin(),
		myBrowser->main().end(),
		myBrowser->main().current()
	);
	for (const auto &item : items)
	{
		myBrowser->remove(item->value());
		const char msg[] = "Deleted %1% \"%2%\"";
		Statusbar::printf(msg,
			itemTypeName(item->value().type()),
			Utf8::shorten(get_name(item->value()), COLS-const_strlen(msg))
		);
	}

	if (!myBrowser->isLocal())
		Mpd.UpdateDirectory(myBrowser->currentDirectory());
	myBrowser->requestUpdate();
}

bool DeleteStoredPlaylist::canBeRun()
{
	return screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Playlists);
}

void DeleteStoredPlaylist::run()
{
	if (myPlaylistEditor->Playlists.empty())
		return;
	std::string question;
	if (hasSelected(myPlaylistEditor->Playlists.begin(), myPlaylistEditor->Playlists.end()))
		question = "Delete selected playlists?";
	else
	{
		const char msg[] = "Delete playlist \"%1%\"?";
		question = stringFormat(msg,
			Utf8::shorten(currentStoredPlaylistPath(),
			            COLS-const_strlen(msg)-10));
	}
	if (!confirmAction(question))
		return;

	auto list = getSelectedOrCurrent(
		myPlaylistEditor->Playlists.begin(),
		myPlaylistEditor->Playlists.end(),
		myPlaylistEditor->Playlists.current()
	);
	for (const auto &item : list)
		Mpd.DeletePlaylist(storedPlaylistPath(item->value()));
	Statusbar::printf("%1% deleted", list.size() == 1 ? "Playlist" : "Playlists");
	// force playlists update. this happens automatically, but only after call
	// to Key::read, therefore when we call PlaylistEditor::Update, it won't
	// yet see it, so let's point that it needs to update it.
	myPlaylistEditor->requestPlaylistsUpdate();
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
	return ((screenLegacyCurrent() == myPlaylist
	    &&  !myPlaylist->main().empty())
	 ||    (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content)
	    &&  !myPlaylistEditor->Content.empty()));
}

void MoveSelectedItemsUp::run()
{
	const char *filteredMsg = "Moving items up is disabled in filtered playlist";
	if (screenLegacyCurrent() == myPlaylist)
	{
		if (myPlaylist->main().isFiltered())
			Statusbar::print(filteredMsg);
		else
			moveSelectedItemsUp(
				myPlaylist->main(),
				[](auto &, unsigned from, unsigned to) { Mpd.Move(from, to); });
	}
	else if (screenLegacyCurrent() == myPlaylistEditor)
	{
		if (myPlaylistEditor->Content.isFiltered())
			Statusbar::print(filteredMsg);
		else
		{
			auto playlist = currentStoredPlaylistPath();
			moveSelectedItemsUp(
				myPlaylistEditor->Content,
				[playlist](auto &, unsigned from, unsigned to) {
					Mpd.PlaylistMove(playlist, from, to);
				});
		}
	}
}

bool MoveSelectedItemsDown::canBeRun()
{
	return ((screenLegacyCurrent() == myPlaylist
	    &&  !myPlaylist->main().empty())
	 ||    (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content)
	    &&  !myPlaylistEditor->Content.empty()));
}

void MoveSelectedItemsDown::run()
{
	const char *filteredMsg = "Moving items down is disabled in filtered playlist";
	if (screenLegacyCurrent() == myPlaylist)
	{
		if (myPlaylist->main().isFiltered())
			Statusbar::print(filteredMsg);
		else
			moveSelectedItemsDown(
				myPlaylist->main(),
				[](auto &, unsigned from, unsigned to) { Mpd.Move(from, to); });
	}
	else if (screenLegacyCurrent() == myPlaylistEditor)
	{
		if (myPlaylistEditor->Content.isFiltered())
			Statusbar::print(filteredMsg);
		else
		{
			auto playlist = currentStoredPlaylistPath();
			moveSelectedItemsDown(
				myPlaylistEditor->Content,
				[playlist](auto &, unsigned from, unsigned to) {
					Mpd.PlaylistMove(playlist, from, to);
				});
		}
	}
}

bool MoveSelectedItemsTo::canBeRun()
{
	return screenLegacyCurrent() == myPlaylist
	    || screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content);
}

void MoveSelectedItemsTo::run()
{
	if (screenLegacyCurrent() == myPlaylist)
	{
		if (!myPlaylist->main().empty())
			moveSelectedItemsTo(myPlaylist->main(), [](auto &, unsigned from, unsigned to) { Mpd.Move(from, to); });
	}
	else
	{
		assert(!myPlaylistEditor->Playlists.empty());
		std::string playlist = currentStoredPlaylistPath();
		auto move_fun = [playlist](auto &, unsigned from, unsigned to) {
			Mpd.PlaylistMove(playlist, from, to);
		};
		moveSelectedItemsTo(myPlaylistEditor->Content, move_fun);
	}
}

bool Add::canBeRun()
{
	return screenLegacyCurrent() != myPlaylistEditor
	   || !myPlaylistEditor->Playlists.empty();
}

void Add::run()
{

	std::string path;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << (screenLegacyCurrent() == myPlaylistEditor ? "Add to playlist: " : "Add: ");
		if (!promptString(path))
				return;
	}

	// confirm when one wants to add the whole database
	if (path.empty())
		if (!confirmAction("Are you sure you want to add the whole database?"))
			return;

	Statusbar::put() << "Adding...";
	static_cast<NC::Window *>(ui_state_footer_legacy_window())->refresh();
	if (screenLegacyCurrent() == myPlaylistEditor)
		Mpd.AddToPlaylist(currentStoredPlaylistPath(), path);
	else
	{
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
}

bool Load::canBeRun()
{
	return screenLegacyCurrent() != myPlaylistEditor;
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
	static_cast<NC::Window *>(ui_state_footer_legacy_window())->refresh();
	Mpd.LoadPlaylist(path);
}

bool ToggleDisplayMode::canBeRun()
{
	return screenLegacyCurrent() == myPlaylist
	    || screenLegacyCurrent() == myBrowser
	    || screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content);
}

void ToggleDisplayMode::run()
{
	if (screenLegacyCurrent() == myPlaylist)
	{
		switch (Config.playlist_display_mode)
		{
			case NCM_DISPLAY_MODE_CLASSIC:
				Config.playlist_display_mode = NCM_DISPLAY_MODE_COLUMNS;
				myPlaylist->main().setItemDisplayer(std::bind(
					Display::SongsInColumns, ph::_1, std::cref(myPlaylist->main())
				));
				if (Config.titles_visibility)
					myPlaylist->main().setTitle(Display::Columns(myPlaylist->main().getWidth()));
				else
					myPlaylist->main().setTitle("");
				break;
			case NCM_DISPLAY_MODE_COLUMNS:
				Config.playlist_display_mode = NCM_DISPLAY_MODE_CLASSIC;
				myPlaylist->main().setItemDisplayer(std::bind(
					Display::Songs, ph::_1, std::cref(myPlaylist->main()), std::cref(Config.song_list_format)
				));
				myPlaylist->main().setTitle("");
		}
		Statusbar::printf("Playlist display mode: %1%", Config.playlist_display_mode);
	}
	else if (screenLegacyCurrent() == myBrowser)
	{
		switch (Config.browser_display_mode)
		{
			case NCM_DISPLAY_MODE_CLASSIC:
				Config.browser_display_mode = NCM_DISPLAY_MODE_COLUMNS;
				if (Config.titles_visibility)
					myBrowser->main().setTitle(Display::Columns(myBrowser->main().getWidth()));
				else
					myBrowser->main().setTitle("");
				break;
			case NCM_DISPLAY_MODE_COLUMNS:
				Config.browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
				myBrowser->main().setTitle("");
				break;
		}
		Statusbar::printf("Browser display mode: %1%", Config.browser_display_mode);
	}
	else if (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content))
	{
		switch (Config.playlist_editor_display_mode)
		{
			case NCM_DISPLAY_MODE_CLASSIC:
				Config.playlist_editor_display_mode = NCM_DISPLAY_MODE_COLUMNS;
				myPlaylistEditor->Content.setItemDisplayer(std::bind(
					Display::SongsInColumns, ph::_1, std::cref(myPlaylistEditor->Content)
				));
				break;
			case NCM_DISPLAY_MODE_COLUMNS:
				Config.playlist_editor_display_mode = NCM_DISPLAY_MODE_CLASSIC;
				myPlaylistEditor->Content.setItemDisplayer(std::bind(
					Display::Songs, ph::_1, std::cref(myPlaylistEditor->Content), std::cref(Config.song_list_format)
				));
				break;
		}
		Statusbar::printf("Playlist editor display mode: %1%", Config.playlist_editor_display_mode);
	}
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
	Config.autocenter_mode = !Config.autocenter_mode;
	Statusbar::printf("Centering playing song: %1%",
		Config.autocenter_mode ? "on" : "off"
	);
	if (Config.autocenter_mode)
	{
		auto s = myPlaylist->nowPlayingSong();
		if (!s.empty())
			myPlaylist->locateSong(s);
	}
}

bool JumpToPlayingSong::canBeRun()
{
	m_song = myPlaylist->nowPlayingSong();
	return !m_song.empty()
		&& (screenLegacyCurrent() == myPlaylist
		    || screenLegacyCurrent() == myPlaylistEditor
		    || screenLegacyCurrent() == myBrowser
		    || native_c_screen_media_library_is_current());
}

void JumpToPlayingSong::run()
{
	if (screenLegacyCurrent() == myPlaylist)
	{
		myPlaylist->locateSong(m_song);
	}
	else if (screenLegacyCurrent() == myPlaylistEditor)
	{
		myPlaylistEditor->locateSong(m_song);
	}
	else if (screenLegacyCurrent() == myBrowser)
	{
		myBrowser->locateSong(m_song);
	}
	else if (native_c_screen_media_library_is_current())
	{
		(void)locateNativeMediaLibrarySong(m_song, false);
	}
}

bool Shuffle::canBeRun()
{
	if (screenLegacyCurrent() != myPlaylist)
		return false;
	m_begin = myPlaylist->main().begin();
	m_end = myPlaylist->main().end();
	return findSelectedRangeAndPrintInfoIfNot(m_begin, m_end);
}

void Shuffle::run()
{
	if (Config.ask_before_shuffling_playlists)
		if (!confirmAction("Do you really want to shuffle selected range?"))
			return;
	auto begin = myPlaylist->main().begin();
	Mpd.ShuffleRange(m_begin-begin, m_end-begin);
	Statusbar::print("Range shuffled");
}

bool SaveTagChanges::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	return native_c_screen_tiny_tag_editor_is_current()
	    || (screenLegacyCurrent() != nullptr
	        && screenLegacyCurrent()->activeWindow() == myTagEditor->TagTypes);
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void SaveTagChanges::run()
{
#	ifdef HAVE_TAGLIB_H
	if (native_c_screen_tiny_tag_editor_is_current())
	{
		(void)native_tiny_tag_editor_screen_run_row(
			native_c_screen_tiny_tag_editor(),
			NATIVE_TINY_TAG_EDITOR_SAVE_ROW);
	}
	else if (screenLegacyCurrent() != nullptr
	         && screenLegacyCurrent()->activeWindow() == myTagEditor->TagTypes)
	{
		myTagEditor->TagTypes->highlight(myTagEditor->TagTypes->size()-1); // Save
		myTagEditor->runAction();
	}
#	endif // HAVE_TAGLIB_H
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
	BaseScreen *current;
	bool result = false;

	current = screenLegacyCurrent();
	if (current == nullptr)
		return false;
	if (current == myBrowser && !myBrowser->main().empty())
	{
		result = myBrowser->main().current()->value().type()
			== MPD::Item::Type::Directory;
	}
#ifdef HAVE_TAGLIB_H
	else if (current->activeWindow() == myTagEditor->Dirs)
		result = true;
#endif // HAVE_TAGLIB_H
	return result;
}

void EnterDirectory::run()
{
	BaseScreen *current;

	current = screenLegacyCurrent();
	if (current == nullptr)
		return;
	if (current == myBrowser)
		myBrowser->enterDirectory();
#ifdef HAVE_TAGLIB_H
	else if (current->activeWindow() == myTagEditor->Dirs)
	{
		if (!myTagEditor->enterDirectory())
			Statusbar::print("No subdirectories found");
	}
#endif // HAVE_TAGLIB_H
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
	BaseScreen *current;

	current = screenLegacyCurrent();
	return  current != nullptr
	    && (((current == myBrowser
	      && !myBrowser->main().empty()
	      && myBrowser->main().current()->value().type() == MPD::Item::Type::Directory)
#	ifdef HAVE_TAGLIB_H
	    ||   (current->activeWindow() == myTagEditor->Dirs
	      && !myTagEditor->Dirs->empty()
	      && myTagEditor->Dirs->choice() > 0)
#	endif // HAVE_TAGLIB_H
		) &&     isMPDMusicDirSet());
}

void EditDirectoryName::run()
{
		if (screenLegacyCurrent() == myBrowser)
	{
		std::string old_dir = myBrowser->main().current()->value().directory().path();
		std::string new_dir;
		{
			Statusbar::ScopedLock slock;
			Statusbar::put() << NC_FORMAT_BOLD << "Directory: " << NC_FORMAT_NO_BOLD;
			if (!promptString(new_dir, old_dir))
					return;
		}
		if (!new_dir.empty() && new_dir != old_dir)
		{
			std::string full_old_dir;
			if (!myBrowser->isLocal())
				full_old_dir += Config.mpd_music_dir;
			full_old_dir += old_dir;
			std::string full_new_dir;
			if (!myBrowser->isLocal())
				full_new_dir += Config.mpd_music_dir;
			full_new_dir += new_dir;
			std::filesystem::rename(full_old_dir, full_new_dir);
			const char msg[] = "Directory renamed to \"%1%\"";
			Statusbar::printf(msg, Utf8::shorten(new_dir, COLS-const_strlen(msg)));
			if (!myBrowser->isLocal())
				Mpd.UpdateDirectory(getSharedDirectory(old_dir, new_dir));
			myBrowser->requestUpdate();
		}
	}
#	ifdef HAVE_TAGLIB_H
	else if (screenLegacyCurrent()->activeWindow() == myTagEditor->Dirs)
	{
		std::string old_dir = myTagEditor->Dirs->current()->value().first, new_dir;
		{
			Statusbar::ScopedLock slock;
			Statusbar::put() << NC_FORMAT_BOLD << "Directory: " << NC_FORMAT_NO_BOLD;
			if (!promptString(new_dir, old_dir))
					return;
		}
		if (!new_dir.empty() && new_dir != old_dir)
		{
			std::string full_old_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + old_dir;
			std::string full_new_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + new_dir;
			if (rename(full_old_dir.c_str(), full_new_dir.c_str()) == 0)
			{
				const char msg[] = "Directory renamed to \"%1%\"";
				Statusbar::printf(msg, Utf8::shorten(new_dir, COLS-const_strlen(msg)));
				Mpd.UpdateDirectory(myTagEditor->CurrentDir());
			}
			else
			{
				const char msg[] = "Couldn't rename \"%1%\": %2%";
				Statusbar::printf(msg, Utf8::shorten(old_dir, COLS-const_strlen(msg)-25), strerror(errno));
			}
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditPlaylistName::canBeRun()
{
	BaseScreen *current;

	current = screenLegacyCurrent();
	return   current != nullptr
	    &&  ((current->isActiveWindow(myPlaylistEditor->Playlists)
	      && !myPlaylistEditor->Playlists.empty())
	    ||   (current == myBrowser
	      && !myBrowser->main().empty()
		  && myBrowser->main().current()->value().type() == MPD::Item::Type::Playlist));
}

void EditPlaylistName::run()
{
		std::string old_name, new_name;
	if (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Playlists))
		old_name = currentStoredPlaylistPath();
	else
		old_name = myBrowser->main().current()->value().playlist().path();
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << NC_FORMAT_BOLD << "Playlist: " << NC_FORMAT_NO_BOLD;
		if (!promptString(new_name, old_name))
				return;
	}
	if (!new_name.empty() && new_name != old_name)
	{
		Mpd.Rename(old_name, new_name);
		const char msg[] = "Playlist renamed to \"%1%\"";
		Statusbar::printf(msg, Utf8::shorten(new_name, COLS-const_strlen(msg)));
	}
}

bool EditLyrics::canBeRun()
{
	return ncm_action_runtime_can_run(nullptr, NCM_ACTION_EDIT_LYRICS);
}

void EditLyrics::run()
{
	(void)ncm_action_runtime_run(nullptr, NCM_ACTION_EDIT_LYRICS);
}

bool JumpToBrowser::canBeRun()
{
	return currentSongFromNative(m_song);
}

void JumpToBrowser::run()
{
	myBrowser->locateSong(m_song);
}

bool JumpToPlaylistEditor::canBeRun()
{
	return screenLegacyCurrent() == myBrowser
	    && myBrowser->main().current()->value().type() == MPD::Item::Type::Playlist;
}

void JumpToPlaylistEditor::run()
{
	myPlaylistEditor->locatePlaylist(
		myBrowser->main().current()->value().playlist());
}

void ToggleScreenLock::run()
{
		const char *msg_unlockable_screen = "Current screen can't be locked";
	if (screenLegacyLocked() != nullptr)
	{
		BaseScreen::unlock();
		Actions::setResizeFlags();
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
#	ifdef HAVE_TAGLIB_H
	return isMPDMusicDirSet() && currentSongFromNative(m_song);
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void JumpToTagEditor::run()
{
#	ifdef HAVE_TAGLIB_H
	myTagEditor->LocateSong(m_song);
#	endif // HAVE_TAGLIB_H
}

bool JumpToPositionInSong::canBeRun()
{
	return ncm_status_state_player() != NCM_STATUS_PLAYER_STOP && ncm_status_state_total_time() > 0;
}

void JumpToPositionInSong::run()
{

	const MPD::Song s = myPlaylist->nowPlayingSong();

	std::string spos;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Position to go (in %/h:m:ss/m:ss/seconds(s)): ";
		if (!promptString(spos))
				return;
	}

	std::regex rx;
	std::smatch what;
	// mm:ss
	if (std::regex_match(spos, what, rx.assign("([0-9]+):([0-9]{2})")))
	{
		auto mins = fromString<unsigned>(what[1]);
		auto secs = fromString<unsigned>(what[2]);
		boundsCheck(secs, 0u, 60u);
		Mpd.Seek(s.getPosition(), mins * 60 + secs);
	}
	// position in seconds
	else if (std::regex_match(spos, what, rx.assign("([0-9]+)s")))
	{
		auto secs = fromString<unsigned>(what[1]);
		Mpd.Seek(s.getPosition(), secs);
	}
	// position in%
	else if (std::regex_match(spos, what, rx.assign("([0-9]+)[%]{0,1}")))
	{
		auto percent = fromString<unsigned>(what[1]);
		boundsCheck(percent, 0u, 100u);
		int secs = (percent * s.getDuration()) / 100.0;
		Mpd.Seek(s.getPosition(), secs);
	}
	// position in hh:mm:ss
	else if (std::regex_match(spos, what, rx.assign("([0-9]+):([0-9]{2}):([0-9]{2})")))
	{
		auto hours = fromString<unsigned>(what[1]);
		auto mins  = fromString<unsigned>(what[2]);
		auto secs  = fromString<unsigned>(what[3]);
		boundsCheck(mins, 0u, 60u);
		boundsCheck(secs, 0u, 60u);
		Mpd.Seek(s.getPosition(), hours * 3600 + mins * 60 + secs);
	}
	else
		Statusbar::print("Invalid format ([h]:[mm]:[ss], [m]:[ss], [s]s, [%]%, [%] accepted)");
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
	// cropping doesn't make sense in this case
	if (myPlaylist->main().size() <= 1)
		return;
	if (Config.ask_before_clearing_playlists)
		if (!confirmAction("Do you really want to crop main playlist?"))
			return;
	Statusbar::print("Cropping playlist...");
	if (ncm_action_crop_main_playlist())
		Statusbar::print("Playlist cropped");
}

bool CropPlaylist::canBeRun()
{
	return screenLegacyCurrent() == myPlaylistEditor;
}

void CropPlaylist::run()
{
	auto &w = myPlaylistEditor->Content;
	// cropping doesn't make sense in this case
	if (w.size() <= 1)
		return;
	assert(!myPlaylistEditor->Playlists.empty());
	std::string playlist = currentStoredPlaylistPath();
	if (Config.ask_before_clearing_playlists
	    && !confirmAction(stringFormat("Do you really want to crop playlist \"%1%\"?",
	                                  playlist)))
		return;
	selectCurrentIfNoneSelected(w);
	Statusbar::printf("Cropping playlist \"%1%\"...", playlist);
	cropPlaylist(w, [playlist](auto &, unsigned pos) { Mpd.PlaylistDelete(playlist, pos); });
	Statusbar::printf("Playlist \"%1%\" cropped", playlist);
}

void ClearMainPlaylist::run()
{
	if (!myPlaylist->main().empty() && Config.ask_before_clearing_playlists)
		if (!confirmAction("Do you really want to clear main playlist?"))
			return;
	Mpd.ClearMainPlaylist();
	Statusbar::print("Playlist cleared");
	myPlaylist->main().reset();
}

bool ClearPlaylist::canBeRun()
{
	return screenLegacyCurrent() == myPlaylistEditor;
}

void ClearPlaylist::run()
{
	if (myPlaylistEditor->Playlists.empty())
		return;
	std::string playlist = currentStoredPlaylistPath();
	if (Config.ask_before_clearing_playlists
	    && !confirmAction(stringFormat("Do you really want to clear playlist \"%1%\"?",
	                                  playlist)))
		return;
	Mpd.ClearPlaylist(playlist);
	Statusbar::printf("Playlist \"%1%\" cleared", playlist);
}

bool ReversePlaylist::canBeRun()
{
	if (screenLegacyCurrent() != myPlaylist)
		return false;
	m_begin = myPlaylist->main().begin();
	m_end = myPlaylist->main().end();
	if (m_begin == m_end)
		return false;
	else
		return findSelectedRangeAndPrintInfoIfNot(m_begin, m_end);
}

void ReversePlaylist::run()
{
	Statusbar::print("Reversing range...");
	Mpd.StartCommandsList();
	for (--m_end; m_begin < m_end; ++m_begin, --m_end)
		Mpd.Swap(m_begin->value().getPosition(), m_end->value().getPosition());
	Mpd.CommitCommandsList();
	Statusbar::print("Range reversed");
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
	return screenLegacyCurrent() == myBrowser;
}

void ToggleBrowserSortMode::run()
{
	switch (Config.browser_sort_mode)
	{
		case NCM_SORT_MODE_TYPE:
			Config.browser_sort_mode = NCM_SORT_MODE_NAME;
			Statusbar::print("Sort songs by: name");
			break;
		case NCM_SORT_MODE_NAME:
			Config.browser_sort_mode = NCM_SORT_MODE_MODIFICATION_TIME;
			Statusbar::print("Sort songs by: modification time");
			break;
		case NCM_SORT_MODE_MODIFICATION_TIME:
			Config.browser_sort_mode = NCM_SORT_MODE_CUSTOM_FORMAT;
			Statusbar::print("Sort songs by: custom format");
			break;
		case NCM_SORT_MODE_CUSTOM_FORMAT:
			Config.browser_sort_mode = NCM_SORT_MODE_NONE;
			Statusbar::print("Do not sort songs");
			break;
		case NCM_SORT_MODE_NONE:
			Config.browser_sort_mode = NCM_SORT_MODE_TYPE;
			Statusbar::print("Sort songs by: type");
	}
	if (Config.browser_sort_mode != NCM_SORT_MODE_NONE)
	{
		size_t sort_offset = myBrowser->inRootDirectory() ? 0 : 1;
		std::stable_sort(
			myBrowser->main().begin()+sort_offset, myBrowser->main().end(),
			LocaleBasedItemSorting(Config.ignore_leading_the,
			                       Config.browser_sort_mode));
	}
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
	if (Mpd.Version() < 17)
	{
		Statusbar::print("Priorities are supported in MPD >= 0.17.0");
		return false;
	}
	return screenLegacyCurrent() == myPlaylist && !myPlaylist->main().empty();
}

void SetSelectedItemsPriority::run()
{

	unsigned prio;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Set priority [0-255]: ";
		std::string input;
		if (!promptString(input))
			return;
		prio = fromString<unsigned>(input);
		boundsCheck(prio, 0u, 255u);
	}
	myPlaylist->setSelectedItemsPriority(prio);
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
	return screenLegacyCurrent() != myPlaylist
#	ifdef HAVE_TAGLIB_H
	    && !native_c_screen_tiny_tag_editor_is_current()
#	endif // HAVE_TAGLIB_H
	;
}

void ShowPlaylist::run()
{
	myPlaylist->switchTo();
}

bool ShowBrowser::canBeRun()
{
	return screenLegacyCurrent() != myBrowser
#	ifdef HAVE_TAGLIB_H
	    && !native_c_screen_tiny_tag_editor_is_current()
#	endif // HAVE_TAGLIB_H
	;
}

void ShowBrowser::run()
{
	myBrowser->switchTo();
}

bool ChangeBrowseMode::canBeRun()
{
	return screenLegacyCurrent() == myBrowser;
}

void ChangeBrowseMode::run()
{
	myBrowser->changeBrowseMode();
}

bool ShowPlaylistEditor::canBeRun()
{
	return screenLegacyCurrent() != myPlaylistEditor
#	ifdef HAVE_TAGLIB_H
	    && !native_c_screen_tiny_tag_editor_is_current()
#	endif // HAVE_TAGLIB_H
	;
}

void ShowPlaylistEditor::run()
{
	myPlaylistEditor->switchTo();
}

bool ShowTagEditor::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	return screenLegacyCurrent() != myTagEditor
	    && !native_c_screen_tiny_tag_editor_is_current();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void ShowTagEditor::run()
{
#	ifdef HAVE_TAGLIB_H
	if (isMPDMusicDirSet())
		myTagEditor->switchTo();
#	endif // HAVE_TAGLIB_H
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
	insert_action(new Actions::UpdateEnvironment());
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
	insert_action(new Actions::JumpToBrowser());
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
	insert_action(new Actions::ShowBrowser());
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

	if (screenLegacyCurrent() == myPlaylistEditor
#	ifdef HAVE_TAGLIB_H
	||  screenLegacyCurrent() == myTagEditor
#	endif // HAVE_TAGLIB_H
	   )
	{
		if (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Playlists))
		{
			myPlaylistEditor->Content.clear();
			myPlaylistEditor->Content.refresh();
			myPlaylistEditor->updateTimer();
		}
#		ifdef HAVE_TAGLIB_H
		else if (screenLegacyCurrent()->activeWindow() == myTagEditor->Dirs)
		{
			myTagEditor->Tags->clear();
		}
#		endif // HAVE_TAGLIB_H
	}
}

}


extern "C" {

void actions_legacy_runtime_init_readline(void)
{
	NC::initReadline();
}

void *actions_legacy_runtime_window_create(int64 start_x, int64 start_y,
                                   int64 width, int64 height,
                                   NcColor color)
{
	NC::Color window_color = NC::fromNcColor(color);

	return new NC::Window(static_cast<size_t>(start_x),
	                      static_cast<size_t>(start_y),
	                      static_cast<size_t>(width),
	                      static_cast<size_t>(height), "",
	                      window_color, NC::Border());
}

void actions_legacy_runtime_window_display(void *window)
{
	if (window == nullptr)
		return;
	static_cast<NC::Window *>(window)->display();
}

void actions_legacy_runtime_window_destroy(void *window)
{
	delete static_cast<NC::Window *>(window);
}

void actions_legacy_runtime_window_set_main_hook(void *window)
{
	if (window == nullptr)
		return;
	static_cast<NC::Window *>(window)->setPromptHook([](const char *s) {
		int32 s_len = 0;

		if (s != nullptr)
			s_len = static_cast<int32>(std::strlen(s));
		return ncm_statusbar_main_hook(const_cast<char *>(s), s_len);
	});
}

void actions_legacy_runtime_window_clear_fd_callbacks(void *window)
{
	if (window == nullptr)
		return;
	static_cast<NC::Window *>(window)->clearFDCallbacksList();
}

NcWindow *actions_legacy_runtime_window_native(void *window)
{
	if (window == nullptr)
		return nullptr;
	return static_cast<NC::Window *>(window)->nativeWindow();
}

void actions_legacy_runtime_initialize_screens(void)
{
	Actions::initializeScreens();
}

void actions_legacy_runtime_resize_screen(bool reload_main_window)
{
	try
	{
		Actions::resizeScreen(reload_main_window);
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
}

bool actions_legacy_runtime_playlist_highlight_mpd_position(int32 position)
{
	return highlightPlaylistMpdPosition(position);
}

void actions_legacy_runtime_browser_fetch_supported_extensions(void)
{
	if (myBrowser == nullptr)
		return;
	myBrowser->fetchSupportedExtensions();
}

bool actions_legacy_runtime_update_environment(bool update_timer,
                                        bool refresh_window,
                                        bool mpd_sync)
{
	try
	{
		auto action = Actions::runtimeAction(
		    NCM_ACTION_UPDATE_ENVIRONMENT);
		auto update_environment = static_cast<Actions::UpdateEnvironment *>(
		    action);

		if (update_environment == nullptr)
			return false;
		update_environment->run(update_timer, refresh_window, mpd_sync);
		return true;
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
