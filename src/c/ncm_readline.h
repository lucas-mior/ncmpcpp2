#if !defined(NCM_READLINE_H)
#define NCM_READLINE_H

#include "c/ncm_defs.h"

NCM_EXTERN_C_BEGIN

char *ncm_readline_prompt(char *prompt);
void ncm_readline_remember(char *line);
void ncm_readline_release(char *line);

NCM_EXTERN_C_END

#endif /* NCM_READLINE_H */
