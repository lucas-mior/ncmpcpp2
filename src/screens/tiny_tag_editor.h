#ifndef NCMPCPP_TINY_TAG_EDITOR_H
#define NCMPCPP_TINY_TAG_EDITOR_H

#include "config.h"

#ifdef HAVE_TAGLIB_H

#include <cerrno>
#include <cstring>
#include <string>

#include "app_controller.h"
#include "c/ncm_taglib.h"
#include "c/ncm_type_conversions.h"
#include "charset.h"
#include "global.h"
#include "helpers_legacy.h"
#include "interfaces.h"
#include "mutable_song.h"
#include "screens/browser.h"
#include "screens/native_c_screens.h"
#include "screens/nc_tiny_tag_editor.h"
#include "screens/playlist.h"
#include "screens/screen_cpp_compat.h"
#include "screens/screen_cpp_legacy.h"
#include "screens/screen_cpp_switcher.h"
#include "screens/song_info.h"
#include "screens/tag_editor.h"
#include "statusbar.h"
#include "tags.h"
#include "title_legacy.h"
#include "ui_state.h"
#include "utility/string.h"

struct TinyTagEditor: Screen<NC::Menu<NC::Buffer>>, HasActions
{
	TinyTagEditor();
    virtual ~TinyTagEditor();
	
	// Screen< NC::Menu<NC::Buffer> > implementation
	virtual void resize() override;
	virtual void switchTo() override;
	
	virtual std::string title() override;
	virtual ScreenType type() override { return NCM_SCREEN_TYPE_TINY_TAG_EDITOR; }
    virtual NcScreen *nativeScreen() override;
    virtual const NcScreen *nativeScreen() const override;
	
	virtual void update() override { }
	
	virtual void mouseButtonPressed(MEVENT me) override;
	
	virtual bool isLockable() override { return false; }
	virtual bool isMergable() override { return true; }

	// HasActions implementation
	virtual bool actionRunnable() override;
	virtual void runAction() override;

	// private members
	void SetEdited(const MPD::Song &);
	
private:
    static NcWindow *nativeActiveWindowCallback(void *user);
    static void nativeRefreshCallback(void *user);
    static void nativeRefreshWindowCallback(void *user);
    static void nativeScrollCallback(void *user, enum NcScroll where);
    static void nativeSwitchToCallback(void *user);
    static void nativeResizeCallback(void *user);
    static char *nativeTitleCallback(void *user);
    static void nativeUpdateCallback(void *user);
    static void nativeMouseButtonPressedCallback(void *user, MEVENT event);
	bool getTags();
	MPD::MutableSong itsEdited;
	BaseScreen *m_previous_screen;
    std::string m_title_cache;
};

inline TinyTagEditor *myTinyTagEditor = nullptr;

namespace {

std::string tinyTagEditorChannelsString(int channels)
{
	char buffer[32];
	int32 len = ncm_channels_to_string(
		static_cast<int32>(channels),
		buffer,
		static_cast<int32>(sizeof(buffer)));
	return std::string(buffer, static_cast<size_t>(len));
}

}

inline TinyTagEditor::TinyTagEditor()
: Screen(NC::Menu<NC::Buffer>(0, static_cast<size_t>(ui_state_main_start_y()), COLS, static_cast<size_t>(ui_state_main_height()), "", Config.main_color, NC::Border()))
, m_previous_screen(nullptr)
{
	w.setHighlightPrefix(Config.current_item_prefix);
	w.setHighlightSuffix(Config.current_item_suffix);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setItemDisplayer([](NC::Menu<NC::Buffer> &menu) {
		menu << menu.drawn()->value();
	});

    native_c_screen_tiny_tag_editor_init();
    {
        NativeTinyTagEditorBridge bridge = {};

        bridge.active_window = nativeActiveWindowCallback;
        bridge.refresh = nativeRefreshCallback;
        bridge.refresh_window = nativeRefreshWindowCallback;
        bridge.scroll = nativeScrollCallback;
        bridge.switch_to = nativeSwitchToCallback;
        bridge.resize = nativeResizeCallback;
        bridge.title = nativeTitleCallback;
        bridge.update = nativeUpdateCallback;
        bridge.mouse_button_pressed = nativeMouseButtonPressedCallback;
        bridge.user = this;
        native_tiny_tag_editor_screen_set_bridge(
            native_c_screen_tiny_tag_editor(), bridge);
    }
    screen_compat::bind_legacy_owner(nativeScreen(), this);
}

inline TinyTagEditor::~TinyTagEditor()
{
    NativeTinyTagEditorBridge bridge = {};

    native_tiny_tag_editor_screen_set_bridge(
        native_c_screen_tiny_tag_editor(), bridge);
    screen_compat::bind_legacy_owner(nativeScreen(), nullptr);
    if (app_controller_is_screen_registered(nativeScreen()))
        app_controller_unregister_screen(nativeScreen());
}

