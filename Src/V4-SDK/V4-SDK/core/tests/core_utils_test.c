#include "cu_test.h"
#include "core_sha256.h"
#include "core_string.h"

CASE(CORE_UTILS, utils_sha256)
{
    char       *plain_text = "hello, world!";
    uint32_t    plain_ilen = strlen(plain_text);
    uint8_t     output[32];
    char        output_str[65];

    memset(output, 0, sizeof(output));
    core_sha256((uint8_t *)plain_text, plain_ilen, output);

    memset(output_str, 0, sizeof(output_str));
    core_hex2str(output, sizeof(output), output_str, 1);

    ASSERT_STR_EQ(output_str,
                  "68e656b251e67e8358bef8483ab0d51c6619f3e7a1a9f0e75838d41ff368f728");
}


CASE(CORE_UTILS, core_str2uint_normal)
{
    typedef struct  {
        char *str;
        uint32_t uint32;
    } str2uint_t;

    uint32_t i = 0;
    str2uint_t test_data[] = {
        { "0", 0 },
        { "1", 1 },
        { "12345", 12345 },
        { "4294967295", 4294967295},
    };
    int32_t res = 0;

    for (i = 0; i < (sizeof(test_data) / sizeof(str2uint_t)); i++) {
        uint32_t output = 0;
        res = core_str2uint(test_data[i].str, strlen(test_data[i].str), &output);
        ASSERT_EQ(res, 0);
        ASSERT_EQ(output, test_data[i].uint32);
    }
}

CASE(CORE_UTILS, core_str2uint_unormal)
{
    typedef struct  {
        char *str;
        uint32_t uint32;
    } str2uint_t;

    uint32_t i = 0;
    str2uint_t test_data[] = {
        { "0.0", 0 },
        { "-1", 0 },
        { "acb12345", 0 },
        { "12345abc", 0 },
        // { "12345678901", 0}, todo: not check value limit
    };
    int32_t res = 0;

    for (i = 0; i < (sizeof(test_data) / sizeof(str2uint_t)); i++) {
        uint32_t output = 0;
        res = core_str2uint(test_data[i].str, strlen(test_data[i].str), &output);
        ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);
        ASSERT_EQ(output, 0);
    }
}

CASE(CORE_UTILS, core_uint2str_normal)
{
    typedef struct  {
        char *str;
        uint32_t uint32;
    } str2uint_t;

    uint32_t i = 0;
    str2uint_t test_data[] = {
        { "0", 0 },
        { "1", 1 },
        { "12345", 12345 },
        { "4294967295", 4294967295},
    };
    int32_t res = 0;

    for (i = 0; i < (sizeof(test_data) / sizeof(str2uint_t)); i++) {
        char output[11] = {0};
        uint8_t output_len = 0;
        res = core_uint2str(test_data[i].uint32, output, &output_len);
        ASSERT_EQ(res, 0);
        ASSERT_STR_EQ(output, test_data[i].str);
        ASSERT_EQ(output_len, strlen(test_data[i].str));
    }
}

CASE(CORE_UTILS, core_uint642str_normal)
{
    typedef struct  {
        char *str;
        uint64_t uint64;
    } uint642str_t;

    uint32_t i = 0;
    uint642str_t test_data[] = {
        { "0", 0 },
        { "1", 1 },
        { "12345", 12345 },
        { "18446744073709551615", 18446744073709551615UL},
    };
    int32_t res = 0;

    for (i = 0; i < (sizeof(test_data) / sizeof(uint642str_t)); i++) {
        char output[21] = {0};
        uint8_t output_len = 0;
        res = core_uint642str(test_data[i].uint64, output, &output_len);
        ASSERT_EQ(res, 0);
        ASSERT_STR_EQ(output, test_data[i].str);
        ASSERT_EQ(output_len, strlen(test_data[i].str));
    }
}

CASE(CORE_UTILS, core_int2str_normal)
{
    typedef struct  {
        char *str;
        int32_t int32;
    } int2str_t;

    uint32_t i = 0;
    int2str_t test_data[] = {
        { "0", 0 },
        { "1", 1 },
        { "12345", 12345 },
        { "-12345", -12345 },
        { "2147483647", 0x7FFFFFFF},
        { "-2147483648", 0x80000000}
    };
    int32_t res = 0;

    for (i = 0; i < (sizeof(test_data) / sizeof(int2str_t)); i++) {
        char output[21] = {0};
        uint8_t output_len = 0;
        res = core_int2str(test_data[i].int32, output, &output_len);
        ASSERT_EQ(res, 0);
        ASSERT_STR_EQ(output, test_data[i].str);
        ASSERT_EQ(output_len, strlen(test_data[i].str));
    }
}

