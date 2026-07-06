#if !defined(NCM_PLAYLIST_H)
#define NCM_PLAYLIST_H

#include <time.h>

#if defined(__cplusplus)
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#endif

#include "c/ncm_defs.h"

struct mpd_playlist;

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmPlaylist {
    char *path;
    int32 path_len;
    time_t last_modified;

#if defined(__cplusplus)
    NcmPlaylist();
    explicit NcmPlaylist(struct mpd_playlist *playlist);
    explicit NcmPlaylist(NcmPlaylist *playlist);
    explicit NcmPlaylist(std::string path_, time_t last_modified_ = 0);
    NcmPlaylist(const NcmPlaylist &rhs);
    NcmPlaylist(NcmPlaylist &&rhs) noexcept;
    NcmPlaylist &operator=(const NcmPlaylist &rhs);
    NcmPlaylist &operator=(NcmPlaylist &&rhs) noexcept;
    ~NcmPlaylist();

    bool operator==(const NcmPlaylist &rhs) const;
    bool operator!=(const NcmPlaylist &rhs) const;
    time_t lastModified() const;
    NcmPlaylist *cPlaylist();
#endif
} NcmPlaylist;

void ncm_playlist_init(NcmPlaylist *playlist);
void ncm_playlist_destroy(NcmPlaylist *playlist);
void ncm_playlist_move(NcmPlaylist *dest, NcmPlaylist *source);
bool ncm_playlist_set(NcmPlaylist *playlist, char *path,
                      int32 path_len, time_t last_modified);
bool ncm_playlist_copy(NcmPlaylist *dest, NcmPlaylist *source);
bool ncm_playlist_path_view(NcmPlaylist *playlist, NcmStringView *view);
time_t ncm_playlist_last_modified(NcmPlaylist *playlist);
bool ncm_playlist_equal(NcmPlaylist *a, NcmPlaylist *b);
bool ncm_playlist_from_mpd_playlist(NcmPlaylist *dest,
                                    struct mpd_playlist *source);

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
inline NcmPlaylist::NcmPlaylist()
{
    ncm_playlist_init(this);
}

inline NcmPlaylist::NcmPlaylist(struct mpd_playlist *playlist_)
{
    ncm_playlist_init(this);
    if (!ncm_playlist_from_mpd_playlist(this, playlist_))
    {
        throw std::runtime_error("invalid mpd playlist");
    }
}

inline NcmPlaylist::NcmPlaylist(NcmPlaylist *playlist_)
{
    ncm_playlist_init(this);
    if (!ncm_playlist_copy(this, playlist_))
    {
        throw std::runtime_error("invalid playlist");
    }
}

inline NcmPlaylist::NcmPlaylist(std::string path_, time_t last_modified_)
{
    ncm_playlist_init(this);
    if (!ncm_playlist_set(this,
                          const_cast<char *>(path_.c_str()),
                          static_cast<int32>(path_.size()),
                          last_modified_))
    {
        throw std::runtime_error("empty playlist path");
    }
}

inline NcmPlaylist::NcmPlaylist(const NcmPlaylist &rhs)
{
    ncm_playlist_init(this);
    if (!ncm_playlist_copy(this, const_cast<NcmPlaylist *>(&rhs)))
    {
        throw std::bad_alloc();
    }
}

inline NcmPlaylist::NcmPlaylist(NcmPlaylist &&rhs) noexcept
{
    ncm_playlist_init(this);
    ncm_playlist_move(this, &rhs);
}

inline NcmPlaylist &NcmPlaylist::operator=(const NcmPlaylist &rhs)
{
    if (this != &rhs)
    {
        if (!ncm_playlist_copy(this, const_cast<NcmPlaylist *>(&rhs)))
        {
            throw std::bad_alloc();
        }
    }
    return *this;
}

inline NcmPlaylist &NcmPlaylist::operator=(NcmPlaylist &&rhs) noexcept
{
    if (this != &rhs)
    {
        ncm_playlist_move(this, &rhs);
    }
    return *this;
}

inline NcmPlaylist::~NcmPlaylist()
{
    ncm_playlist_destroy(this);
}

inline bool NcmPlaylist::operator==(const NcmPlaylist &rhs) const
{
    return ncm_playlist_equal(const_cast<NcmPlaylist *>(this),
                              const_cast<NcmPlaylist *>(&rhs));
}

inline bool NcmPlaylist::operator!=(const NcmPlaylist &rhs) const
{
    return !(*this == rhs);
}

inline std::string ncm_playlist_cpp_path(const NcmPlaylist &playlist)
{
    NcmStringView view;

    if (!ncm_playlist_path_view(const_cast<NcmPlaylist *>(&playlist), &view))
    {
        return "";
    }
    return std::string(view.data, static_cast<size_t>(view.len));
}

inline time_t NcmPlaylist::lastModified() const
{
    return ncm_playlist_last_modified(const_cast<NcmPlaylist *>(this));
}

inline NcmPlaylist *NcmPlaylist::cPlaylist()
{
    return this;
}
#endif

#endif /* NCM_PLAYLIST_H */
