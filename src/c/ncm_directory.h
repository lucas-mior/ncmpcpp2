#if !defined(NCM_DIRECTORY_H)
#define NCM_DIRECTORY_H

#include <time.h>

#include "c/ncm_defs.h"

struct mpd_directory;

typedef struct NcmDirectory {
    char *path;
    int32 path_len;
    time_t last_modified;
} NcmDirectory;

void ncm_directory_init(NcmDirectory *directory);
void ncm_directory_destroy(NcmDirectory *directory);
void ncm_directory_move(NcmDirectory *dest, NcmDirectory *source);
bool ncm_directory_set(NcmDirectory *directory, char *path,
                       int32 path_len, time_t last_modified);
bool ncm_directory_copy(NcmDirectory *dest, NcmDirectory *source);
bool ncm_directory_path_view(NcmDirectory *directory, NcmStringView *view);
time_t ncm_directory_last_modified(NcmDirectory *directory);
bool ncm_directory_from_mpd_directory(NcmDirectory *dest,
                                      struct mpd_directory *source);

#endif /* NCM_DIRECTORY_H */
