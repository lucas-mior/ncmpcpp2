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
#include <algorithm>
#include <map>
#include <memory>
#include <new>
#include <regex>

#include "charset.h"
#include "c/ncm_mpd_item.h"
#include "mpdpp.h"

MPD::Connection Mpd;

namespace {

const char *mpdDirectory(const std::string &directory)
{
	// MPD <= 0.19 accepts "/" for a root directory whereas later
	// versions do not, so provide a compatibility layer.
	if (directory == "/")
		return "";
	else
		return directory.c_str();
}

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

bool fetchItemSong(MPD::SongIterator::State &state)
{
	auto src = mpd_recv_entity(state.connection());
	while (src != nullptr && mpd_entity_get_type(src) != MPD_ENTITY_TYPE_SONG)
	{
		mpd_entity_free(src);
		src = mpd_recv_entity(state.connection());
	}
	if (src != nullptr)
	{
		std::shared_ptr<mpd_entity> owner(src, mpd_entity_free);
		const mpd_song *song = mpd_entity_get_song(src);
		if (song == nullptr)
			throw std::runtime_error("song entity without song");
		state.setObject(MPD::Song(song, std::move(owner)));
		return true;
	}
	else
		return false;
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

Connection::Connection()
: m_command_list_active(false)
, m_fd(-1)
, m_idle(false)
, m_host("localhost")
, m_port(6600)
, m_timeout(15)
{
	ncm_mpd_connection_init(&m_connection);
}

Connection::~Connection()
{
	ncm_mpd_connection_destroy(&m_connection);
}

mpd_connection *Connection::rawConnection() const
{
	return ncm_mpd_connection_mpd(
		const_cast<NcmMpdConnection *>(&m_connection));
}

void Connection::throwConnectionError() const
{
	NcmMpdConnection *connection;
	mpd_error code;
	std::string msg;
	bool clearable;

	connection = const_cast<NcmMpdConnection *>(&m_connection);
	code = ncm_mpd_connection_error_code(connection);
	msg = ncm_mpd_connection_error(connection);
	clearable = ncm_mpd_connection_error_clearable(connection);

	if (code == MPD_ERROR_SERVER)
	{
		throw ServerError(
			ncm_mpd_connection_server_error_code(connection),
			msg,
			clearable);
	}
	else
	{
		throw ClientError(code, msg, clearable);
	}
}

void Connection::Connect()
{
	assert(!Connected());
	try
	{
		if (!ncm_mpd_connection_connect(
			    &m_connection,
			    const_cast<char *>(m_host.c_str()),
			    static_cast<uint16>(m_port),
			    static_cast<uint32>(m_timeout * 1000)))
			throwConnectionError();
		if (!m_password.empty())
			SendPassword();
		m_fd = ncm_mpd_connection_fd(&m_connection);
		checkErrors();
	}
	catch (MPD::ClientError &e)
	{
		Disconnect();
		throw e;
	}
}

bool Connection::Connected() const
{
	return ncm_mpd_connection_is_connected(
		const_cast<NcmMpdConnection *>(&m_connection));
}

void Connection::Disconnect()
{
	ncm_mpd_connection_disconnect(&m_connection);
	m_command_list_active = false;
	m_idle = false;
	m_fd = -1;
}

unsigned Connection::Version() const
{
	return Connected() ? mpd_connection_get_server_version(rawConnection())[1] : 0;
}

void Connection::SetHostname(const std::string &host)
{
	size_t at = host.find("@");
	if (at != 0 && at != std::string::npos)
	{
		m_password = host.substr(0, at);
		m_host = host.substr(at+1);
	}
	else
		m_host = host;
}

void Connection::SendPassword()
{
	assert(Connected());
	noidle();
	assert(!m_command_list_active);
	mpd_run_password(rawConnection(), m_password.c_str());
	checkErrors();
}

void Connection::idle()
{
	checkConnection();
	if (!m_idle)
	{
		mpd_send_idle(rawConnection());
		checkErrors();
	}
	m_idle = true;
}

int Connection::noidle()
{
	checkConnection();
	int flags = 0;
	if (m_idle && mpd_send_noidle(rawConnection()))
	{
		m_idle = false;
		flags = mpd_recv_idle(rawConnection(), true);
		mpd_response_finish(rawConnection());
		checkErrors();
	}
	return flags;
}

void Connection::setNoidleCallback(NoidleCallback callback)
{
	m_noidle_callback = std::move(callback);
}

Statistics Connection::getStatistics()
{
	NcmMpdStats stats{};

	prechecks();
	if (!ncm_mpd_connection_get_stats(&m_connection, &stats))
		throwConnectionError();
	return Statistics(stats);
}

Status Connection::getStatus()
{
	NcmMpdStatus status{};

	prechecks();
	if (!ncm_mpd_connection_get_status(&m_connection, &status))
		throwConnectionError();
	return Status(status);
}

void Connection::UpdateDirectory(const std::string &path)
{
	prechecksNoCommandsList();
	// Use update as mpd_run_update doesn't call mpd_response_finish if the id
	// returned from mpd_recv_update_id is 0 which breaks mopidy.
	mpd_send_update(rawConnection(), path.c_str());
	mpd_recv_update_id(rawConnection());
	mpd_response_finish(rawConnection());
	checkErrors();
}

void Connection::Play()
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_play(&m_connection))
		throwConnectionError();
}

