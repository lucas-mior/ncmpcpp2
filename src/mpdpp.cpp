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

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <map>
#include <memory>
#include <new>
#include <regex>

#include "charset.h"
#include "c/ncm_mpd_item.h"
#include "c/ncm_random.h"
#include "global.h"
#include "mpdpp.h"

MPD::Connection Mpd;

namespace {

struct NcmMpdItemGuard
{
	NcmMpdItemGuard()
	{
		ncm_mpd_item_init(&m_item);
	}
	~NcmMpdItemGuard()
	{
		ncm_mpd_item_destroy(&m_item);
	}

	NcmMpdItem *get()
	{
		return &m_item;
	}

private:
	NcmMpdItem m_item;
};

struct NcmSongGuard
{
	NcmSongGuard()
	{
		ncm_song_init(&m_song);
	}
	~NcmSongGuard()
	{
		ncm_song_destroy(&m_song);
	}

	NcmSong *get()
	{
		return &m_song;
	}

private:
	NcmSong m_song;
};

struct NcmMpdSongListGuard
{
	NcmMpdSongListGuard()
	{
		ncm_mpd_song_list_init(&m_list);
	}
	~NcmMpdSongListGuard()
	{
		ncm_mpd_song_list_destroy(&m_list);
	}

	NcmMpdSongList *get()
	{
		return &m_list;
	}

private:
	NcmMpdSongList m_list;
};

struct NcmMpdItemListGuard
{
	NcmMpdItemListGuard()
	{
		ncm_mpd_item_list_init(&m_list);
	}
	~NcmMpdItemListGuard()
	{
		ncm_mpd_item_list_destroy(&m_list);
	}

	NcmMpdItemList *get()
	{
		return &m_list;
	}

private:
	NcmMpdItemList m_list;
};

struct NcmMpdStringListGuard
{
	NcmMpdStringListGuard()
	{
		ncm_mpd_string_list_init(&m_list);
	}
	~NcmMpdStringListGuard()
	{
		ncm_mpd_string_list_destroy(&m_list);
	}

	NcmMpdStringList *get()
	{
		return &m_list;
	}

private:
	NcmMpdStringList m_list;
};



struct NcmMpdPlaylistListGuard
{
	NcmMpdPlaylistListGuard()
	{
		ncm_mpd_playlist_list_init(&m_list);
	}
	~NcmMpdPlaylistListGuard()
	{
		ncm_mpd_playlist_list_destroy(&m_list);
	}

	NcmMpdPlaylistList *get()
	{
		return &m_list;
	}

private:
	NcmMpdPlaylistList m_list;
};

std::vector<MPD::Song> songVectorFromList(NcmMpdSongList *list)
{
	std::vector<MPD::Song> result;

	result.reserve(static_cast<size_t>(list->count));
	for (int32 i = 0; i < list->count; i += 1)
		result.emplace_back(&list->items[i]);
	return result;
}

std::vector<MPD::Item> itemVectorFromList(NcmMpdItemList *list)
{
	std::vector<MPD::Item> result;

	result.reserve(static_cast<size_t>(list->count));
	for (int32 i = 0; i < list->count; i += 1)
		result.emplace_back(&list->items[i]);
	return result;
}

std::vector<MPD::Directory> directoryVectorFromItemList(NcmMpdItemList *list)
{
	std::vector<MPD::Directory> result;

	for (int32 i = 0; i < list->count; i += 1)
	{
		if (ncm_mpd_item_kind(&list->items[i]) == NCM_MPD_ITEM_DIRECTORY)
			result.emplace_back(ncm_mpd_item_directory(&list->items[i]));
	}
	return result;
}

std::vector<std::string> stringVectorFromList(NcmMpdStringList *list)
{
	std::vector<std::string> result;

	result.reserve(static_cast<size_t>(list->count));
	for (int32 i = 0; i < list->count; i += 1)
	{
		result.emplace_back(list->items[i].value,
		                    static_cast<size_t>(list->items[i].value_len));
	}
	return result;
}



template <typename ObjectT, typename SourceT>
std::function<bool(typename MPD::Iterator<ObjectT>::State &)>
defaultFetcher(SourceT *(fetcher)(mpd_connection *))
{
	return [fetcher](typename MPD::Iterator<ObjectT>::State &state) {
		auto src = fetcher(state.connection());
		if (src != nullptr)
		{
			state.setObject(src);
			return true;
		}
		else
			return false;
	};
}

}

