#ifndef NCMPCPP_EDITABLE_SONG_H
#define NCMPCPP_EDITABLE_SONG_H

#include <string>
#include <utility>

#include "c/ncm_mutable_song.h"
#include "c/ncm_type_conversions.h"
#include "config.h"
#include "song.h"

namespace MPD {

namespace Detail {

inline int32 stringLength(const std::string &value)
{
	return static_cast<int32>(value.size());
}

}

struct MutableSong : public Song
{
	MutableSong();
	MutableSong(Song s);
	MutableSong(const MutableSong &rhs);
	MutableSong(MutableSong &&rhs) noexcept;
	MutableSong &operator=(const MutableSong &rhs);
	MutableSong &operator=(MutableSong &&rhs) noexcept;
	virtual ~MutableSong() override;

	virtual std::string get(enum NcmSongGetter getter,
	                        unsigned idx = 0) const override;
	virtual std::string getArtist(unsigned idx = 0) const override;
	virtual std::string getTitle(unsigned idx = 0) const override;
	virtual std::string getAlbum(unsigned idx = 0) const override;
	virtual std::string getAlbumArtist(unsigned idx = 0) const override;
	virtual std::string getTrack(unsigned idx = 0) const override;
	virtual std::string getDate(unsigned idx = 0) const override;
	virtual std::string getGenre(unsigned idx = 0) const override;
	virtual std::string getComposer(unsigned idx = 0) const override;
	virtual std::string getPerformer(unsigned idx = 0) const override;
	virtual std::string getDisc(unsigned idx = 0) const override;
	virtual std::string getComment(unsigned idx = 0) const override;
	virtual std::string getTags(enum NcmSongGetter getter) const override;

	std::string get(enum NcmTagsField field, unsigned idx = 0) const;
	void set(enum NcmTagsField field, const std::string &value,
	         unsigned idx = 0);
	void setTags(enum NcmTagsField field, const std::string &value);

	void setArtist(const std::string &value, unsigned idx = 0);
	void setTitle(const std::string &value, unsigned idx = 0);
	void setAlbum(const std::string &value, unsigned idx = 0);
	void setAlbumArtist(const std::string &value, unsigned idx = 0);
	void setTrack(const std::string &value, unsigned idx = 0);
	void setDate(const std::string &value, unsigned idx = 0);
	void setGenre(const std::string &value, unsigned idx = 0);
	void setComposer(const std::string &value, unsigned idx = 0);
	void setPerformer(const std::string &value, unsigned idx = 0);
	void setDisc(const std::string &value, unsigned idx = 0);
	void setComment(const std::string &value, unsigned idx = 0);

	std::string getNewName() const;
	void setNewName(const std::string &value);

	virtual unsigned getDuration() const override;
	virtual time_t getMTime() const override;
	void setDuration(unsigned duration);
	void setMTime(time_t mtime);

	bool isModified() const;
	void clearModifications();

	NcmMutableSong *cMutableSong();

private:
	void loadOriginals();

	NcmMutableSong m_mutable;
};

inline MutableSong::MutableSong()
{
	ncm_mutable_song_init(&m_mutable);
}

inline MutableSong::MutableSong(Song s)
: Song(std::move(s))
{
	ncm_mutable_song_init(&m_mutable);
	loadOriginals();
}

inline MutableSong::MutableSong(const MutableSong &rhs)
: Song(static_cast<const Song &>(rhs))
{
	ncm_mutable_song_init(&m_mutable);
	ncm_mutable_song_copy(&m_mutable,
	                      const_cast<NcmMutableSong *>(&rhs.m_mutable));
}

inline MutableSong::MutableSong(MutableSong &&rhs) noexcept
: Song(static_cast<Song &&>(rhs))
{
	ncm_mutable_song_init(&m_mutable);
	ncm_mutable_song_move(&m_mutable, &rhs.m_mutable);
}

inline MutableSong &MutableSong::operator=(const MutableSong &rhs)
{
	if (this != &rhs)
	{
		Song::operator=(static_cast<const Song &>(rhs));
		ncm_mutable_song_copy(&m_mutable,
		                      const_cast<NcmMutableSong *>(&rhs.m_mutable));
	}
	return *this;
}

inline MutableSong &MutableSong::operator=(MutableSong &&rhs) noexcept
{
	if (this != &rhs)
	{
		Song::operator=(static_cast<Song &&>(rhs));
		ncm_mutable_song_move(&m_mutable, &rhs.m_mutable);
	}
	return *this;
}

inline MutableSong::~MutableSong()
{
	ncm_mutable_song_destroy(&m_mutable);
}

inline std::string MutableSong::get(enum NcmSongGetter getter,
                                    unsigned idx) const
{
	enum NcmTagsField field;

	field = ncm_song_getter_to_tags_field(getter);
	if (field == NCM_TAGS_FIELD_LAST
	    || getter == NCM_SONG_GETTER_TRACK_NUMBER)
		return Song::get(getter, idx);
	return get(field, idx);
}

inline std::string MutableSong::get(enum NcmTagsField field,
                                    unsigned idx) const
{
	return Detail::stringFromBuffer(ncm_mutable_song_get_tag_buffer(
		const_cast<NcmMutableSong *>(&m_mutable), field,
		static_cast<int32>(idx)));
}

inline void MutableSong::set(enum NcmTagsField field,
                             const std::string &value, unsigned idx)
{
	if (field == NCM_TAGS_FIELD_LAST)
		return;
	ncm_mutable_song_set_tag(&m_mutable, field, static_cast<int32>(idx),
	                         const_cast<char *>(value.c_str()),
	                         Detail::stringLength(value));
}

inline void MutableSong::setTags(enum NcmTagsField field,
                                 const std::string &value)
{
	if (field == NCM_TAGS_FIELD_LAST)
		return;

	ncm_mutable_song_set_tags(&m_mutable, field,
	                          const_cast<char *>(value.c_str()),
	                          Detail::stringLength(value),
	                          const_cast<char *>(Song::TagsSeparator.c_str()),
	                          Detail::stringLength(Song::TagsSeparator));
}

inline std::string MutableSong::getArtist(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_ARTIST, idx);
}