inline NcScreen *TinyTagEditor::nativeScreen()
{
    return native_c_screen_tiny_tag_editor_native();
}

inline const NcScreen *TinyTagEditor::nativeScreen() const
{
    return native_c_screen_tiny_tag_editor_native();
}

inline void TinyTagEditor::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, static_cast<size_t>(ui_state_main_height()));
	w.moveTo(x_offset, static_cast<size_t>(ui_state_main_start_y()));
	hasToBeResized = 0;
}

inline void TinyTagEditor::switchTo()
{
		if (itsEdited.isStream())
	{
		Statusbar::print("Streams can't be edited");
	}
	else if (getTags())
	{
		m_previous_screen = screenLegacyCurrent();
        nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
        app_controller_switch_to_screen(nativeScreen());
		drawHeader();
	}
	else
	{
		std::string full_path;
		if (itsEdited.isFromDatabase())
			full_path += Config.mpd_music_dir;
		full_path += itsEdited.getURI();
		
		const char msg[] = "Couldn't read file \"%1%\"";
		Statusbar::printf(msg, Utf8::shorten(full_path, COLS-const_strlen(msg)));
	}
}

inline std::string TinyTagEditor::title()
{
	return "Tiny tag editor";
}

inline void TinyTagEditor::mouseButtonPressed(MEVENT me)
{
	if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		if (!w.Goto(me.y))
			return;
		if (me.bstate & BUTTON3_PRESSED)
		{
			w.refresh();
			runAction();
		}
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

inline NcWindow *TinyTagEditor::nativeActiveWindowCallback(void *user)
{
    TinyTagEditor *editor = static_cast<TinyTagEditor *>(user);

    if (editor == nullptr)
        return nullptr;
    return editor->w.nativeWindow();
}

inline void TinyTagEditor::nativeRefreshCallback(void *user)
{
    TinyTagEditor *editor = static_cast<TinyTagEditor *>(user);

    if (editor == nullptr)
        return;
    editor->refresh();
}

inline void TinyTagEditor::nativeRefreshWindowCallback(void *user)
{
    TinyTagEditor *editor = static_cast<TinyTagEditor *>(user);

    if (editor == nullptr)
        return;
    editor->refreshWindow();
}

inline void TinyTagEditor::nativeScrollCallback(void *user,
                                               enum NcScroll where)
{
    TinyTagEditor *editor = static_cast<TinyTagEditor *>(user);

    if (editor == nullptr)
        return;
    editor->scroll(where);
}

inline void TinyTagEditor::nativeSwitchToCallback(void *user)
{
    TinyTagEditor *editor = static_cast<TinyTagEditor *>(user);

    if (editor == nullptr)
        return;
    screen_compat::set_tab_previous_screen(editor);
    syncLegacyScreenPointers();
}

inline void TinyTagEditor::nativeResizeCallback(void *user)
{
    TinyTagEditor *editor = static_cast<TinyTagEditor *>(user);

    if (editor == nullptr)
        return;
    editor->resize();
}

inline char *TinyTagEditor::nativeTitleCallback(void *user)
{
    TinyTagEditor *editor = static_cast<TinyTagEditor *>(user);

    if (editor == nullptr)
        return nullptr;
    editor->m_title_cache = editor->title();
    return const_cast<char *>(editor->m_title_cache.c_str());
}

inline void TinyTagEditor::nativeUpdateCallback(void *user)
{
    TinyTagEditor *editor = static_cast<TinyTagEditor *>(user);

    if (editor == nullptr)
        return;
    editor->update();
}

inline void TinyTagEditor::nativeMouseButtonPressedCallback(void *user,
                                                           MEVENT event)
{
    TinyTagEditor *editor = static_cast<TinyTagEditor *>(user);

    if (editor == nullptr)
        return;
    editor->mouseButtonPressed(event);
}


/**********************************************************************/

inline bool TinyTagEditor::actionRunnable()
{
	return !w.empty();
}

inline void TinyTagEditor::runAction()
{
	size_t option = w.choice();
	if (option < 19) // separator after comment
	{
		Statusbar::ScopedLock slock;
		size_t pos = option-8;
		Statusbar::put() << NC_FORMAT_BOLD << ncm_song_info_tags[pos].name << ": " << NC_FORMAT_NO_BOLD;
		itsEdited.setTags(ncm_song_info_tags[pos].field, static_cast<NC::Window *>(ui_state_footer_legacy_window())->prompt(
			itsEdited.getTags(ncm_song_info_tags[pos].get)));
		w.at(option).value().clear();
		w.at(option).value() << NC_FORMAT_BOLD << ncm_song_info_tags[pos].name << ':' << NC_FORMAT_NO_BOLD << ' ';
		ShowTag(w.at(option).value(), itsEdited.getTags(ncm_song_info_tags[pos].get));
	}
	else if (option == 20)
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << NC_FORMAT_BOLD << "Filename: " << NC_FORMAT_NO_BOLD;
		std::string filename = itsEdited.getNewName().empty() ? itsEdited.getName() : itsEdited.getNewName();
		size_t dot = filename.rfind(".");
		std::string extension;
		if (dot != std::string::npos)
		{
			extension = filename.substr(dot);
			filename = filename.substr(0, dot);
		}
		std::string new_name = static_cast<NC::Window *>(ui_state_footer_legacy_window())->prompt(filename);
		if (!new_name.empty())
		{
			itsEdited.setNewName(new_name + extension);
			w.at(option).value().clear();
			w.at(option).value() << NC_FORMAT_BOLD << "Filename:" << NC_FORMAT_NO_BOLD << ' ' << (itsEdited.getNewName().empty() ? itsEdited.getName() : itsEdited.getNewName());
		}
	}

	if (option == 22)
	{
		Statusbar::print("Updating tags...");
		if (ncm_tags_write_mutable_song(itsEdited))
		{
			Statusbar::print("Tags updated");
			if (itsEdited.isFromDatabase())
				Mpd.UpdateDirectory(itsEdited.getDirectory());
			else
			{
				if (m_previous_screen == myPlaylist)
					myPlaylist->main().current()->value() = itsEdited;
				else if (m_previous_screen == myBrowser)
					myBrowser->requestUpdate();
			}
		}
		else
			Statusbar::printf("Error while writing tags: %1%", strerror(errno));
	}
	if (option > 21)
		m_previous_screen->switchTo();
}

