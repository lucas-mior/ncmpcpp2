/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef NCMPCPP_MPDPP_H
#define NCMPCPP_MPDPP_H

#include <cassert>
#include <cstddef>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <mpd/client.h>
#include "c/ncm_mpd_item.h"
#include "c/ncm_mpd_connection.h"
#include "c/ncm_random.h"
#include "cbase/cbase.h"
#include "song.h"

namespace MPD {

void checkConnectionErrors(mpd_connection *conn);

enum PlayerState { psUnknown, psStop, psPlay, psPause };
enum ReplayGainMode { rgmOff, rgmTrack, rgmAlbum };

struct Error: public std::exception
{
	Error(std::string msg, bool clearable_)
		: m_msg(msg), m_clearable(clearable_) { }
	virtual ~Error() noexcept { }

	virtual const char *what() const noexcept { return m_msg.c_str(); }
	bool clearable() const { return m_clearable; }

private:
	std::string m_msg;
	bool m_clearable;
};

struct ClientError: public Error
{
	ClientError(mpd_error code_, std::string msg, bool clearable_)
		: Error(msg, clearable_), m_code(code_) { }
	virtual ~ClientError() noexcept { }
	
	mpd_error code() const { return m_code; }

private:
	mpd_error m_code;
};

struct ServerError: public Error
{
	ServerError(mpd_server_error code_, std::string msg, bool clearable_)
		: Error(msg, clearable_), m_code(code_) { }
	virtual ~ServerError() noexcept { }
	
	mpd_server_error code() const { return m_code; }

private:
	mpd_server_error m_code;
};

struct Statistics
{
	friend struct Connection;
	
	bool empty() const { return !m_valid; }
	
	unsigned artists() const { return m_stats.artists; }
	unsigned albums() const { return m_stats.albums; }
	unsigned songs() const { return m_stats.songs; }
	unsigned long playTime() const { return m_stats.play_time; }
	unsigned long uptime() const { return m_stats.uptime; }
	unsigned long dbUpdateTime() const { return m_stats.db_update_time; }
	unsigned long dbPlayTime() const { return m_stats.db_play_time; }
	
private:
	Statistics(const NcmMpdStats &stats)
	: m_stats(stats)
	, m_valid(true)
	{ }
	
	NcmMpdStats m_stats;
	bool m_valid;
};

struct Status
{
	friend struct Connection;
	
	Status()
	: m_status{}
	, m_valid(false)
	{ }
	
	void clear()
	{
		m_status = NcmMpdStatus{};
		m_valid = false;
	}
	bool empty() const { return !m_valid; }
	
	int volume() const { return m_status.volume; }
	bool repeat() const { return m_status.repeat; }
	bool random() const { return m_status.random; }
	bool single() const { return m_status.single; }
	bool consume() const { return m_status.consume; }
	unsigned playlistLength() const { return m_status.queue_length; }
	unsigned playlistVersion() const { return m_status.queue_version; }
	PlayerState playerState() const { return PlayerState(m_status.state); }
	unsigned crossfade() const { return m_status.crossfade; }
	int currentSongPosition() const { return m_status.song_pos; }
	int currentSongID() const { return m_status.song_id; }
	int nextSongPosition() const { return m_status.next_song_pos; }
	int nextSongID() const { return m_status.next_song_id; }
	unsigned elapsedTime() const { return m_status.elapsed_time; }
	unsigned totalTime() const { return m_status.total_time; }
	unsigned kbps() const { return m_status.kbit_rate; }
	unsigned updateID() const { return m_status.update_id; }
	const char *error() const { return m_status.error; }
	
private:
	Status(const NcmMpdStatus &status)
	: m_status(status)
	, m_valid(true)
	{ }
	
	NcmMpdStatus m_status;
	bool m_valid;
};

struct Directory
{
	Directory();
	Directory(const mpd_directory *directory);
	Directory(NcmDirectory *directory);
	Directory(std::string path_, time_t last_modified = 0);
	Directory(const Directory &rhs);
	Directory(Directory &&rhs) noexcept;
	Directory &operator=(const Directory &rhs);
	Directory &operator=(Directory &&rhs) noexcept;
	~Directory();

	bool operator==(const Directory &rhs) const;
	bool operator!=(const Directory &rhs) const
	{
		return !(*this == rhs);
	}

	const std::string &path() const;
	time_t lastModified() const;
	NcmDirectory *cDirectory()
	{
		return &m_directory;
	}

private:
	NcmDirectory m_directory;
	mutable std::string m_path_cache;
};

struct Playlist
{
	Playlist();
	Playlist(const mpd_playlist *playlist);
	Playlist(NcmPlaylist *playlist);
	Playlist(std::string path_, time_t last_modified = 0);
	Playlist(const Playlist &rhs);
	Playlist(Playlist &&rhs) noexcept;
	Playlist &operator=(const Playlist &rhs);
	Playlist &operator=(Playlist &&rhs) noexcept;
	~Playlist();

