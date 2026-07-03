#if !defined(NCM_DIRECTORY_H)
#define NCM_DIRECTORY_H

#include <time.h>

#include "c/ncm_defs.h"

struct mpd_directory;

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmDirectory {
    char *path;
    int32 path_len;
    time_t last_modified;
} NcmDirectory;

void ncm_directory_init(NcmDirectory *directory);
void ncm_directory_destroy(NcmDirectory *directory);
bool ncm_directory_set(NcmDirectory *directory, char *path,
                       int32 path_len, time_t last_modified);
bool ncm_directory_copy(NcmDirectory *dest, NcmDirectory *source);
bool ncm_directory_from_mpd_directory(NcmDirectory *dest,
                                      struct mpd_directory *source);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_DIRECTORY_H */