void Connection::Play(int pos)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_play_pos(&m_connection, pos))
		throwConnectionError();
}

void Connection::PlayID(int id)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_play_id(&m_connection, id))
		throwConnectionError();
}

void Connection::Pause(bool state)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_pause(&m_connection, state))
		throwConnectionError();
}

void Connection::Toggle()
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_toggle_pause(&m_connection))
		throwConnectionError();
}

void Connection::Stop()
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_stop(&m_connection))
		throwConnectionError();
}

void Connection::Next()
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_next(&m_connection))
		throwConnectionError();
}

void Connection::Prev()
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_previous(&m_connection))
		throwConnectionError();
}

void Connection::Move(unsigned from, unsigned to)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_move(rawConnection(), from, to);
	else
	{
		mpd_run_move(rawConnection(), from, to);
		checkErrors();
	}
}

void Connection::Swap(unsigned from, unsigned to)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_swap(rawConnection(), from, to);
	else
	{
		mpd_run_swap(rawConnection(), from, to);
		checkErrors();
	}
}

void Connection::Seek(unsigned pos, unsigned where)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_seek_pos(&m_connection, pos, where))
		throwConnectionError();
}

void Connection::Shuffle()
{
	prechecksNoCommandsList();
	mpd_run_shuffle(rawConnection());
	checkErrors();
}

void Connection::ShuffleRange(unsigned start, unsigned end)
{
	prechecksNoCommandsList();
	mpd_run_shuffle_range(rawConnection(), start, end);
	checkErrors();
}

void Connection::ClearMainPlaylist()
{
	prechecksNoCommandsList();
	mpd_run_clear(rawConnection());
	checkErrors();
}

void Connection::ClearPlaylist(const std::string &playlist)
{
	prechecksNoCommandsList();
	mpd_run_playlist_clear(rawConnection(), playlist.c_str());
	checkErrors();
}

void Connection::AddToPlaylist(const std::string &path, const Song &s)
{
	AddToPlaylist(path, s.getURI());
}

void Connection::AddToPlaylist(const std::string &path, const std::string &file)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_playlist_add(rawConnection(), path.c_str(), file.c_str());
	else
	{
		mpd_run_playlist_add(rawConnection(), path.c_str(), file.c_str());
		checkErrors();
	}
}

void Connection::PlaylistMove(const std::string &path, int from, int to)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_playlist_move(rawConnection(), path.c_str(), from, to);
	else
	{
		mpd_send_playlist_move(rawConnection(), path.c_str(), from, to);
		mpd_response_finish(rawConnection());
		checkErrors();
	}
}

void Connection::Rename(const std::string &from, const std::string &to)
{
	prechecksNoCommandsList();
	mpd_run_rename(rawConnection(), from.c_str(), to.c_str());
	checkErrors();
}

SongIterator Connection::GetPlaylistChanges(unsigned version)
{
	prechecksNoCommandsList();
	mpd_send_queue_changes_meta(rawConnection(), version);
	checkErrors();
	return SongIterator(rawConnection(), defaultFetcher<Song>(mpd_recv_song));
}

Song Connection::GetCurrentSong()
{
	prechecksNoCommandsList();
	mpd_send_current_song(rawConnection());
	mpd_song *s = mpd_recv_song(rawConnection());
	mpd_response_finish(rawConnection());
	checkErrors();
	// currentsong doesn't return error if there is no playing song.
	if (s == nullptr)
		return Song();
	else
		return Song(s);
}

Song Connection::GetSong(const std::string &path)
{
	prechecksNoCommandsList();
	mpd_send_list_all_meta(rawConnection(), path.c_str());
	mpd_song *s = mpd_recv_song(rawConnection());
	mpd_response_finish(rawConnection());
	checkErrors();
	return Song(s);
}