namespace MPD {

Directory::Directory()
{
	ncm_directory_init(&m_directory);
}

Directory::Directory(const mpd_directory *directory)
{
	ncm_directory_init(&m_directory);
	if (!ncm_directory_from_mpd_directory(
		    &m_directory, const_cast<mpd_directory *>(directory)))
		throw std::runtime_error("invalid mpd directory");
}

Directory::Directory(NcmDirectory *directory)
{
	ncm_directory_init(&m_directory);
	if (!ncm_directory_copy(&m_directory, directory))
		throw std::runtime_error("invalid directory");
}

Directory::Directory(std::string path_, time_t last_modified)
{
	ncm_directory_init(&m_directory);
	if (!ncm_directory_set(&m_directory,
	                       const_cast<char *>(path_.c_str()),
	                       static_cast<int32>(path_.size()),
	                       last_modified))
		throw std::runtime_error("empty path");
}

Directory::Directory(const Directory &rhs)
{
	ncm_directory_init(&m_directory);
	if (!ncm_directory_copy(&m_directory,
	                        const_cast<NcmDirectory *>(&rhs.m_directory)))
		throw std::bad_alloc();
}

Directory::Directory(Directory &&rhs) noexcept
: m_path_cache(std::move(rhs.m_path_cache))
{
	ncm_directory_init(&m_directory);
	ncm_directory_move(&m_directory, &rhs.m_directory);
}

Directory &Directory::operator=(const Directory &rhs)
{
	if (this != &rhs)
	{
		if (!ncm_directory_copy(&m_directory,
		                        const_cast<NcmDirectory *>(&rhs.m_directory)))
			throw std::bad_alloc();
		m_path_cache.clear();
	}
	return *this;
}

Directory &Directory::operator=(Directory &&rhs) noexcept
{
	if (this != &rhs)
	{
		ncm_directory_move(&m_directory, &rhs.m_directory);
		m_path_cache = std::move(rhs.m_path_cache);
	}
	return *this;
}

Directory::~Directory()
{
	ncm_directory_destroy(&m_directory);
}

bool Directory::operator==(const Directory &rhs) const
{
	return ncm_directory_equal(const_cast<NcmDirectory *>(&m_directory),
	                           const_cast<NcmDirectory *>(&rhs.m_directory));
}

const std::string &Directory::path() const
{
	NcmStringView path;

	if (ncm_directory_path_view(const_cast<NcmDirectory *>(&m_directory),
	                            &path))
		m_path_cache.assign(path.data, path.len);
	else
		m_path_cache.clear();
	return m_path_cache;
}

time_t Directory::lastModified() const
{
	return ncm_directory_last_modified(
		const_cast<NcmDirectory *>(&m_directory));
}

Playlist::Playlist()
{
	ncm_playlist_init(&m_playlist);
}

Playlist::Playlist(const mpd_playlist *playlist)
{
	ncm_playlist_init(&m_playlist);
	if (!ncm_playlist_from_mpd_playlist(
		    &m_playlist, const_cast<mpd_playlist *>(playlist)))
		throw std::runtime_error("invalid mpd playlist");
}

Playlist::Playlist(NcmPlaylist *playlist)
{
	ncm_playlist_init(&m_playlist);
	if (!ncm_playlist_copy(&m_playlist, playlist))
		throw std::runtime_error("invalid playlist");
}

Playlist::Playlist(std::string path_, time_t last_modified)
{
	ncm_playlist_init(&m_playlist);
	if (!ncm_playlist_set(&m_playlist,
	                      const_cast<char *>(path_.c_str()),
	                      static_cast<int32>(path_.size()),
	                      last_modified))
		throw std::runtime_error("empty path");
}

Playlist::Playlist(const Playlist &rhs)
{
	ncm_playlist_init(&m_playlist);
	if (!ncm_playlist_copy(&m_playlist,
	                       const_cast<NcmPlaylist *>(&rhs.m_playlist)))
		throw std::bad_alloc();
}

Playlist::Playlist(Playlist &&rhs) noexcept
: m_path_cache(std::move(rhs.m_path_cache))
{
	ncm_playlist_init(&m_playlist);
	ncm_playlist_move(&m_playlist, &rhs.m_playlist);
}

Playlist &Playlist::operator=(const Playlist &rhs)
{
	if (this != &rhs)
	{
		if (!ncm_playlist_copy(&m_playlist,
		                       const_cast<NcmPlaylist *>(&rhs.m_playlist)))
			throw std::bad_alloc();
		m_path_cache.clear();
	}
	return *this;
}

Playlist &Playlist::operator=(Playlist &&rhs) noexcept
{
	if (this != &rhs)
	{
		ncm_playlist_move(&m_playlist, &rhs.m_playlist);
		m_path_cache = std::move(rhs.m_path_cache);
	}
	return *this;
}

Playlist::~Playlist()
{
	ncm_playlist_destroy(&m_playlist);
}

bool Playlist::operator==(const Playlist &rhs) const
{
	return ncm_playlist_equal(const_cast<NcmPlaylist *>(&m_playlist),
	                          const_cast<NcmPlaylist *>(&rhs.m_playlist));
}

const std::string &Playlist::path() const
{
	NcmStringView path;

	if (ncm_playlist_path_view(const_cast<NcmPlaylist *>(&m_playlist), &path))
		m_path_cache.assign(path.data, path.len);
	else
		m_path_cache.clear();
	return m_path_cache;
}

time_t Playlist::lastModified() const
{
	return ncm_playlist_last_modified(const_cast<NcmPlaylist *>(&m_playlist));
}

Item::Item()
{
	ncm_mpd_item_init(&m_item);
}

Item::Item(mpd_entity *entity)
{
	ncm_mpd_item_init(&m_item);
	std::shared_ptr<mpd_entity> owner(entity, mpd_entity_free);
	if (!ncm_mpd_item_from_entity_copy(&m_item, owner.get()))
		throw std::runtime_error("unknown or invalid mpd_entity");
}

Item::Item(NcmMpdItem *item)
{
	ncm_mpd_item_init(&m_item);
	if (!ncm_mpd_item_copy(&m_item, item))
		throw std::runtime_error("invalid MPD item");
}

Item::Item(Directory directory_)
{
	ncm_mpd_item_init(&m_item);
	if (!ncm_mpd_item_set_directory(&m_item, directory_.cDirectory()))
		throw std::runtime_error("invalid directory item");
}

Item::Item(Song song_)
{
	ncm_mpd_item_init(&m_item);
	if (!ncm_mpd_item_set_song(&m_item, song_.cSong()))
		throw std::runtime_error("invalid song item");
}

Item::Item(Playlist playlist_)
{
	ncm_mpd_item_init(&m_item);
	if (!ncm_mpd_item_set_playlist(&m_item, playlist_.cPlaylist()))
		throw std::runtime_error("invalid playlist item");
}

Item::Item(const Item &rhs)
{
	ncm_mpd_item_init(&m_item);
	if (!ncm_mpd_item_copy(&m_item, const_cast<NcmMpdItem *>(&rhs.m_item)))
		throw std::bad_alloc();
}

Item::Item(Item &&rhs) noexcept
: m_directory_cache(std::move(rhs.m_directory_cache))
, m_song_cache(std::move(rhs.m_song_cache))
, m_playlist_cache(std::move(rhs.m_playlist_cache))
{
	ncm_mpd_item_init(&m_item);
	ncm_mpd_item_move(&m_item, &rhs.m_item);
}

Item &Item::operator=(const Item &rhs)
{
	if (this != &rhs)
	{
		if (!ncm_mpd_item_copy(&m_item,
		                       const_cast<NcmMpdItem *>(&rhs.m_item)))
			throw std::bad_alloc();
	}
	return *this;
}

Item &Item::operator=(Item &&rhs) noexcept
{
	if (this != &rhs)
	{
		ncm_mpd_item_move(&m_item, &rhs.m_item);
		m_directory_cache = std::move(rhs.m_directory_cache);
		m_song_cache = std::move(rhs.m_song_cache);
		m_playlist_cache = std::move(rhs.m_playlist_cache);
	}
	return *this;
}

Item::~Item()
{
	ncm_mpd_item_destroy(&m_item);
}

bool Item::operator==(const Item &rhs) const
{
	if (ncm_mpd_item_kind(const_cast<NcmMpdItem *>(&m_item))
	    == NCM_MPD_ITEM_UNKNOWN)
		throw std::runtime_error("unknown item type");
	if (ncm_mpd_item_kind(const_cast<NcmMpdItem *>(&rhs.m_item))
	    == NCM_MPD_ITEM_UNKNOWN)
		throw std::runtime_error("unknown item type");
	return ncm_mpd_item_equal(const_cast<NcmMpdItem *>(&m_item),
	                          const_cast<NcmMpdItem *>(&rhs.m_item));
}

Item::Type Item::type() const
{
	switch (ncm_mpd_item_kind(const_cast<NcmMpdItem *>(&m_item)))
	{
		case NCM_MPD_ITEM_DIRECTORY:
			return Type::Directory;
		case NCM_MPD_ITEM_SONG:
			return Type::Song;
		case NCM_MPD_ITEM_PLAYLIST:
			return Type::Playlist;
		case NCM_MPD_ITEM_UNKNOWN:
			throw std::runtime_error("unknown item type");
	}
	throw std::runtime_error("unknown item type");
}

const Directory &Item::directory() const
{
	NcmDirectory *directory;

	directory = ncm_mpd_item_directory(const_cast<NcmMpdItem *>(&m_item));
	assert(directory != nullptr);
	m_directory_cache = Directory(directory);
	return m_directory_cache;
}

const Song &Item::song() const
{
	NcmSong *song;

	song = ncm_mpd_item_song(const_cast<NcmMpdItem *>(&m_item));
	assert(song != nullptr);
	m_song_cache = Song(song);
	return m_song_cache;
}

const Playlist &Item::playlist() const
{
	NcmPlaylist *playlist;

	playlist = ncm_mpd_item_playlist(const_cast<NcmMpdItem *>(&m_item));
	assert(playlist != nullptr);
	m_playlist_cache = Playlist(playlist);
	return m_playlist_cache;
}

void checkConnectionErrors(mpd_connection *conn)
{
	mpd_error code = mpd_connection_get_error(conn);
	if (code != MPD_ERROR_SUCCESS)
	{
		std::string msg = mpd_connection_get_error_message(conn);
		if (code == MPD_ERROR_SERVER)
		{
			mpd_server_error server_code = mpd_connection_get_server_error(conn);
			bool clearable = mpd_connection_clear_error(conn);
			throw ServerError(server_code, msg, clearable);
		}
		else
		{
			bool clearable = mpd_connection_clear_error(conn);
			throw ClientError(code, msg, clearable);
		}
	}
}

namespace {


const char *replayGainModeName(NcmMpdReplayGainMode mode)
{
	switch (mode)
	{
		case NCM_MPD_REPLAY_GAIN_OFF:
			return "off";
		case NCM_MPD_REPLAY_GAIN_TRACK:
			return "track";
		case NCM_MPD_REPLAY_GAIN_ALBUM:
			return "album";
	}
	return "off";
}

NcmMpdReplayGainMode replayGainModeFromLegacy(MPD::ReplayGainMode mode)
{
	switch (mode)
	{
		case MPD::rgmOff:
			return NCM_MPD_REPLAY_GAIN_OFF;
		case MPD::rgmTrack:
			return NCM_MPD_REPLAY_GAIN_TRACK;
		case MPD::rgmAlbum:
			return NCM_MPD_REPLAY_GAIN_ALBUM;
	}
	return NCM_MPD_REPLAY_GAIN_OFF;
}

void throwClientError(const NcmError &error)
{
	mpd_error code = static_cast<mpd_error>(error.code);
	std::string msg = error.message;

	if (code == MPD_ERROR_SUCCESS)
		code = ncm_mpd_client_error_code(&global_mpd);
	if (msg.empty())
		msg = ncm_mpd_client_error_message(&global_mpd);
	if (msg.empty())
		msg = "MPD command failed";

	if (code == MPD_ERROR_SERVER)
	{
		throw MPD::ServerError(
			ncm_mpd_client_server_error_code(&global_mpd),
			msg,
			ncm_mpd_client_error_clearable(&global_mpd));
	}
	throw MPD::ClientError(
		code,
		msg,
		ncm_mpd_client_error_clearable(&global_mpd));
}

void throwIfFailed(bool ok, const NcmError &error)
{
	if (!ok)
		throwClientError(error);
}

void noidleThunk(int32 flags, void *user)
{
	auto connection = static_cast<MPD::Connection *>(user);
	if (connection != nullptr)
		connection->dispatchNoidleCallback(flags);
}

}

Connection::Connection()
: m_port(6600)
, m_timeout(15)
{ }

Connection::~Connection()
{ }

void Connection::dispatchNoidleCallback(int flags)
{
	if (m_noidle_callback)
		m_noidle_callback(flags);
}

mpd_connection *Connection::rawConnection() const
{
	return ncm_mpd_connection_mpd(
		ncm_mpd_client_connection(&global_mpd));
}

void Connection::throwConnectionError() const
{
	NcmError error{};

	ncm_error_set(&error,
	              static_cast<int32>(ncm_mpd_client_error_code(&global_mpd)),
	              ncm_mpd_client_error_message(&global_mpd),
	              static_cast<int32>(strlen(ncm_mpd_client_error_message(
		              &global_mpd))));
	throwClientError(error);
}

void Connection::Connect()
{
	NcmError error{};

	assert(!Connected());
	try
	{
		throwIfFailed(ncm_mpd_client_connect(&global_mpd, &error), error);
	}
	catch (MPD::ClientError &e)
	{
		Disconnect();
		throw e;
	}
}

bool Connection::Connected() const
{
	return ncm_mpd_client_connected(&global_mpd);
}

void Connection::Disconnect()
{
	ncm_mpd_client_disconnect(&global_mpd);
}

const std::string &Connection::GetHostname()
{
	m_host = ncm_mpd_client_hostname(&global_mpd);
	return m_host;
}

int Connection::GetPort()
{
	return ncm_mpd_client_port(&global_mpd);
}

unsigned Connection::Version() const
{
	return ncm_mpd_client_version(&global_mpd);
}

int Connection::GetFD() const
{
	return ncm_mpd_client_fd(&global_mpd);
}

void Connection::SetHostname(const std::string &host)
{
	NcmError error{};

	throwIfFailed(ncm_mpd_client_set_hostname(
		&global_mpd,
		const_cast<char *>(host.c_str()),
		static_cast<int32>(host.size()),
		&error), error);
	m_host = ncm_mpd_client_hostname(&global_mpd);
}

void Connection::SetPort(int port)
{
	ncm_mpd_client_set_port(&global_mpd, static_cast<uint16>(port));
	m_port = port;
}

void Connection::SetTimeout(int timeout)
{
	NcmError error{};

	m_timeout = timeout;
	throwIfFailed(ncm_mpd_client_set_timeout_ms(
		&global_mpd,
		static_cast<uint32>(timeout * 1000),
		&error), error);
}

void Connection::SetPassword(const std::string &password)
{
	NcmError error{};

	throwIfFailed(ncm_mpd_client_set_password(
		&global_mpd,
		const_cast<char *>(password.c_str()),
		static_cast<int32>(password.size()),
		&error), error);
	m_password = password;
}

void Connection::SendPassword()
{
	NcmError error{};

	throwIfFailed(ncm_mpd_client_send_password(&global_mpd, &error), error);
}

void Connection::idle()
{
	NcmError error{};

	throwIfFailed(ncm_mpd_client_idle(&global_mpd, &error), error);
}

int Connection::noidle()
{
	NcmError error{};
	int32 flags = 0;

	throwIfFailed(ncm_mpd_client_noidle(&global_mpd, &flags, &error), error);
	return flags;
}

void Connection::setNoidleCallback(NoidleCallback callback)
{
	m_noidle_callback = std::move(callback);
	ncm_mpd_client_set_noidle_callback(&global_mpd, noidleThunk, this);
}

Statistics Connection::getStatistics()
{
	NcmError error{};
	NcmMpdStats stats{};

	throwIfFailed(ncm_mpd_client_get_stats(&global_mpd, &stats, &error),
	              error);
	return Statistics(stats);
}

Status Connection::getStatus()
{
	NcmError error{};
	NcmMpdStatus status{};

	throwIfFailed(ncm_mpd_client_get_status(&global_mpd, &status, &error),
	              error);
	return Status(status);
}

void Connection::UpdateDirectory(const std::string &path)
{
	NcmError error{};

	throwIfFailed(ncm_mpd_client_update_directory(
		&global_mpd, const_cast<char *>(path.c_str()), nullptr, &error),
		error);
}

void Connection::Play()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_play(&global_mpd, &error), error);
}

