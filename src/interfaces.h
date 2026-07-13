#ifndef NCMPCPP_INTERFACES_H
#define NCMPCPP_INTERFACES_H

#include <string>
#include <vector>
#include "c/ncm_enums.h"
#include "screens/screen_cpp_compat.h"
#include "song.h"

struct Searchable
{
	virtual bool allowsSearching() = 0;

	virtual const std::string &searchConstraint() = 0;
	virtual void setSearchConstraint(const std::string &constraint) = 0;
	virtual void clearSearchConstraint() = 0;
	virtual bool search(SearchDirection direction, bool wrap, bool skip_current) = 0;
};

struct Filterable
{
	virtual bool allowsFiltering() = 0;
	virtual std::string currentFilter() = 0;
	virtual void applyFilter(const std::string &constraint) = 0;
};

struct HasActions
{
	virtual bool actionRunnable() = 0;
	virtual void runAction() = 0;
};

struct HasSongs
{
	virtual bool itemAvailable() = 0;
	virtual bool addItemToPlaylist(bool play) = 0;
	virtual std::vector<MPD::Song> getSelectedSongs() = 0;
};

struct HasColumns
{
	virtual bool previousColumnAvailable() = 0;
	virtual void previousColumn() = 0;
	
	virtual bool nextColumnAvailable() = 0;
	virtual void nextColumn() = 0;
};

struct Tabbable
{
	Tabbable() : m_previous_screen(0) { }
	
	void switchToPreviousScreen() const {
		if (m_previous_screen)
			m_previous_screen->switchTo();
	}
	void setPreviousScreen(BaseScreen *screen) {
		m_previous_screen = screen;
	}
	
private:
	BaseScreen *m_previous_screen;
};

namespace screen_compat {

inline void set_tab_previous_screen(BaseScreen *screen)
{
    BaseScreen *previous_screen = previous_legacy_screen();
    auto tab = dynamic_cast<Tabbable *>(screen);

    if (tab == nullptr)
        return;
    if (dynamic_cast<Tabbable *>(previous_screen) == nullptr)
        return;
    tab->setPreviousScreen(previous_screen);
}

}

#endif // NCMPCPP_INTERFACES_H