CASE(CORE_UTILS, core_hex2str_normal)
{
    typedef struct  {
        char *str_lower;
        char *str_uppper;
        uint8_t *hex_array;
        uint32_t hex_array_len;
    } hex2str_t;

    uint32_t i = 0;
    hex2str_t test_data[] = {
        { "00", "00", (uint8_t *)"\x00", 1 },
        { "ff", "FF", (uint8_t *)"\xFF", 1 },
        { "ab", "AB", (uint8_t *)"\xAB", 1 },
        { "abcd", "ABCD", (uint8_t *)"\xAB\xCD", 2 },
        { "12cd00", "12CD00", (uint8_t *)"\x12\xCD\x00", 3 },
        { "00000000", "00000000", (uint8_t *)"\x00\x00\x00\x00", 4 },
    };
    int32_t res = 0;

    for (i = 0; i < (sizeof(test_data) / sizeof(hex2str_t)); i++) {
        char output_lower[32] = {0};
        char output_uppper[32] = {0};
        res = core_hex2str(test_data[i].hex_array, test_data[i].hex_array_len, output_lower, 1);
        res = core_hex2str(test_data[i].hex_array, test_data[i].hex_array_len, output_uppper, 0);
        ASSERT_EQ(res, 0);
        ASSERT_STR_EQ(output_lower, test_data[i].str_lower);
        ASSERT_STR_EQ(output_uppper, test_data[i].str_uppper);
    }
}

CASE(CORE_UTILS, core_str2hex)
{
    typedef struct  {
        char *str;
        uint8_t hex_array[32];
    } str2hex_t;

    uint32_t i = 0;
    str2hex_t test_data[] = {
        { "00", {0x00} },
        { "fF", {0xFF} },
        { "aB", {0xAB} },
        { "00aBCdEF1200", {0x00, 0xAB, 0xCD, 0xEF, 0x12, 0x00} },
    };
    int32_t res = 0;

    for (i = 0; i < (sizeof(test_data) / sizeof(str2hex_t)); i++) {
        uint8_t output[32] = {0};
        memset(output, 0xA1, sizeof(output));

        res = core_str2hex(test_data[i].str, strlen(test_data[i].str), output);
        ASSERT_EQ(res, 0);
        ASSERT_EQ(memcmp(output, test_data[i].hex_array, strlen(test_data[i].str) / 2), 0) ;
    }

    res = core_str2hex(NULL, 11, NULL);
    ASSERT_EQ(res, STATE_USER_INPUT_OUT_RANGE);
}

CASE(CORE_UTILS, core_strdup)
{
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);

    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    char *dest = NULL;

    core_strdup(sysdep, &dest, "core_strdup test", "ut");
    ASSERT_STR_EQ(dest, "core_strdup test");

    core_strdup(sysdep, &dest, "core_strdup test again", "ut");
    ASSERT_STR_EQ(dest, "core_strdup test again");

    sysdep->core_sysdep_free(dest);
    aiot_sysdep_set_portfile(NULL);
}

CASE(CORE_UTILS, core_sprintf)
{
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);

    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    char temp[64] = {0};
    char *dest = NULL;
    char *fmt = "%s1%s2%s3%s";
    char *src[] = {
        "str1",
        "str2",
        "str3",
        "str4",
    };

    core_sprintf(sysdep, &dest, "%s1%s2%s3%s", src, sizeof(src) / sizeof(char *), "ut");
    snprintf(temp, sizeof(temp), fmt, src[0], src[1], src[2], src[3]);
    ASSERT_STR_EQ(dest, temp);

    sysdep->core_sysdep_free(dest);
    aiot_sysdep_set_portfile(NULL);
}

int32_t core_json_value(const char *input, uint32_t input_len, const char *key, uint32_t key_len, char **value, uint32_t *value_len);


CASE(CORE_UTILS, core_json_value)
{
    uint8_t i = 0;
    char *json = \
    "{\"key_str\":\"val_str\",\"key_int\":123,\"key_float\":123.123,\"key_struct\":{\"key_member\":123},\"key_array\":[1, 2, 3]}";
    char *kv[][2] = {
        {"key_str", "val_str"},
        {"key_int", "123"},
        {"key_float", "123.123"},
        {"key_struct", "{\"key_member\":123}"},
        {"key_member", "123"},
        {"key_array", "[1, 2, 3]"}
    };

    for (i = 0; i<sizeof(kv)/sizeof(kv[1]); i++) {
        char *value = NULL;
        uint32_t value_len = 0;

        core_json_value(json, strlen(json), kv[i][0], strlen(kv[i][0]), &value, &value_len);
        ASSERT_EQ(strlen(kv[i][1]), value_len);
        ASSERT_EQ(memcmp(kv[i][1], value, value_len), 0);
    }
}

SUITE(CORE_UTILS) = {
    ADD_CASE(CORE_UTILS, utils_sha256),
    ADD_CASE(CORE_UTILS, core_str2uint_normal),
    ADD_CASE(CORE_UTILS, core_str2uint_unormal),
    ADD_CASE(CORE_UTILS, core_uint2str_normal),
    ADD_CASE(CORE_UTILS, core_uint642str_normal),
    ADD_CASE(CORE_UTILS, core_int2str_normal),
    ADD_CASE(CORE_UTILS, core_hex2str_normal),
    ADD_CASE(CORE_UTILS, core_str2hex),
    ADD_CASE(CORE_UTILS, core_strdup),
    ADD_CASE(CORE_UTILS, core_sprintf),
    ADD_CASE(CORE_UTILS, core_json_value),
    ADD_CASE_NULL
};