void Connection::Play(int pos)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_play_pos(&global_mpd, pos, &error), error);
}

void Connection::PlayID(int id)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_play_id(&global_mpd, id, &error), error);
}

void Connection::Pause(bool state)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_pause(&global_mpd, state, &error), error);
}

void Connection::Toggle()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_toggle_pause(&global_mpd, &error), error);
}

void Connection::Stop()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_stop(&global_mpd, &error), error);
}

void Connection::Next()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_next(&global_mpd, &error), error);
}

void Connection::Prev()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_previous(&global_mpd, &error), error);
}

void Connection::Move(unsigned from, unsigned to)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_move(&global_mpd, from, to, &error), error);
}

void Connection::Swap(unsigned from, unsigned to)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_swap(&global_mpd, from, to, &error), error);
}

void Connection::Seek(unsigned pos, unsigned where)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_seek_pos(&global_mpd, pos, where, &error),
	              error);
}

void Connection::Shuffle()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_shuffle(&global_mpd, &error), error);
}

void Connection::ShuffleRange(unsigned start, unsigned end)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_shuffle_range(
		&global_mpd, start, end, &error), error);
}

void Connection::ClearMainPlaylist()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_clear_queue(&global_mpd, &error), error);
}

SongIterator Connection::GetPlaylistChanges(unsigned version)
{
	NcmError error{};
	NcmMpdSongListGuard songs;

	throwIfFailed(ncm_mpd_client_get_queue_changes(
		&global_mpd, version, songs.get(), &error), error);
	return SongIterator(songVectorFromList(songs.get()));
}

