#if !defined(NCMPCPP_NC_VISUALIZER_H)
#define NCMPCPP_NC_VISUALIZER_H

#include "cbase.h"

#include "config.h"

#if defined(HAVE_FFTW3_H)
#define FFTW_NO_Complex 1
#include <fftw3.h>
#endif

#include "c/ncm_defs.h"
#include "c/ncm_sample_buffer.h"
#include "c/ncm_time.h"
#include "curses/nc_formatted_color.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define VISUALIZER_PI 3.14159265358979323846

#if defined(HAVE_FFTW3_H)
#define VISUALIZER_FREQUENCY_FIELD \
    X(VISUALIZER_FREQUENCY)
#else
#define VISUALIZER_FREQUENCY_FIELD
#endif

#define ENUM_NAME VisualizerScreenType
#define ENUM_PREFIX_ VISUALIZER_TYPE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(VISUALIZER_WAVE) \
    X(VISUALIZER_WAVE_FILLED) \
    VISUALIZER_FREQUENCY_FIELD \
    X(VISUALIZER_ELLIPSE)
#include "cbase/xenums.c"
#undef VISUALIZER_FREQUENCY_FIELD

struct NcmError;
struct NcmMpdClient;
struct NcmMpdOutputList;

typedef struct VisualizerDataSourceHooks {
    int32 (*open_fifo)(void *user, char *location, int32 location_len);
    int32 (*open_udp)(void *user, char *location, int32 location_len,
                      char *port, int32 port_len);
    int32 (*read_source)(void *user, int32 fd, void *buffer, int32 buffer_size);
    void (*close_source)(void *user, int32 fd);
    bool (*get_outputs)(void *user, struct NcmMpdOutputList *outputs,
                        struct NcmError *error);
    bool (*disable_output)(void *user, int32 id, struct NcmError *error);
    bool (*enable_output)(void *user, int32 id, struct NcmError *error);
    void (*sleep_microseconds)(void *user, int32 microseconds);
    void *user;
} VisualizerDataSourceHooks;

typedef struct VisualizerScreenConfig {
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

    VisualizerDataSourceHooks data_source_hooks;
    enum VisualizerScreenType visualization_type;
    bool autoscale;
    bool stereo;
    bool spectrum_smooth_look;
    bool spectrum_smooth_look_legacy_chars;
    bool spectrum_log_scale_x;
    bool spectrum_log_scale_y;
} VisualizerScreenConfig;

#if defined(HAVE_FFTW3_H)
typedef struct VisualizerBarHeight {
    int32 column;
    double height;
} VisualizerBarHeight;

typedef struct VisualizerFftState {
    double *input;
    fftw_complex *output;
    fftw_plan plan;

    double *freqs_mags;
    double *dft_frequency_space;
    VisualizerBarHeight *bar_heights;

    int32 results_len;
    int32 dft_nonzero_size;
    int32 dft_total_size;

    int32 freqs_mags_len;
    int32 freqs_mags_cap;
    int32 dft_frequency_space_len;
    int32 dft_frequency_space_cap;
    int32 bar_heights_len;
    int32 bar_heights_cap;

    double dynamic_range;
    double hz_min;
    double hz_max;
    double gain;
} VisualizerFftState;
#endif

typedef struct VisualizerScreen {
    NcScreen screen;
    NcWindow window;

    StrBuilder source_location;
    StrBuilder source_port;
    StrBuilder output_name;
    StrBuilder visualizer_chars;
    NcFormattedColor *visualizer_colors;
    VisualizerDataSourceHooks data_source_hooks;

    NcmSampleBuffer incoming_samples;
    NcmSampleBuffer buffered_samples;
    NcmSampleBuffer rendered_samples;
    NcmSampleBuffer left_channel;
    NcmSampleBuffer right_channel;

#if defined(HAVE_FFTW3_H)
    VisualizerFftState fft;
#endif

    double auto_scale_multiplier;
    enum VisualizerScreenType visualization_type;

    int32 source_fd;
    int32 output_id;
    int32 fps;
    int32 sample_rate;
    NcmTimePoint sample_clock;
    int64 sample_clock_frame_remainder;
    int32 visualizer_colors_len;
    int32 visualizer_colors_cap;
    int32 point_char_offset;
    int32 point_char_len;
    int32 bar_char_offset;
    int32 bar_char_len;

    bool reset_output;
    bool sample_clock_initialized;
    bool autoscale;
    bool stereo;
    bool spectrum_smooth_look;
    bool spectrum_smooth_look_legacy_chars;
    bool spectrum_log_scale_x;
    bool spectrum_log_scale_y;
    bool initialized;
} VisualizerScreen;

void visualizer_screen_init(VisualizerScreen *screen,
                                   int32 start_x, int32 start_y,
                                   int32 width, int32 height,
                                   NcColor color, NcBorder border,
                                   VisualizerScreenConfig *config);
void visualizer_screen_destroy(VisualizerScreen *screen);
VisualizerDataSourceHooks visualizer_data_source_system_hooks(
    struct NcmMpdClient *client);
void visualizer_screen_init_data_source(
    VisualizerScreen *screen, char *source_location,
    int32 source_location_len);
bool visualizer_screen_open_data_source(
    VisualizerScreen *screen);
void visualizer_screen_close_data_source(
    VisualizerScreen *screen);
int32 visualizer_screen_drain_data_source(
    VisualizerScreen *screen);
bool visualizer_screen_find_output_id(
    VisualizerScreen *screen);
NcScreen *visualizer_screen_base(VisualizerScreen *screen);
NcWindow *visualizer_screen_window(VisualizerScreen *screen);
void visualizer_screen_set_geometry(VisualizerScreen *screen,
                                           int32 start_x, int32 start_y,
                                           int32 width, int32 height);
void visualizer_screen_init_visualization(
    VisualizerScreen *screen);
void visualizer_screen_clear(VisualizerScreen *screen);
void visualizer_screen_reset_audio_state(
    VisualizerScreen *screen);
void visualizer_screen_reset_auto_scale_multiplier(
    VisualizerScreen *screen);
void visualizer_screen_toggle_type(VisualizerScreen *screen);
int32 visualizer_screen_requested_samples(
    VisualizerScreen *screen);
bool visualizer_screen_push_samples(VisualizerScreen *screen,
                                           int16 *samples,
                                           int32 samples_len);
int32 visualizer_screen_take_render_samples(
    VisualizerScreen *screen, int16 *dest, int32 dest_len);
int32 visualizer_screen_split_stereo(VisualizerScreen *screen,
                                            int16 *samples,
                                            int32 samples_len);
void visualizer_screen_apply_auto_scale(VisualizerScreen *screen,
                                               int16 *samples,
                                               int32 samples_len);
bool visualizer_screen_draw(VisualizerScreen *screen,
                                   int16 *samples, int32 samples_len);
int16 visualizer_clamp_sample(int32 sample);

#endif /* NCMPCPP_NC_VISUALIZER_H */
