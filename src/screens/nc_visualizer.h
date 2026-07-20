#if !defined(NCMPCPP_NC_VISUALIZER_H)
#define NCMPCPP_NC_VISUALIZER_H

#include <stdbool.h>

#include "config.h"

#if defined(HAVE_FFTW3_H)
#include <fftw3.h>
#endif

#include "c/ncm_defs.h"
#include "c/ncm_sample_buffer.h"
#include "curses/nc_formatted_color.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define NATIVE_VISUALIZER_PI 3.14159265358979323846

#if defined(HAVE_FFTW3_H)
#define NATIVE_VISUALIZER_FREQUENCY_FIELD \
    X(NATIVE_VISUALIZER_FREQUENCY)
#else
#define NATIVE_VISUALIZER_FREQUENCY_FIELD
#endif

#define ENUM_NAME NativeVisualizerType
#define ENUM_PREFIX_ NATIVE_VISUALIZER_TYPE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NATIVE_VISUALIZER_WAVE) \
    X(NATIVE_VISUALIZER_WAVE_FILLED) \
    NATIVE_VISUALIZER_FREQUENCY_FIELD \
    X(NATIVE_VISUALIZER_ELLIPSE)
#include "cbase/xenums.c"
#undef NATIVE_VISUALIZER_FREQUENCY_FIELD

struct NcmError;
struct NcmMpdClient;
struct NcmMpdOutputList;

typedef struct NativeVisualizerDataSourceHooks {
    int32 (*open_fifo)(void *user, char *location, int32 location_len);
    int32 (*open_udp)(void *user, char *location, int32 location_len,
                      char *port, int32 port_len);
    int32 (*read_source)(void *user, int32 fd, void *buffer,
                         int32 buffer_size);
    void (*close_source)(void *user, int32 fd);
    bool (*get_outputs)(void *user, struct NcmMpdOutputList *outputs,
                        struct NcmError *error);
    bool (*disable_output)(void *user, uint32 id,
                           struct NcmError *error);
    bool (*enable_output)(void *user, uint32 id,
                          struct NcmError *error);
    void (*sleep_microseconds)(void *user, int32 microseconds);
    void *user;
} NativeVisualizerDataSourceHooks;

typedef struct NativeVisualizerScreenConfig {
    char *source_location;
    char *output_name;
    char *visualizer_chars;
    NcFormattedColor *visualizer_colors;

    int32 source_location_len;
    int32 output_name_len;
    int32 visualizer_chars_len;
    int32 visualizer_colors_len;
    int32 fps;
    int32 spectrum_dft_size;

    double spectrum_gain;
    double spectrum_hz_min;
    double spectrum_hz_max;

    NativeVisualizerDataSourceHooks data_source_hooks;
    enum NativeVisualizerType visualization_type;
    bool autoscale;
    bool stereo;
    bool spectrum_smooth_look;
    bool spectrum_smooth_look_legacy_chars;
    bool spectrum_log_scale_x;
    bool spectrum_log_scale_y;
} NativeVisualizerScreenConfig;

#if defined(HAVE_FFTW3_H)
typedef struct NativeVisualizerBarHeight {
    int32 column;
    double height;
} NativeVisualizerBarHeight;

typedef struct NativeVisualizerFftState {
    double *input;
    fftw_complex *output;
    fftw_plan plan;

    double *frequency_magnitudes;
    double *dft_frequency_space;
    NativeVisualizerBarHeight *bar_heights;

    int32 results_len;
    int32 dft_nonzero_size;
    int32 dft_total_size;

    int32 frequency_magnitudes_len;
    int32 frequency_magnitudes_cap;
    int32 dft_frequency_space_len;
    int32 dft_frequency_space_cap;
    int32 bar_heights_len;
    int32 bar_heights_cap;

    double dynamic_range;
    double hz_min;
    double hz_max;
    double gain;
} NativeVisualizerFftState;
#endif