SongIterator Connection::GetPlaylistContent(const std::string &path)
{
	prechecksNoCommandsList();
	mpd_send_list_playlist_meta(rawConnection(), path.c_str());
	SongIterator result(rawConnection(), defaultFetcher<Song>(mpd_recv_song));
	checkErrors();
	return result;
}

SongIterator Connection::GetPlaylistContentNoInfo(const std::string &path)
{
	prechecksNoCommandsList();
	mpd_send_list_playlist(rawConnection(), path.c_str());
	SongIterator result(rawConnection(), defaultFetcher<Song>(mpd_recv_song));
	checkErrors();
	return result;
}

StringIterator Connection::GetSupportedExtensions()
{
	prechecksNoCommandsList();
	mpd_send_command(rawConnection(), "decoders", NULL);
	checkErrors();
	return StringIterator(rawConnection(), [](StringIterator::State &state) {
		auto src = mpd_recv_pair_named(state.connection(), "suffix");
		if (src != nullptr)
		{
			state.setObject(src->value);
			mpd_return_pair(state.connection(), src);
			return true;
		}
		else
			return false;
	});
}

void Connection::SetRepeat(bool mode)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_set_repeat(&m_connection, mode))
		throwConnectionError();
}

void Connection::SetRandom(bool mode)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_set_random(&m_connection, mode))
		throwConnectionError();
}

void Connection::SetSingle(bool mode)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_set_single(&m_connection, mode))
		throwConnectionError();
}

void Connection::SetConsume(bool mode)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_set_consume(&m_connection, mode))
		throwConnectionError();
}

void Connection::SetVolume(unsigned vol)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_set_volume(&m_connection, vol))
		throwConnectionError();
}

void Connection::ChangeVolume(int change)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_change_volume(&m_connection, change))
		throwConnectionError();
}


std::string Connection::GetReplayGainMode()
{
	prechecksNoCommandsList();
	mpd_send_command(rawConnection(), "replay_gain_status", NULL);
	std::string result;
	if (mpd_pair *pair = mpd_recv_pair_named(rawConnection(), "replay_gain_mode"))
	{
		result = pair->value;
		mpd_return_pair(rawConnection(), pair);
	}
	mpd_response_finish(rawConnection());
	checkErrors();
	return result;
}

void Connection::SetReplayGainMode(ReplayGainMode mode)
{
	prechecksNoCommandsList();
	const char *rg_mode;
	switch (mode)
	{
		case rgmOff:
			rg_mode = "off";
			break;
		case rgmTrack:
			rg_mode = "track";
			break;
		case rgmAlbum:
			rg_mode = "album";
			break;
		default:
			rg_mode = "";
			break;
	}
	mpd_send_command(rawConnection(), "replay_gain_mode", rg_mode, NULL);
	mpd_response_finish(rawConnection());
	checkErrors();
}

void Connection::SetCrossfade(unsigned crossfade)
{
	prechecksNoCommandsList();
	if (!ncm_mpd_connection_set_crossfade(&m_connection, crossfade))
		throwConnectionError();
}

void Connection::SetPriority(const Song &s, int prio)
{
	prechecks();
	if (m_command_list_active)
		mpd_send_prio_id(rawConnection(), prio, s.getID());
	else
	{
		mpd_run_prio_id(rawConnection(), prio, s.getID());
		checkErrors();
	}
}

int Connection::AddSong(const std::string &path, int pos)
{
	prechecks();
	int id;
	if (pos < 0)
		mpd_send_add_id(rawConnection(), path.c_str());
	else
		mpd_send_add_id_to(rawConnection(), path.c_str(), pos);
	if (!m_command_list_active)
	{
		id = mpd_recv_song_id(rawConnection());
		mpd_response_finish(rawConnection());
		checkErrors();
	}
	else
		id = 0;
	return id;
}

int Connection::AddSong(const Song &s, int pos)
{
	return AddSong((!s.isFromDatabase() ? "file://" : "") + s.getURI(), pos);
}

bool Connection::Add(const std::string &path)
{
	bool result;
	prechecks();
	if (m_command_list_active)
		result = mpd_send_add(rawConnection(), path.c_str());
	else
	{
		result = mpd_run_add(rawConnection(), path.c_str());
		checkErrors();
	}
	return result;
}

