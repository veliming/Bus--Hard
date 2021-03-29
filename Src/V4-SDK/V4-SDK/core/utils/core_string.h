#ifndef _CORE_STRING_H_
#define _CORE_STRING_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "core_stdinc.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"

int32_t core_str2uint(char *input, uint8_t input_len, uint32_t *output);
int32_t core_uint2str(uint32_t input, char *output, uint8_t *output_len);
int32_t core_uint642str(uint64_t input, char *output, uint8_t *output_len);
int32_t core_int2str(int32_t input, char *output, uint8_t *output_len);
int32_t core_hex2str(uint8_t *input, uint32_t input_len, char *output, uint8_t lowercase);
int32_t core_str2hex(char *input, uint32_t input_len, uint8_t *output);
int32_t core_strdup(aiot_sysdep_portfile_t *sysdep, char **dest, char *src, char *module_name);
int32_t core_sprintf(aiot_sysdep_portfile_t *sysdep, char **dest, char *fmt, char *src[], uint8_t count, char *module_name);
int32_t core_json_value(const char *input, uint32_t input_len, const char *key, uint32_t key_len, char **value, uint32_t *value_len);

#if defined(__cplusplus)
}
#endif

#endif

