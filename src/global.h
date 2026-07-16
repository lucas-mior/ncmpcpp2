#if !defined(NCMPCPP_GLOBAL_H)
#define NCMPCPP_GLOBAL_H

#include "c/ncm_base.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_random.h"
#include "c/ncm_time.h"

extern bool global_show_messages;
extern bool global_seeking_in_progress;
extern NcmBuffer global_volume_state;
extern NcmTimePoint global_timer;
extern NcmRandom global_random;
extern NcmMpdClient global_mpd;

void global_state_init(void);
void global_state_destroy(void);
bool global_timer_update(NcmError *error);
int64 global_timer_elapsed_ms(NcmTimePoint start);
int64 global_timer_elapsed_seconds(NcmTimePoint start);
void global_volume_state_set(char *string, int32 string_len);
void global_volume_state_append(char *string, int32 string_len);
char *global_volume_state_cstr(void);
int32 global_volume_state_len(void);

#endif /* NCMPCPP_GLOBAL_H */
