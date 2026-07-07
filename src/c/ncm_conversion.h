#if !defined(NCM_CONVERSION_H)
#define NCM_CONVERSION_H

#include "c/ncm_error.h"

NCM_EXTERN_C_BEGIN

bool ncm_parse_int32(char *source, int32 source_len,
                     int32 *out, NcmError *error);
bool ncm_parse_uint32(char *source, int32 source_len,
                      uint32 *out, NcmError *error);
bool ncm_parse_int64(char *source, int32 source_len,
                     int64 *out, NcmError *error);
bool ncm_parse_uint64(char *source, int32 source_len,
                      uint64 *out, NcmError *error);
bool ncm_parse_double(char *source, int32 source_len,
                      double *out, NcmError *error);

bool ncm_bounds_check_i64(int64 value, int64 lbound, int64 ubound,
                          NcmError *error);
bool ncm_lower_bound_check_i64(int64 value, int64 lbound,
                               NcmError *error);
bool ncm_upper_bound_check_i64(int64 value, int64 ubound,
                               NcmError *error);

bool ncm_bounds_check_u64(uint64 value, uint64 lbound, uint64 ubound,
                          NcmError *error);
bool ncm_lower_bound_check_u64(uint64 value, uint64 lbound,
                               NcmError *error);
bool ncm_upper_bound_check_u64(uint64 value, uint64 ubound,
                               NcmError *error);

bool ncm_bounds_check_f64(double value, double lbound, double ubound,
                          NcmError *error);
bool ncm_lower_bound_check_f64(double value, double lbound,
                               NcmError *error);
bool ncm_upper_bound_check_f64(double value, double ubound,
                               NcmError *error);

NCM_EXTERN_C_END

#endif /* NCM_CONVERSION_H */