typedef struct NativeVisualizerScreen {
    NcScreen screen;
    NcWindow window;

    NcmBuffer source_location;
    NcmBuffer source_port;
    NcmBuffer output_name;
    NcmBuffer visualizer_chars;
    NcFormattedColor *visualizer_colors;
    NativeVisualizerDataSourceHooks data_source_hooks;

    NcmSampleBuffer incoming_samples;
    NcmSampleBuffer buffered_samples;
    NcmSampleBuffer rendered_samples;
    NcmSampleBuffer left_channel;
    NcmSampleBuffer right_channel;

#if defined(HAVE_FFTW3_H)
    NativeVisualizerFftState fft;
#endif

    double auto_scale_multiplier;
    enum NativeVisualizerType visualization_type;

    int32 source_fd;
    int32 output_id;
    int32 fps;
    int32 sample_rate;
    int32 sample_consumption_rate;
    int32 sample_consumption_rate_up_ctr;
    int32 sample_consumption_rate_dn_ctr;
    int32 visualizer_colors_len;
    int32 visualizer_colors_cap;
    int32 point_char_offset;
    int32 point_char_len;
    int32 bar_char_offset;
    int32 bar_char_len;

    bool reset_output;
    bool autoscale;
    bool stereo;
    bool spectrum_smooth_look;
    bool spectrum_smooth_look_legacy_chars;
    bool spectrum_log_scale_x;
    bool spectrum_log_scale_y;
    bool initialized;
} NativeVisualizerScreen;

void native_visualizer_screen_init(NativeVisualizerScreen *screen,
                                   int32 start_x, int32 start_y,
                                   int32 width, int32 height,
                                   NcColor color, NcBorder border,
                                   NativeVisualizerScreenConfig *config);
void native_visualizer_screen_destroy(NativeVisualizerScreen *screen);
NativeVisualizerDataSourceHooks native_visualizer_data_source_system_hooks(
    struct NcmMpdClient *client);
void native_visualizer_screen_init_data_source(
    NativeVisualizerScreen *screen, char *source_location,
    int32 source_location_len);
bool native_visualizer_screen_open_data_source(
    NativeVisualizerScreen *screen);
void native_visualizer_screen_close_data_source(
    NativeVisualizerScreen *screen);
int32 native_visualizer_screen_drain_data_source(
    NativeVisualizerScreen *screen);
bool native_visualizer_screen_find_output_id(
    NativeVisualizerScreen *screen);
NcScreen *native_visualizer_screen_base(NativeVisualizerScreen *screen);
NcWindow *native_visualizer_screen_window(NativeVisualizerScreen *screen);
void native_visualizer_screen_set_geometry(NativeVisualizerScreen *screen,
                                           int32 start_x, int32 start_y,
                                           int32 width, int32 height);
void native_visualizer_screen_init_visualization(
    NativeVisualizerScreen *screen);
void native_visualizer_screen_clear(NativeVisualizerScreen *screen);
void native_visualizer_screen_reset_auto_scale_multiplier(
    NativeVisualizerScreen *screen);
void native_visualizer_screen_toggle_type(NativeVisualizerScreen *screen);
int32 native_visualizer_screen_requested_samples(
    NativeVisualizerScreen *screen);
bool native_visualizer_screen_push_samples(NativeVisualizerScreen *screen,
                                           int16 *samples,
                                           int32 samples_len);
int32 native_visualizer_screen_take_render_samples(
    NativeVisualizerScreen *screen, int16 *dest, int32 dest_len);
int32 native_visualizer_screen_split_stereo(NativeVisualizerScreen *screen,
                                            int16 *samples,
                                            int32 samples_len);
void native_visualizer_screen_apply_auto_scale(NativeVisualizerScreen *screen,
                                               int16 *samples,
                                               int32 samples_len);
bool native_visualizer_screen_draw(NativeVisualizerScreen *screen,
                                   int16 *samples, int32 samples_len);
int16 native_visualizer_clamp_sample(int32 sample);

#endif /* NCMPCPP_NC_VISUALIZER_H */
