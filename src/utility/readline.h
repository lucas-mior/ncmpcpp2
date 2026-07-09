#ifndef NCMPCPP_UTILITY_READLINE_H
#define NCMPCPP_UTILITY_READLINE_H

#include "config.h"

#if defined(HAVE_READLINE_HISTORY_H)
# include <readline/history.h>
#endif // HAVE_READLINE_HISTORY

#if defined(HAVE_READLINE_H)
# include <readline.h>
#elif defined(HAVE_READLINE_READLINE_H)
# include <readline/readline.h>
#else
# error "readline is not available"
#endif

#endif // NCMPCPP_READLINE_UTILITY_H
