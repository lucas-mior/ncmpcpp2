#if !defined(NCMPCPP_NC_VISUALIZER_H)
#define NCMPCPP_NC_VISUALIZER_H

#include <stdbool.h>

#include "c/ncm_sample_buffer.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NATIVE_VISUALIZER_PI 3.14159265358979323846

enum NativeVisualizerType {
    NATIVE_VISUALIZER_WAVE,
    NATIVE_VISUALIZER_WAVE_FILLED,
    NATIVE_VISUALIZER_FREQUENCY,
    NATIVE_VISUALIZER_ELLIPSE,
    NATIVE_VISUALIZER_TYPE_LAST,
};

typedef struct NativeVisualizerScreen {
    NcScreen screen;
    NcWindow window;

    NcmSampleBuffer buffered_samples;
    NcmSampleBuffer rendered_samples;
    NcmSampleBuffer left_channel;
    NcmSampleBuffer right_channel;

    double auto_scale_multiplier;
    enum NativeVisualizerType visualization_type;

    int32 fps;
    int32 sample_rate;
    int32 sample_consumption_rate;
    int32 sample_consumption_rate_up_ctr;
    int32 sample_consumption_rate_dn_ctr;

    bool stereo;
    bool initialized;
} NativeVisualizerScreen;

void native_visualizer_screen_init(NativeVisualizerScreen *screen,
                                   int64 start_x, int64 start_y,
                                   int64 width, int64 height,
                                   NcColor color, NcBorder border,
                                   int32 fps, bool stereo);
void native_visualizer_screen_destroy(NativeVisualizerScreen *screen);
NcScreen *native_visualizer_screen_base(NativeVisualizerScreen *screen);
NcWindow *native_visualizer_screen_window(NativeVisualizerScreen *screen);
void native_visualizer_screen_set_geometry(NativeVisualizerScreen *screen,
                                           int64 start_x, int64 start_y,
                                           int64 width, int64 height);
void native_visualizer_screen_clear(NativeVisualizerScreen *screen);
void native_visualizer_screen_reset_auto_scale_multiplier(
    NativeVisualizerScreen *screen);
void native_visualizer_screen_set_type(NativeVisualizerScreen *screen,
                                       enum NativeVisualizerType type);
void native_visualizer_screen_toggle_type(NativeVisualizerScreen *screen);
enum NativeVisualizerType native_visualizer_screen_type(
    NativeVisualizerScreen *screen);
void native_visualizer_screen_set_stereo(NativeVisualizerScreen *screen,
                                         bool stereo);
void native_visualizer_screen_set_fps(NativeVisualizerScreen *screen,
                                      int32 fps);
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
int32 native_visualizer_screen_left_len(NativeVisualizerScreen *screen);
int32 native_visualizer_screen_right_len(NativeVisualizerScreen *screen);
int16 *native_visualizer_screen_left_data(NativeVisualizerScreen *screen);
int16 *native_visualizer_screen_right_data(NativeVisualizerScreen *screen);
void native_visualizer_screen_apply_auto_scale(NativeVisualizerScreen *screen,
                                               int16 *samples,
                                               int32 samples_len);
int16 native_visualizer_clamp_sample(int64 sample);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_VISUALIZER_H */
