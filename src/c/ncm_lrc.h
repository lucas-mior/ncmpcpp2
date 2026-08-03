#if !defined(NCM_LRC_H)
#define NCM_LRC_H

#include "cbase.h"

#include "c/ncm_error.h"

#define NCM_LRC_NO_BUFFER_POSITION (-1)

typedef struct NcmLrcEntry {
    int32 time_ms;
    int32 text_start;
    int32 text_len;
    int32 buffer_start;
    int32 buffer_end;
    int32 source_order;
} NcmLrcEntry;

typedef struct NcmLrcDocument {
    StrBuilder text;
    NcmLrcEntry *entries;

    int32 entries_len;
    int32 entries_cap;
    int32 offset_ms;
    bool has_offset;
} NcmLrcDocument;

void ncm_lrc_document_init(NcmLrcDocument *document);
void ncm_lrc_document_clear(NcmLrcDocument *document);
void ncm_lrc_document_destroy(NcmLrcDocument *document);
bool ncm_lrc_parse(NcmLrcDocument *document,
                   char *data, int32 data_len,
                   NcmError *error);
NcmStringView ncm_lrc_entry_text(NcmLrcDocument *document,
                                 NcmLrcEntry *entry);

#endif /* NCM_LRC_H */