Song Connection::GetCurrentSong()
{
	NcmError error{};
	NcmSongGuard song;

	throwIfFailed(ncm_mpd_client_get_current_song(
		&global_mpd, song.get(), &error), error);
	if (ncm_song_empty(song.get()))
		return Song();
	return Song(song.get());
}

Song Connection::GetSong(const std::string &path)
{
	NcmError error{};
	NcmSongGuard song;

	throwIfFailed(ncm_mpd_client_get_song(
		&global_mpd, const_cast<char *>(path.c_str()), song.get(), &error),
		error);
	return Song(song.get());
}

SongIterator Connection::GetPlaylistContent(const std::string &path)
{
	NcmError error{};
	NcmMpdSongListGuard songs;

	throwIfFailed(ncm_mpd_client_get_playlist_content(
		&global_mpd, const_cast<char *>(path.c_str()), songs.get(), &error),
		error);
	return SongIterator(songVectorFromList(songs.get()));
}

SongIterator Connection::GetPlaylistContentNoInfo(const std::string &path)
{
	NcmError error{};
	NcmMpdSongListGuard songs;

	throwIfFailed(ncm_mpd_client_get_playlist_content_no_info(
		&global_mpd, const_cast<char *>(path.c_str()), songs.get(), &error),
		error);
	return SongIterator(songVectorFromList(songs.get()));
}

