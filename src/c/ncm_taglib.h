#if !defined(NCM_TAGLIB_H)
#define NCM_TAGLIB_H

#include "c/ncm_defs.h"

typedef struct NcmTaglibFile {
    void *handle;
} NcmTaglibFile;

typedef struct NcmTaglibAudioProperties {
    int32 length;
    int32 bitrate;
    int32 sample_rate;
    int32 channels;
} NcmTaglibAudioProperties;

typedef void (*NcmTaglibPairCallback)(char *name, char *value, void *user);
typedef void (*NcmTaglibValueCallback)(char *value, void *user);

void ncm_taglib_init(void);
void ncm_taglib_file_init(NcmTaglibFile *file);
bool ncm_taglib_file_open(NcmTaglibFile *file, char *path);
void ncm_taglib_file_close(NcmTaglibFile *file);
bool ncm_taglib_file_is_open(NcmTaglibFile *file);
bool ncm_taglib_file_save(NcmTaglibFile *file);
bool ncm_taglib_file_audio_properties(NcmTaglibFile *file,
                                      NcmTaglibAudioProperties *properties);
bool ncm_taglib_read_mapped_properties(NcmTaglibFile *file,
                                       NcmTaglibPairCallback callback,
                                       void *user);
bool ncm_taglib_read_property(NcmTaglibFile *file, char *property,
                              NcmTaglibValueCallback callback, void *user);
void ncm_taglib_clear_property(NcmTaglibFile *file, char *property);
void ncm_taglib_append_property(NcmTaglibFile *file, char *property,
                                char *value);
bool ncm_taglib_extended_set_supported(NcmTaglibFile *file);
void ncm_taglib_clear_strings(void);

#endif /* NCM_TAGLIB_H */
