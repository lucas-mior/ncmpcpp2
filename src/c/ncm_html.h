#if !defined(NCM_HTML_H)
#define NCM_HTML_H

#include "c/ncm_base.h"

NcmBuffer ncm_html_unescape_utf8(char *data, int32 data_len);
NcmBuffer ncm_html_unescape_entities(char *data, int32 data_len);
NcmBuffer ncm_html_strip_tags(char *data, int32 data_len);

#endif /* NCM_HTML_H */
