#if !defined(NCMPCPP_CONFIGURATION_H)
#define NCMPCPP_CONFIGURATION_H

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

void ncm_configuration_options_init(NcmConfigurationOptions *options);
void ncm_configuration_options_destroy(NcmConfigurationOptions *options);
int32 ncm_configuration_options_parse(NcmConfigurationOptions *options,
                                       int32 argc, char **argv,
                                       NcmError *ncm_error);
int32 ncm_configuration_options_apply(NcmConfigurationOptions *options,
                                      NcmError *ncm_error);

int32 configuration_discover_default_paths(StrBuilderArray *config_paths,
                                           StrBuilderArray *bindings_paths,
                                           NcmError *ncm_error);
int32 configure(int32 argc, char **argv);

#endif /* NCMPCPP_CONFIGURATION_H */