/**********************************************************************/

inline void TinyTagEditor::SetEdited(const MPD::Song &s)
{
	if (auto ms = dynamic_cast<const MPD::MutableSong *>(&s))
		itsEdited = *ms;
	else
		itsEdited = s;
}

inline bool TinyTagEditor::getTags()
{
	std::string path_to_file;
	if (itsEdited.isFromDatabase())
		path_to_file += Config.mpd_music_dir;
	path_to_file += itsEdited.getURI();
	
	NcmTaglibFile file;
	NcmTaglibAudioProperties properties;
	ncm_taglib_file_init(&file);
	if (!ncm_taglib_file_open(&file, const_cast<char *>(path_to_file.c_str())))
		return false;
	ncm_taglib_file_audio_properties(&file, &properties);
	
	std::string ext = itsEdited.getURI();
	ext = lowercaseAscii(ext.substr(ext.rfind(".")+1));
	
	w.clear();
	w.reset();
	
	w.resizeList(24);
	
	for (size_t i = 0; i < 7; ++i)
		w[i].setInactive(true);
	
	w[7].setSeparator(true);
	w[19].setSeparator(true);
	w[21].setSeparator(true);
	
	if (!ncm_taglib_extended_set_supported(&file))
	{
		w[10].setInactive(true);
		for (size_t i = 15; i <= 17; ++i)
			w[i].setInactive(true);
	}
	
	ncm_taglib_file_close(&file);
	w.highlight(8);

	auto print_key_value = [](NC::Buffer &buf, const char *key, const auto &value) {
		buf << NC_FORMAT_BOLD
		    << Config.color1
		    << key
		    << ":"
		    << NC::FormattedColor::End<>(Config.color1)
		    << NC_FORMAT_NO_BOLD
		    << " "
		    << Config.color2
		    << value
		    << NC::FormattedColor::End<>(Config.color2);
	};

	print_key_value(w[0].value(), "Filename", itsEdited.getName());
	print_key_value(w[1].value(), "Directory", ShowTag(itsEdited.getDirectory()));
	print_key_value(w[3].value(), "Length", itsEdited.getLength());
	print_key_value(
		w[4].value(),
		"Bitrate",
		std::to_string(properties.bitrate) + " kbps");
	print_key_value(
		w[5].value(),
		"Sample rate",
		std::to_string(properties.sample_rate) + " Hz");
	print_key_value(
		w[6].value(),
		"Channels",
		tinyTagEditorChannelsString(properties.channels));
	
	unsigned pos = 8;
	for (const NcmSongInfoMetadata *m = ncm_song_info_tags; m->name; ++m, ++pos)
	{
		w[pos].value() << NC_FORMAT_BOLD
		                  << m->name
		                  << ":"
		                  << NC_FORMAT_NO_BOLD
		                  << " ";
		ShowTag(w[pos].value(), itsEdited.getTags(m->get));
	}
	
	w[20].value() << NC_FORMAT_BOLD
	                 << "Filename:"
	                 << NC_FORMAT_NO_BOLD
	                 << " "
	                 << itsEdited.getName();
	
	w[22].value() << "Save";
	w[23].value() << "Cancel";
	return true;
}


#endif // HAVE_TAGLIB_H

#endif // NCMPCPP_TINY_TAG_EDITOR_H

