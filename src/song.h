#ifndef NCMPCPP_SONG_H
#define NCMPCPP_SONG_H

#include <cassert>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>

#include <mpd/client.h>

#include "c/ncm_base.h"
#include "c/ncm_song.h"

namespace MPD {

namespace Detail {

inline std::string stringFromView(NcmStringView view)
{
	if (view.data == nullptr)
		return "";
	else
		return std::string(view.data, view.len);
}

inline std::string stringFromBuffer(NcmBuffer buffer)
{
	std::string result;
	if (buffer.data != nullptr)
		result.assign(buffer.data, buffer.len);
	ncm_buffer_destroy(&buffer);
	return result;
}

inline void checkedLoadMpdSong(NcmSong *dest, const mpd_song *source)
{
	if (source == nullptr)
		throw std::bad_alloc();
	if (!ncm_song_from_mpd_song_copy(dest, const_cast<mpd_song *>(source)))
		throw std::runtime_error("invalid mpd song");
}

}

struct Song
{
	struct Hash {
		size_t operator()(const Song &s) const { return s.m_hash; }
	};

	Song();
	virtual ~Song();

	Song(mpd_song *s);
	Song(const mpd_song *s, std::shared_ptr<mpd_entity> owner);
	Song(NcmSong *song);

	Song(const Song &rhs);
	Song(Song &&rhs) noexcept;
	Song &operator=(const Song &rhs);
	Song &operator=(Song &&rhs) noexcept;

	std::string get(mpd_tag_type type, unsigned idx = 0) const;
	virtual std::string get(enum NcmSongGetter getter,
	                        unsigned idx = 0) const;

	virtual std::string getURI(unsigned idx = 0) const;
	virtual std::string getName(unsigned idx = 0) const;
	virtual std::string getDirectory(unsigned idx = 0) const;
	virtual std::string getArtist(unsigned idx = 0) const;
	virtual std::string getTitle(unsigned idx = 0) const;
	virtual std::string getAlbum(unsigned idx = 0) const;
	virtual std::string getAlbumArtist(unsigned idx = 0) const;
	virtual std::string getTrack(unsigned idx = 0) const;
	virtual std::string getTrackNumber(unsigned idx = 0) const;
	virtual std::string getDate(unsigned idx = 0) const;
	virtual std::string getGenre(unsigned idx = 0) const;
	virtual std::string getComposer(unsigned idx = 0) const;
	virtual std::string getPerformer(unsigned idx = 0) const;
	virtual std::string getDisc(unsigned idx = 0) const;
	virtual std::string getComment(unsigned idx = 0) const;
	virtual std::string getLength(unsigned idx = 0) const;
	virtual std::string getPriority(unsigned idx = 0) const;

	virtual std::string getTags(enum NcmSongGetter getter) const;

	virtual unsigned getDuration() const;
	virtual unsigned getPosition() const;
	virtual unsigned getID() const;
	virtual unsigned getPrio() const;
	virtual time_t getMTime() const;

	virtual bool isFromDatabase() const;
	virtual bool isStream() const;

	virtual bool empty() const;

	bool operator==(const Song &rhs) const
	{
		if (m_hash != rhs.m_hash)
			return false;
		return ncm_song_equal(const_cast<NcmSong *>(&m_song),
		                      const_cast<NcmSong *>(&rhs.m_song));
	}
	bool operator!=(const Song &rhs) const
	{
		return !(operator==(rhs));
	}

	const char *c_uri() const
	{
		NcmStringView uri;
		if (ncm_song_uri_view(const_cast<NcmSong *>(&m_song), 0, &uri))
			return uri.data;
		else
			return "";
	}

	NcmSong *cSong()
	{
		return &m_song;
	}

	NcmSong *cSong() const
	{
		return const_cast<NcmSong *>(&m_song);
	}

	static std::string ShowTime(unsigned length);

	inline static std::string TagsSeparator = " | ";