bool Connection::AddRandomTag(mpd_tag_type tag, size_t number, std::mt19937 &rng)
{
	std::vector<std::string> tags(
		std::make_move_iterator(GetList(tag)),
		std::make_move_iterator(StringIterator())
	);
	if (number > tags.size())
		return false;

	std::shuffle(tags.begin(), tags.end(), rng);
	auto it = tags.begin();
	for (size_t i = 0; i < number && it != tags.end(); ++i)
	{
		StartSearch(true);
		AddSearch(tag, *it++);
		std::vector<std::string> paths;
		MPD::SongIterator s = CommitSearchSongs(), end;
		for (; s != end; ++s)
			paths.push_back(s->getURI());
		StartCommandsList();
		for (const auto &path : paths)
			AddSong(path);
		CommitCommandsList();
	}
	return true;
}

bool Connection::AddRandomSongs(size_t number, const std::string &random_exclude_pattern, std::mt19937 &rng)
{
	prechecksNoCommandsList();
	std::vector<std::string> files;
	mpd_send_list_all(rawConnection(), "/");
	while (mpd_pair *item = mpd_recv_pair_named(rawConnection(), "file"))
	{
		files.push_back(item->value);
		mpd_return_pair(rawConnection(), item);
	}
	mpd_response_finish(rawConnection());
	checkErrors();
	
	if (number > files.size())
	{
		//if (itsErrorHandler)
		//	itsErrorHandler(this, 0, "Requested number of random songs is bigger than size of your library", itsErrorHandlerUserdata);
		return false;
	}
	else
	{
		std::shuffle(files.begin(), files.end(), rng);
		StartCommandsList();
		auto it = files.begin();
		std::regex re(random_exclude_pattern);
		for (size_t i = 0; i < number && it != files.end(); ++it) {
			if (random_exclude_pattern.empty() || !std::regex_match((*it), re)) {
				AddSong(*it);
				i++;
			}
		}
		CommitCommandsList();
	}
	return true;
}

void Connection::Delete(unsigned pos)
{
	prechecks();
	mpd_send_delete(rawConnection(), pos);
	if (!m_command_list_active)
	{
		mpd_response_finish(rawConnection());
		checkErrors();
	}
}

void Connection::DeleteRange(unsigned begin, unsigned end)
{
	prechecks();
	mpd_send_delete_range(rawConnection(), begin, end);
	if (!m_command_list_active)
	{
		mpd_response_finish(rawConnection());
		checkErrors();
	}
}

void Connection::PlaylistDelete(const std::string &playlist, unsigned pos)
{
	prechecks();
	mpd_send_playlist_delete(rawConnection(), playlist.c_str(), pos);
	if (!m_command_list_active)
	{
		mpd_response_finish(rawConnection());
		checkErrors();
	}
}

void Connection::StartCommandsList()
{
	prechecksNoCommandsList();
	mpd_command_list_begin(rawConnection(), true);
	m_command_list_active = true;
	checkErrors();
}

void Connection::CommitCommandsList()
{
	prechecks();
	assert(m_command_list_active);
	mpd_command_list_end(rawConnection());
	mpd_response_finish(rawConnection());
	m_command_list_active = false;
	checkErrors();
}

void Connection::DeletePlaylist(const std::string &name)
{
	prechecksNoCommandsList();
	mpd_run_rm(rawConnection(), name.c_str());
	checkErrors();
}

bool Connection::LoadPlaylist(const std::string &name)
{
	prechecksNoCommandsList();
	bool result = mpd_run_load(rawConnection(), name.c_str());
	checkErrors();
	return result;
}

void Connection::SavePlaylist(const std::string &name)
{
	prechecksNoCommandsList();
	mpd_send_save(rawConnection(), name.c_str());
	mpd_response_finish(rawConnection());
	checkErrors();
}

PlaylistIterator Connection::GetPlaylists()
{
	prechecksNoCommandsList();
	mpd_send_list_playlists(rawConnection());
	checkErrors();
	return PlaylistIterator(rawConnection(), defaultFetcher<Playlist>(mpd_recv_playlist));
}

StringIterator Connection::GetList(mpd_tag_type type)
{
	prechecksNoCommandsList();
	mpd_search_db_tags(rawConnection(), type);
	mpd_search_commit(rawConnection());
	checkErrors();
	return StringIterator(rawConnection(), [type](StringIterator::State &state) {
		auto src = mpd_recv_pair_tag(state.connection(), type);
		if (src != nullptr)
		{
			state.setObject(src->value);
			mpd_return_pair(state.connection(), src);
			return true;
		}
		else
			return false;
	});
}

