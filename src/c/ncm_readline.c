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