StringIterator Connection::GetSupportedExtensions()
{
	NcmError error{};
	NcmMpdStringListGuard strings;

	throwIfFailed(ncm_mpd_client_get_supported_extensions(
		&global_mpd, strings.get(), &error), error);
	return StringIterator(stringVectorFromList(strings.get()));
}

void Connection::SetRepeat(bool mode)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_set_repeat(&global_mpd, mode, &error),
	              error);
}

void Connection::SetRandom(bool mode)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_set_random(&global_mpd, mode, &error),
	              error);
}

void Connection::SetSingle(bool mode)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_set_single(&global_mpd, mode, &error),
	              error);
}

void Connection::SetConsume(bool mode)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_set_consume(&global_mpd, mode, &error),
	              error);
}

void Connection::SetCrossfade(unsigned seconds)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_set_crossfade(&global_mpd, seconds, &error),
	              error);
}

void Connection::SetVolume(unsigned volume)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_set_volume(&global_mpd, volume, &error),
	              error);
}

void Connection::ChangeVolume(int change)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_change_volume(&global_mpd, change, &error),
	              error);
}

std::string Connection::GetReplayGainMode()
{
	NcmError error{};
	NcmMpdReplayGainMode mode;

	throwIfFailed(ncm_mpd_client_get_replay_gain_mode(
		&global_mpd, &mode, &error), error);
	return replayGainModeName(mode);
}

