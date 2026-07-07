#if !defined(NCM_PLAYLIST_LEGACY_H)
#define NCM_PLAYLIST_LEGACY_H

#include <string>

#include "c/ncm_playlist.h"

inline std::string
ncm_playlist_cpp_path(NcmPlaylist *playlist)
{
    NcmStringView view;

    if (!ncm_playlist_path_view(playlist, &view)) {
        return "";
    }
    return std::string(view.data, static_cast<size_t>(view.len));
}

inline std::string
ncm_playlist_cpp_path(const NcmPlaylist &playlist)
{
    return ncm_playlist_cpp_path(const_cast<NcmPlaylist *>(&playlist));
}

inline bool
operator==(const NcmPlaylist &a, const NcmPlaylist &b)
{
    return ncm_playlist_equal(const_cast<NcmPlaylist *>(&a),
                              const_cast<NcmPlaylist *>(&b));
}

inline bool
operator!=(const NcmPlaylist &a, const NcmPlaylist &b)
{
    return !(a == b);
}

#endif /* NCM_PLAYLIST_LEGACY_H */
