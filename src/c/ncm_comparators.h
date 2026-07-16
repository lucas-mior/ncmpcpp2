#if !defined(NCM_COMPARATORS_H)
#define NCM_COMPARATORS_H

#include <stdbool.h>

#include "cbase/primitives.h"

int32 ncm_compare_locale_strings(char *left, int32 left_len,
                                  char *right, int32 right_len,
                                  bool ignore_the);

#endif /* NCM_COMPARATORS_H */