void Connection::SetReplayGainMode(ReplayGainMode mode)
{
	NcmError error{};

	throwIfFailed(ncm_mpd_client_set_replay_gain_mode(
		&global_mpd, replayGainModeFromLegacy(mode), &error), error);
}

void Connection::SetPriority(const MPD::Song &s, int prio)
{
	NcmError error{};

	throwIfFailed(ncm_mpd_client_set_priority_id(
		&global_mpd, s.getID(), prio, &error), error);
}

int Connection::AddSong(const std::string &path, int pos)
{
	NcmError error{};
	int32 id = -1;

	throwIfFailed(ncm_mpd_client_add_song(
		&global_mpd, const_cast<char *>(path.c_str()), pos, &id, &error),
		error);
	return id;
}

int Connection::AddSong(const Song &s, int pos)
{
	return AddSong((!s.isFromDatabase() ? "file://" : "") + s.getURI(), pos);
}

bool Connection::AddRandomTag(mpd_tag_type tag, size_t number, NcmRandom *rng)
{
	NcmError error{};

	return ncm_mpd_client_add_random_tag(
		&global_mpd, tag, static_cast<int32>(number), rng, &error);
}

bool Connection::AddRandomSongs(size_t number,
                                const std::string &random_exclude_pattern,
                                NcmRandom *rng)
{
	NcmError error{};

	return ncm_mpd_client_add_random_songs(
		&global_mpd,
		static_cast<int32>(number),
		const_cast<char *>(random_exclude_pattern.c_str()),
		static_cast<int32>(random_exclude_pattern.size()),
		rng,
		&error);
}

