#if !defined(NCMPCPP_CONFIGURATION_H)
#define NCMPCPP_CONFIGURATION_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_error.h"

typedef struct NcmConfigurationOptions {
    NcmBuffer host;
    NcmBuffer current_song_format;
    NcmBuffer screen_name;
    NcmBuffer slave_screen_name;
    NcmBufferArray config_paths;
    NcmBufferArray bindings_paths;

    uint32 port;

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
bool ncm_configuration_options_parse(NcmConfigurationOptions *options,
                                     int32 argc, char **argv,
                                     NcmError *error);
bool ncm_configuration_options_apply(NcmConfigurationOptions *options,
                                     NcmError *error);

bool configuration_discover_default_paths(NcmBufferArray *config_paths,
                                          NcmBufferArray *bindings_paths,
                                          NcmError *error);
bool configure(int32 argc, char **argv);

#endif /* NCMPCPP_CONFIGURATION_H */
