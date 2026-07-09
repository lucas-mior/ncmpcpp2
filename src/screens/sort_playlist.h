#ifndef NCMPCPP_SORT_PLAYLIST_H
#define NCMPCPP_SORT_PLAYLIST_H

#include "runnable_item.h"
#include "interfaces.h"
#include "screens/screen_cpp_compat.h"
#include "song.h"

struct SortPlaylistDialog
	: Screen<NC::Menu<RunnableItem<std::pair<std::string, NcmSongGetter>, void()>>>, HasActions, Tabbable
{
	typedef SortPlaylistDialog Self;
	
	SortPlaylistDialog();
	
	virtual void switchTo() override;
	virtual void resize() override;
	
	virtual std::string title() override;
	virtual ScreenType type() override { return NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG; }
	
	virtual void update() override { }
	
	virtual void mouseButtonPressed(MEVENT me) override;
	
	virtual bool isLockable() override { return false; }
	virtual bool isMergable() override { return false; }

	// HasActions implementation
	virtual bool actionRunnable() override;
	virtual void runAction() override;

	// private members
	void moveSortOrderUp();
	void moveSortOrderDown();
	
private:
	void moveSortOrderHint() const;
	void sort() const;
	void cancel() const;
	
	void setDimensions();
	
	size_t m_height;
	size_t m_width;
};

extern SortPlaylistDialog *mySortPlaylistDialog;

#endif // NCMPCPP_SORT_PLAYLIST_H