	bool operator==(const Playlist &rhs) const;
	bool operator!=(const Playlist &rhs) const
	{
		return !(*this == rhs);
	}

	const std::string &path() const;
	time_t lastModified() const;
	NcmPlaylist *cPlaylist()
	{
		return &m_playlist;
	}

private:
	NcmPlaylist m_playlist;
	mutable std::string m_path_cache;
};

struct Item
{
	enum class Type { Directory, Song, Playlist };

	Item();
	Item(mpd_entity *entity);
	Item(NcmMpdItem *item);
	Item(Directory directory_);
	Item(Song song_);
	Item(Playlist playlist_);
	Item(const Item &rhs);
	Item(Item &&rhs) noexcept;
	Item &operator=(const Item &rhs);
	Item &operator=(Item &&rhs) noexcept;
	~Item();

	bool operator==(const Item &rhs) const;
	bool operator!=(const Item &rhs) const
	{
		return !(*this == rhs);
	}

	Type type() const;

	Directory &directory()
	{
		return const_cast<Directory &>(
			static_cast<const Item &>(*this).directory());
	}
	Song &song()
	{
		return const_cast<Song &>(
			static_cast<const Item &>(*this).song());
	}
	Playlist &playlist()
	{
		return const_cast<Playlist &>(
			static_cast<const Item &>(*this).playlist());
	}

	const Directory &directory() const;
	const Song &song() const;
	const Playlist &playlist() const;

private:
	NcmMpdItem m_item;
	mutable Directory m_directory_cache;
	mutable Song m_song_cache;
	mutable Playlist m_playlist_cache;
};


template <typename ObjectT>
struct Iterator
{
	using iterator_category = std::input_iterator_tag;
	using value_type = ObjectT;
	using difference_type = std::ptrdiff_t;
	using pointer = ObjectT *;
	using reference = ObjectT &;

	// shared state of the iterator
	struct State
	{
		friend Iterator;

		typedef std::function<bool(State &)> Fetcher;

		State(mpd_connection *connection_, Fetcher fetcher)
		: m_connection(connection_)
		, m_fetcher(fetcher)
		, m_objects(nullptr)
		, m_index(0)
		{
			assert(m_connection != nullptr);
			assert(m_fetcher != nullptr);
		}
		State(std::vector<ObjectT> objects)
		: m_connection(nullptr)
		, m_fetcher(nullptr)
		, m_objects(new std::vector<ObjectT>(std::move(objects)))
		, m_index(0)
		{ }
		~State()
		{
			if (m_connection != nullptr)
				mpd_response_finish(m_connection);
		}

		mpd_connection *connection() const
		{
			return m_connection;
		}

		void setObject(ObjectT object)
		{
			if (hasObject())
				*m_object = std::move(object);
			else
				m_object.reset(new ObjectT(std::move(object)));
		}

	private:
		bool operator==(const State &rhs) const
		{
			return m_connection == rhs.m_connection
			    && m_object == m_object;
		}
		bool operator!=(const State &rhs) const
		{
			return !(*this == rhs);
		}

		bool fetch()
		{
			if (m_objects != nullptr)
			{
				if (m_index >= m_objects->size())
					return false;
				setObject(std::move((*m_objects)[m_index]));
				m_index += 1;
				return true;
			}
			return m_fetcher(*this);
		}
		ObjectT &getObject() const
		{
			return *m_object;
		}
		bool hasObject() const
		{
			return m_object.get() != nullptr;
		}

		mpd_connection *m_connection;
		Fetcher m_fetcher;
		std::shared_ptr<std::vector<ObjectT>> m_objects;
		size_t m_index;
		std::unique_ptr<ObjectT> m_object;
	};

	Iterator()
	: m_state(nullptr)
	{ }
	Iterator(mpd_connection *connection, typename State::Fetcher fetcher)
	: m_state(std::make_shared<State>(connection, std::move(fetcher)))
	{
		// get the first element
		++*this;
	}
	Iterator(std::vector<ObjectT> objects)
	: m_state(std::make_shared<State>(std::move(objects)))
	{
		// get the first element
		++*this;
	}
	~Iterator()
	{
		if (m_state && m_state->connection() != nullptr)
			checkConnectionErrors(m_state->connection());
	}

	void finish()
	{
		assert(m_state);
		// check errors and change the iterator into end iterator
		if (m_state->connection() != nullptr)
			checkConnectionErrors(m_state->connection());
		m_state = nullptr;
	}

	ObjectT &operator*() const
	{
		if (!m_state)
			throw std::runtime_error("no object associated with the iterator");
		assert(m_state->hasObject());
		return m_state->getObject();
	}
	ObjectT *operator->() const
	{
		return &**this;
	}