inline std::string MutableSong::getTitle(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_TITLE, idx);
}

inline std::string MutableSong::getAlbum(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_ALBUM, idx);
}

inline std::string MutableSong::getAlbumArtist(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_ALBUM_ARTIST, idx);
}

inline std::string MutableSong::getTrack(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_TRACK, idx);
}

inline std::string MutableSong::getDate(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_DATE, idx);
}

inline std::string MutableSong::getGenre(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_GENRE, idx);
}

inline std::string MutableSong::getComposer(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_COMPOSER, idx);
}

inline std::string MutableSong::getPerformer(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_PERFORMER, idx);
}

inline std::string MutableSong::getDisc(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_DISC, idx);
}

inline std::string MutableSong::getComment(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_COMMENT, idx);
}

inline void MutableSong::setArtist(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_ARTIST, value, idx);
}

inline void MutableSong::setTitle(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_TITLE, value, idx);
}

inline void MutableSong::setAlbum(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_ALBUM, value, idx);
}

inline void MutableSong::setAlbumArtist(const std::string &value,
                                        unsigned idx)
{
	set(NCM_TAGS_FIELD_ALBUM_ARTIST, value, idx);
}

inline void MutableSong::setTrack(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_TRACK, value, idx);
}

inline void MutableSong::setDate(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_DATE, value, idx);
}

inline void MutableSong::setGenre(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_GENRE, value, idx);
}

inline void MutableSong::setComposer(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_COMPOSER, value, idx);
}

inline void MutableSong::setPerformer(const std::string &value,
                                      unsigned idx)
{
	set(NCM_TAGS_FIELD_PERFORMER, value, idx);
}

inline void MutableSong::setDisc(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_DISC, value, idx);
}

inline void MutableSong::setComment(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_COMMENT, value, idx);
}

inline std::string MutableSong::getTags(enum NcmSongGetter getter) const
{
	enum NcmTagsField field;

	field = ncm_song_getter_to_tags_field(getter);
	if (field == NCM_TAGS_FIELD_LAST
	    || getter == NCM_SONG_GETTER_TRACK_NUMBER)
		return Song::getTags(getter);
	return Detail::stringFromBuffer(ncm_mutable_song_tags_buffer(
		const_cast<NcmMutableSong *>(&m_mutable), field,
		const_cast<char *>(Song::TagsSeparator.c_str()),
		static_cast<int32>(Song::TagsSeparator.size()),
		Song::ShowDuplicateTags));
}

inline std::string MutableSong::getNewName() const
{
	NcmStringView view;

	if (!ncm_mutable_song_get_new_name(
		    const_cast<NcmMutableSong *>(&m_mutable), &view))
		return "";
	return Detail::stringFromView(view);
}

inline void MutableSong::setNewName(const std::string &value)
{
	ncm_mutable_song_set_new_name(&m_mutable,
	                              const_cast<char *>(value.c_str()),
	                              Detail::stringLength(value));
}

inline unsigned MutableSong::getDuration() const
{
	uint32 duration;

	duration = ncm_mutable_song_duration(
		const_cast<NcmMutableSong *>(&m_mutable));
	if (duration > 0)
		return duration;
	else
		return Song::getDuration();
}

inline time_t MutableSong::getMTime() const
{
	int64 mtime;

	mtime = ncm_mutable_song_mtime(
		const_cast<NcmMutableSong *>(&m_mutable));
	if (mtime > 0)
		return static_cast<time_t>(mtime);
	else
		return Song::getMTime();
}

inline void MutableSong::setDuration(unsigned int duration)
{
	ncm_mutable_song_set_duration(&m_mutable, static_cast<uint32>(duration));
}

inline void MutableSong::setMTime(time_t mtime)
{
	ncm_mutable_song_set_mtime(&m_mutable, static_cast<int64>(mtime));
}

inline bool MutableSong::isModified() const
{
	return ncm_mutable_song_is_modified(
		const_cast<NcmMutableSong *>(&m_mutable));
}

inline void MutableSong::clearModifications()
{
	ncm_mutable_song_clear_modifications(&m_mutable);
}

inline NcmMutableSong *MutableSong::cMutableSong()
{
	return &m_mutable;
}

inline void MutableSong::loadOriginals()
{
	ncm_mutable_song_load_originals_from_song(&m_mutable, cSong());
}

}

#endif // NCMPCPP_EDITABLE_SONG_H