	inline static bool ShowDuplicateTags = true;

private:
	NcmSong m_song;
	size_t m_hash;
};

inline Song::Song()
: m_hash(0)
{
	ncm_song_init(&m_song);
}

inline Song::~Song()
{
	ncm_song_destroy(&m_song);
}

inline Song::Song(mpd_song *s)
: m_hash(0)
{
	ncm_song_init(&m_song);
	try
	{
		Detail::checkedLoadMpdSong(&m_song, s);
		m_hash = static_cast<size_t>(ncm_song_hash(&m_song));
	}
	catch (...)
	{
		if (s != nullptr)
			mpd_song_free(s);
		throw;
	}
	mpd_song_free(s);
}

inline Song::Song(const mpd_song *s, std::shared_ptr<mpd_entity> owner)
: m_hash(0)
{
	(void)owner;
	ncm_song_init(&m_song);
	Detail::checkedLoadMpdSong(&m_song, s);
	m_hash = static_cast<size_t>(ncm_song_hash(&m_song));
}

inline Song::Song(NcmSong *song)
: m_hash(0)
{
	ncm_song_init(&m_song);
	if (!ncm_song_copy(&m_song, song))
		throw std::runtime_error("invalid song");
	m_hash = static_cast<size_t>(ncm_song_hash(&m_song));
}

inline Song::Song(const Song &rhs)
: m_hash(rhs.m_hash)
{
	ncm_song_init(&m_song);
	if (!ncm_song_copy(&m_song, const_cast<NcmSong *>(&rhs.m_song)))
		throw std::bad_alloc();
}

inline Song::Song(Song &&rhs) noexcept
: m_hash(rhs.m_hash)
{
	ncm_song_init(&m_song);
	ncm_song_move(&m_song, &rhs.m_song);
	rhs.m_hash = 0;
}

inline Song &Song::operator=(const Song &rhs)
{
	if (this != &rhs)
	{
		if (!ncm_song_copy(&m_song, const_cast<NcmSong *>(&rhs.m_song)))
			throw std::bad_alloc();
		m_hash = rhs.m_hash;
	}
	return *this;
}

inline Song &Song::operator=(Song &&rhs) noexcept
{
	if (this != &rhs)
	{
		ncm_song_move(&m_song, &rhs.m_song);
		m_hash = rhs.m_hash;
		rhs.m_hash = 0;
	}
	return *this;
}

inline std::string Song::get(mpd_tag_type type, unsigned idx) const
{
	NcmStringView tag;

	assert(!empty());
	if (!ncm_song_tag_view(const_cast<NcmSong *>(&m_song), type, idx, &tag))
		return "";
	return Detail::stringFromView(tag);
}

inline std::string Song::get(enum NcmSongGetter getter, unsigned idx) const
{
	assert(!empty());
	return Detail::stringFromBuffer(ncm_song_getter_buffer(
		const_cast<NcmSong *>(&m_song), getter, idx));
}

inline std::string Song::getURI(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_URI, idx);
}

inline std::string Song::getName(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_NAME, idx);
}

inline std::string Song::getDirectory(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_DIRECTORY, idx);
}

inline std::string Song::getArtist(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_ARTIST, idx);
}

inline std::string Song::getTitle(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_TITLE, idx);
}

inline std::string Song::getAlbum(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_ALBUM, idx);
}

inline std::string Song::getAlbumArtist(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_ALBUM_ARTIST, idx);
}

inline std::string Song::getTrack(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_TRACK, idx);
}

inline std::string Song::getTrackNumber(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_TRACK_NUMBER, idx);
}

inline std::string Song::getDate(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_DATE, idx);
}

inline std::string Song::getGenre(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_GENRE, idx);
}

inline std::string Song::getComposer(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_COMPOSER, idx);
}

inline std::string Song::getPerformer(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_PERFORMER, idx);
}

inline std::string Song::getDisc(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_DISC, idx);
}

inline std::string Song::getComment(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_COMMENT, idx);
}

inline std::string Song::getLength(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_LENGTH, idx);
}

inline std::string Song::getPriority(unsigned idx) const
{
	assert(!empty());
	return get(NCM_SONG_GETTER_PRIORITY, idx);
}

inline std::string Song::getTags(enum NcmSongGetter getter) const
{
	assert(!empty());
	return Detail::stringFromBuffer(ncm_song_tags_buffer(
		const_cast<NcmSong *>(&m_song), getter,
		const_cast<char *>(TagsSeparator.c_str()),
		static_cast<int32>(TagsSeparator.size()), ShowDuplicateTags));
}

inline unsigned Song::getDuration() const
{
	assert(!empty());
	return ncm_song_duration(const_cast<NcmSong *>(&m_song));
}

inline unsigned Song::getPosition() const
{
	assert(!empty());
	return ncm_song_position(const_cast<NcmSong *>(&m_song));
}

inline unsigned Song::getID() const
{
	assert(!empty());
	return ncm_song_id(const_cast<NcmSong *>(&m_song));
}

inline unsigned Song::getPrio() const
{
	assert(!empty());
	return ncm_song_priority(const_cast<NcmSong *>(&m_song));
}

inline time_t Song::getMTime() const
{
	assert(!empty());
	return ncm_song_mtime(const_cast<NcmSong *>(&m_song));
}

inline bool Song::isFromDatabase() const
{
	assert(!empty());
	return ncm_song_is_from_database(const_cast<NcmSong *>(&m_song));
}

inline bool Song::isStream() const
{
	assert(!empty());
	return ncm_song_is_stream(const_cast<NcmSong *>(&m_song));
}

inline bool Song::empty() const
{
	return ncm_song_empty(const_cast<NcmSong *>(&m_song));
}

inline std::string Song::ShowTime(unsigned length)
{
	char buffer[32];
	int len;

	len = ncm_song_show_time(length, buffer, sizeof(buffer));
	return std::string(buffer, len);
}

}

#endif // NCMPCPP_SONG_H
