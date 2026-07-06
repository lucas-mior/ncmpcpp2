#include "c/ncm_readline.h"

#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#if defined(HAVE_READLINE_HISTORY_H)
#include <readline/history.h>
#endif

#if defined(HAVE_READLINE_H)
#include <readline.h>
#elif defined(HAVE_READLINE_READLINE_H)
#include <readline/readline.h>
#else
#error "readline is not available"
#endif

char *
ncm_readline_prompt(char *prompt) {
    return readline(prompt);
}

void
ncm_readline_remember(char *line) {
#if defined(HAVE_READLINE_HISTORY_H)
    if ((line != NULL) && (line[0] != '\0')) {
        add_history(line);
    }
#else
    (void)line;
#endif
    return;
}

void
ncm_readline_release(char *line) {
    free(line);
    return;
}
