#include "core_global.h"

typedef struct {
    void *mutex;
    uint8_t is_inited;
    uint32_t used_count;
    int32_t alink_id;
} g_core_global_t;

g_core_global_t g_core_clobal = {NULL, 0, 0, 0};

int32_t core_global_init(aiot_sysdep_portfile_t *sysdep)
{
    if (g_core_clobal.is_inited == 1) {
        g_core_clobal.used_count++;
        return STATE_SUCCESS;
    }
    g_core_clobal.is_inited = 1;


    g_core_clobal.mutex = sysdep->core_sysdep_mutex_init();
    g_core_clobal.used_count++;

    return STATE_SUCCESS;
}

int32_t core_global_alink_id_next(aiot_sysdep_portfile_t *sysdep, int32_t *alink_id)
{
    int32_t id = 0;
    sysdep->core_sysdep_mutex_lock(g_core_clobal.mutex);
    if (g_core_clobal.alink_id < 0) {
        g_core_clobal.alink_id = 0;
    }
    id = g_core_clobal.alink_id++;
    sysdep->core_sysdep_mutex_unlock(g_core_clobal.mutex);

    *alink_id = id;
    return STATE_SUCCESS;
}

int32_t core_global_deinit(aiot_sysdep_portfile_t *sysdep)
{
    if (g_core_clobal.used_count > 0) {
        g_core_clobal.used_count--;
    }

    if (g_core_clobal.used_count != 0) {
        return STATE_SUCCESS;
    }
    sysdep->core_sysdep_mutex_deinit(&g_core_clobal.mutex);

    memset(&g_core_clobal, 0, sizeof(g_core_global_t));

    return STATE_SUCCESS;
}

