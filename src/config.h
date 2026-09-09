#if !defined(config_H)
#define config_H

#include "cbase.h"

#include "c/ncm_c.h"

typedef struct NcmConfigurationOptions {
    StrBuilder host;
    StrBuilder current_song_format;
    StrBuilder screen_name;
    StrBuilder slave_screen_name;
    StrBuilderArray config_paths;
    StrBuilderArray bindings_paths;

    int32 port;

    bool host_provided;
    bool port_provided;
    bool current_song;
    bool ignore_config_errors;
    bool test_lyrics_fetchers;
    bool screen;
    bool slave_screen;
    bool help;
    bool version;
    bool quiet;
} NcmConfigurationOptions;

void ncm_config_options_init(NcmConfigurationOptions *);
void ncm_config_options_destroy(NcmConfigurationOptions *);
int32 ncm_config_options_parse(NcmConfigurationOptions *, int32,
                                      char **, NcmError *);
int32 ncm_config_options_apply(NcmConfigurationOptions *, NcmError *);

int32 config_discover_default_paths(StrBuilderArray *config_paths,
                                           StrBuilderArray *bindings_paths,
                                           NcmError *);
int32 configure(int32, char **);

#endif /* config_H */