bool Connection::Add(const std::string &path)
{
	NcmError error{};
	bool added = false;

	throwIfFailed(ncm_mpd_client_add(
		&global_mpd, const_cast<char *>(path.c_str()), &added, &error),
		error);
	return added;
}

void Connection::Delete(unsigned pos)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_delete(&global_mpd, pos, &error), error);
}

void Connection::DeleteRange(unsigned begin, unsigned end)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_delete_range(
		&global_mpd, begin, end, &error), error);
}

void Connection::PlaylistDelete(const std::string &playlist, unsigned pos)
{
	NcmError error{};

	throwIfFailed(ncm_mpd_client_playlist_delete(
		&global_mpd, const_cast<char *>(playlist.c_str()), pos, &error),
		error);
}

void Connection::StartCommandsList()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_start_command_list(&global_mpd, &error),
	              error);
}

void Connection::CommitCommandsList()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_commit_command_list(&global_mpd, &error),
	              error);
}

void Connection::DeletePlaylist(const std::string &name)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_delete_playlist(
		&global_mpd, const_cast<char *>(name.c_str()), &error), error);
}

bool Connection::LoadPlaylist(const std::string &name)
{
	NcmError error{};
	bool loaded = false;

	throwIfFailed(ncm_mpd_client_load_playlist(
		&global_mpd, const_cast<char *>(name.c_str()), &loaded, &error),
		error);
	return loaded;
}

void Connection::SavePlaylist(const std::string &name)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_save_playlist(
		&global_mpd, const_cast<char *>(name.c_str()), &error), error);
}

void Connection::ClearPlaylist(const std::string &playlist)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_clear_playlist(
		&global_mpd, const_cast<char *>(playlist.c_str()), &error), error);
}

void Connection::AddToPlaylist(const std::string &path, const Song &s)
{
	AddToPlaylist(path, s.getURI());
}

void Connection::AddToPlaylist(const std::string &path,
                               const std::string &file)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_add_to_playlist(
		&global_mpd,
		const_cast<char *>(path.c_str()),
		const_cast<char *>(file.c_str()),
		&error), error);
}

void Connection::PlaylistMove(const std::string &path, int from, int to)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_playlist_move(
		&global_mpd,
		const_cast<char *>(path.c_str()),
		static_cast<uint32>(from),
		static_cast<uint32>(to),
		&error), error);
}