void Connection::StartSearch(bool exact_match)
{
	prechecksNoCommandsList();
	mpd_search_db_songs(rawConnection(), exact_match);
}

void Connection::StartFieldSearch(mpd_tag_type item)
{
	prechecksNoCommandsList();
	mpd_search_db_tags(rawConnection(), item);
}

void Connection::AddSearch(mpd_tag_type item, const std::string &str) const
{
	checkConnection();
	mpd_search_add_tag_constraint(rawConnection(), MPD_OPERATOR_DEFAULT, item, str.c_str());
}

void Connection::AddSearchAny(const std::string &str) const
{
	checkConnection();
	mpd_search_add_any_tag_constraint(rawConnection(), MPD_OPERATOR_DEFAULT, str.c_str());
}

void Connection::AddSearchURI(const std::string &str) const
{
	checkConnection();
	mpd_search_add_uri_constraint(rawConnection(), MPD_OPERATOR_DEFAULT, str.c_str());
}

SongIterator Connection::CommitSearchSongs()
{
	prechecksNoCommandsList();
	mpd_search_commit(rawConnection());
	checkErrors();
	return SongIterator(rawConnection(), defaultFetcher<Song>(mpd_recv_song));
}

ItemIterator Connection::GetDirectory(const std::string &directory)
{
	prechecksNoCommandsList();
	mpd_send_list_meta(rawConnection(), mpdDirectory(directory));
	checkErrors();
	return ItemIterator(rawConnection(), defaultFetcher<Item>(mpd_recv_entity));
}

SongIterator Connection::GetDirectoryRecursive(const std::string &directory)
{
	prechecksNoCommandsList();
	mpd_send_list_all_meta(rawConnection(), mpdDirectory(directory));
	checkErrors();
	return SongIterator(rawConnection(), fetchItemSong);
}

DirectoryIterator Connection::GetDirectories(const std::string &directory)
{
	prechecksNoCommandsList();
	mpd_send_list_meta(rawConnection(), mpdDirectory(directory));
	checkErrors();
	return DirectoryIterator(rawConnection(), defaultFetcher<Directory>(mpd_recv_directory));
}

SongIterator Connection::GetSongs(const std::string &directory)
{
	prechecksNoCommandsList();
	mpd_send_list_meta(rawConnection(), mpdDirectory(directory));
	checkErrors();
	return SongIterator(rawConnection(), defaultFetcher<Song>(mpd_recv_song));
}

OutputIterator Connection::GetOutputs()
{
	prechecksNoCommandsList();
	mpd_send_outputs(rawConnection());
	checkErrors();
	return OutputIterator(rawConnection(), defaultFetcher<Output>(mpd_recv_output));
}

void Connection::EnableOutput(int id)
{
	prechecksNoCommandsList();
	mpd_run_enable_output(rawConnection(), id);
	checkErrors();
}

void Connection::DisableOutput(int id)
{
	prechecksNoCommandsList();
	mpd_run_disable_output(rawConnection(), id);
	checkErrors();
}

StringIterator Connection::GetURLHandlers()
{
	prechecksNoCommandsList();
	mpd_send_list_url_schemes(rawConnection());
	checkErrors();
	return StringIterator(rawConnection(), [](StringIterator::State &state) {
		auto src = mpd_recv_pair_named(state.connection(), "handler");
		if (src != nullptr)
		{
			state.setObject(src->value);
			mpd_return_pair(state.connection(), src);
			return true;
		}
		else
			return false;
	});
}

StringIterator Connection::GetTagTypes()
{
	
	prechecksNoCommandsList();
	mpd_send_list_tag_types(rawConnection());
	checkErrors();
	return StringIterator(rawConnection(), [](StringIterator::State &state) {
		auto src = mpd_recv_pair_named(state.connection(), "tagtype");
		if (src != nullptr)
		{
			state.setObject(src->value);
			mpd_return_pair(state.connection(), src);
			return true;
		}
		else
			return false;
	});
}

void Connection::checkConnection() const
{
	if (!Connected())
		throw ClientError(MPD_ERROR_STATE, "No active MPD connection", false);
}

void Connection::prechecks()
{
	checkConnection();
	int flags = noidle();
	if (flags && m_noidle_callback)
		m_noidle_callback(flags);
}

void Connection::prechecksNoCommandsList()
{
	assert(!m_command_list_active);
	prechecks();
}

void Connection::checkErrors() const
{
	if (!ncm_mpd_connection_check_error(
	    const_cast<NcmMpdConnection *>(&m_connection)))
		throwConnectionError();
}

}