	Iterator &operator++()
	{
		assert(m_state);
		if (!m_state->fetch())
			finish();
		return *this;
	}
	Iterator operator++(int)
	{
		Iterator it(*this);
		++*this;
		return it;
	}

	bool operator==(const Iterator &rhs) const
	{
		return m_state == rhs.m_state;
	}
	bool operator!=(const Iterator &rhs) const
	{
		return !(*this == rhs);
	}

private:
	std::shared_ptr<State> m_state;
};

typedef Iterator<Directory> DirectoryIterator;
typedef Iterator<Item> ItemIterator;
typedef Iterator<Song> SongIterator;
typedef Iterator<std::string> StringIterator;

struct Connection
{
	typedef std::function<void(int)> NoidleCallback;

	Connection();
	~Connection();
	
	void Connect();
	bool Connected() const;
	void Disconnect();
	
	const std::string &GetHostname();
	int GetPort();
	
	unsigned Version() const;
	
	int GetFD() const;
	
	void SetHostname(const std::string &);
	void SetPort(int port);
	void SetTimeout(int timeout);
	void SetPassword(const std::string &password);
	void SendPassword();
	
	Statistics getStatistics();
	Status getStatus();
	
	void UpdateDirectory(const std::string &);
	
	void Play();
	void Play(int);
	void PlayID(int);
	void Pause(bool);
	void Toggle();
	void Stop();
	void Next();
	void Prev();
	void Move(unsigned int from, unsigned int to);
	void Swap(unsigned, unsigned);
	void Seek(unsigned int pos, unsigned int where);
	void Shuffle();
	void ShuffleRange(unsigned start, unsigned end);
	void ClearMainPlaylist();
	
	SongIterator GetPlaylistChanges(unsigned);
	
	Song GetCurrentSong();
	Song GetSong(const std::string &);
	SongIterator GetPlaylistContent(const std::string &name);
	SongIterator GetPlaylistContentNoInfo(const std::string &name);
	
	StringIterator GetSupportedExtensions();
	
	void SetRepeat(bool);
	void SetRandom(bool);
	void SetSingle(bool);
	void SetConsume(bool);
	void SetCrossfade(unsigned);
	void SetVolume(unsigned int vol);
	void ChangeVolume(int change);

	std::string GetReplayGainMode();
	void SetReplayGainMode(ReplayGainMode);
	
	void SetPriority(NcmSong *s, int prio);
	
	int AddSong(const std::string &, int = -1); // returns id of added song
	int AddSong(NcmSong *, int = -1); // returns id of added song
	bool AddRandomTag(mpd_tag_type, size_t, NcmRandom *rng);
	bool AddRandomSongs(size_t number, const std::string &random_exclude_pattern, NcmRandom *rng);
	bool Add(const std::string &path);
	void Delete(unsigned int pos);
	void DeleteRange(unsigned begin, unsigned end);
	void PlaylistDelete(const std::string &playlist, unsigned int pos);
	void StartCommandsList();
	void CommitCommandsList();
	
	void DeletePlaylist(const std::string &name);
	bool LoadPlaylist(const std::string &name);
	void SavePlaylist(const std::string &);
	void ClearPlaylist(const std::string &playlist);
	void AddToPlaylist(const std::string &, NcmSong *);
	void AddToPlaylist(const std::string &, const std::string &);
	void PlaylistMove(const std::string &path, int from, int to);
	void Rename(const std::string &from, const std::string &to);
	
	void StartSearch(bool);
	void StartFieldSearch(mpd_tag_type);
	void AddSearch(mpd_tag_type item, const std::string &str) const;
	void AddSearchAny(const std::string &str) const;
	void AddSearchURI(const std::string &str) const;
	SongIterator CommitSearchSongs();
	
	StringIterator GetList(mpd_tag_type type);
	ItemIterator GetDirectory(const std::string &directory);
	SongIterator GetDirectoryRecursive(const std::string &directory);
	SongIterator GetSongs(const std::string &directory);
	DirectoryIterator GetDirectories(const std::string &directory);
	
	StringIterator GetURLHandlers();
	StringIterator GetTagTypes();
	
	void idle();
	int noidle();
	void setNoidleCallback(NoidleCallback callback);
	void dispatchNoidleCallback(int flags);
	
private:
	mpd_connection *rawConnection() const;
	void throwConnectionError() const;
	void checkConnection() const;
	void prechecks();
	void prechecksNoCommandsList();
	void checkErrors() const;

	NoidleCallback m_noidle_callback;
	std::string m_host;
	int m_port;
	int m_timeout;
	std::string m_password;
};

}

extern MPD::Connection Mpd;

#endif // NCMPCPP_MPDPP_H