void Connection::Rename(const std::string &from, const std::string &to)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_rename_playlist(
		&global_mpd,
		const_cast<char *>(from.c_str()),
		const_cast<char *>(to.c_str()),
		&error), error);
}

void Connection::StartSearch(bool exact_match)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_start_search(
		&global_mpd, exact_match, &error), error);
}

void Connection::StartFieldSearch(mpd_tag_type item)
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_start_field_search(
		&global_mpd, item, &error), error);
}

void Connection::AddSearch(mpd_tag_type item, const std::string &str) const
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_add_search_tag(
		&global_mpd, item, const_cast<char *>(str.c_str()), &error), error);
}

void Connection::AddSearchAny(const std::string &str) const
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_add_search_any(
		&global_mpd, const_cast<char *>(str.c_str()), &error), error);
}

void Connection::AddSearchURI(const std::string &str) const
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_add_search_uri(
		&global_mpd, const_cast<char *>(str.c_str()), &error), error);
}

SongIterator Connection::CommitSearchSongs()
{
	NcmError error{};
	NcmMpdSongListGuard songs;

	throwIfFailed(ncm_mpd_client_commit_search_songs(
		&global_mpd, songs.get(), &error), error);
	return SongIterator(songVectorFromList(songs.get()));
}


StringIterator Connection::GetList(mpd_tag_type type)
{
	NcmError error{};
	NcmMpdStringListGuard strings;

	throwIfFailed(ncm_mpd_client_get_list(
		&global_mpd, type, strings.get(), &error), error);
	return StringIterator(stringVectorFromList(strings.get()));
}

ItemIterator Connection::GetDirectory(const std::string &directory)
{
	NcmError error{};
	NcmMpdItemListGuard items;

	throwIfFailed(ncm_mpd_client_get_directory(
		&global_mpd, const_cast<char *>(directory.c_str()), items.get(),
		&error), error);
	return ItemIterator(itemVectorFromList(items.get()));
}

SongIterator Connection::GetDirectoryRecursive(const std::string &directory)
{
	NcmError error{};
	NcmMpdSongListGuard songs;

	throwIfFailed(ncm_mpd_client_get_directory_recursive(
		&global_mpd, const_cast<char *>(directory.c_str()), songs.get(),
		&error), error);
	return SongIterator(songVectorFromList(songs.get()));
}

DirectoryIterator Connection::GetDirectories(const std::string &directory)
{
	NcmError error{};
	NcmMpdItemListGuard items;

	throwIfFailed(ncm_mpd_client_get_directories(
		&global_mpd, const_cast<char *>(directory.c_str()), items.get(),
		&error), error);
	return DirectoryIterator(directoryVectorFromItemList(items.get()));
}

SongIterator Connection::GetSongs(const std::string &directory)
{
	NcmError error{};
	NcmMpdSongListGuard songs;

	throwIfFailed(ncm_mpd_client_get_songs(
		&global_mpd, const_cast<char *>(directory.c_str()), songs.get(),
		&error), error);
	return SongIterator(songVectorFromList(songs.get()));
}


StringIterator Connection::GetURLHandlers()
{
	NcmError error{};
	NcmMpdStringListGuard strings;

	throwIfFailed(ncm_mpd_client_get_url_handlers(
		&global_mpd, strings.get(), &error), error);
	return StringIterator(stringVectorFromList(strings.get()));
}

StringIterator Connection::GetTagTypes()
{
	NcmError error{};
	NcmMpdStringListGuard strings;

	throwIfFailed(ncm_mpd_client_get_tag_types(
		&global_mpd, strings.get(), &error), error);
	return StringIterator(stringVectorFromList(strings.get()));
}

void Connection::checkConnection() const
{
	if (!Connected())
		throw ClientError(MPD_ERROR_STATE, "No active MPD connection", false);
}

void Connection::prechecks()
{
	NcmError error{};
	throwIfFailed(ncm_mpd_client_noidle(&global_mpd, nullptr, &error), error);
}

void Connection::prechecksNoCommandsList()
{
	prechecks();
}

void Connection::checkErrors() const
{
	NcmMpdConnection *connection;

	connection = ncm_mpd_client_connection(&global_mpd);
	if (!ncm_mpd_connection_check_error(connection))
		throwConnectionError();
}

}
