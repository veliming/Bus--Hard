#include "core_stdinc.h"
#include "core_string.h"
#include "core_list.h"
#include "core_log.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_at_api.h"

#define CORE_AT_MODULE_NAME "AT"

#define CORE_AT_DEFAULT_RINGBUF_LEN (2048)

typedef struct {
    aiot_sysdep_portfile_t *sysdep;
    char *socket_id;
    uint8_t *ringbuf;
    uint32_t head;
    uint32_t tail;
    uint32_t ringbuf_len;
    void *ringbuf_mutex;
    void *data_mutex;
    uint8_t connect_response;
    uint8_t disconnect_response;
    struct core_list_head linked_node;
} core_at_handle_t;

typedef struct {
    struct core_list_head at_handle_list;
    uint8_t send_response;
    void *mutex;
    aiot_at_send_connect_handler_t connect_handler;
    aiot_at_send_buf_handler_t send_handler;
    aiot_at_send_disconnect_handler_t disconnect_handler;
} core_at_global_t;

core_at_global_t g_at_global = {
    .at_handle_list = {
        .prev = &g_at_global.at_handle_list, .next = &g_at_global.at_handle_list
    },
    0,
    NULL,
    NULL,
    NULL,
    NULL
};

static int32_t _core_at_read_ringbuf(core_at_handle_t *at_handle, aiot_at_buf_t *buf)
{
    uint32_t read_bytes = 0;

    if (at_handle->head == at_handle->tail) {
        read_bytes = 0;
    } else if (at_handle->head > at_handle->tail) {
        read_bytes = (buf->len <= (at_handle->head - at_handle->tail)) ? (buf->len) : (at_handle->head - at_handle->tail);

        memcpy(buf->buf, &at_handle->ringbuf[at_handle->tail + 1], read_bytes);
        at_handle->tail += read_bytes;
    } else if (at_handle->head < at_handle->tail) {
        uint32_t ringbuf_bytes_1st = at_handle->ringbuf_len - at_handle->tail - 1;
        uint32_t ringbuf_bytes_2nd = at_handle->head + 1;

        read_bytes = (buf->len <= (ringbuf_bytes_1st + ringbuf_bytes_2nd)) ? (buf->len) : ((
                                 ringbuf_bytes_1st + ringbuf_bytes_2nd));
        if (read_bytes < ringbuf_bytes_1st) {
            memcpy(buf->buf, &at_handle->ringbuf[at_handle->tail + 1], read_bytes);
            at_handle->tail += read_bytes;
        } else {
            uint32_t read_bytes_2nd = read_bytes - ringbuf_bytes_1st;

            memcpy(buf->buf, &at_handle->ringbuf[at_handle->tail + 1], ringbuf_bytes_1st);
            memcpy(&buf->buf[ringbuf_bytes_1st], at_handle->ringbuf, read_bytes_2nd);
            at_handle->tail = read_bytes_2nd - 1;
        }
    }

    return read_bytes;
}

static int32_t _core_at_write_ringbuf(core_at_handle_t *at_handle, aiot_at_buf_t *buf)
{
    uint32_t write_bytes = 0;

    if (buf->len == 0) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    if (at_handle->head < at_handle->tail) {
        write_bytes = (buf->len <= (at_handle->tail - at_handle->head - 1)) ? (buf->len) : (at_handle->tail - at_handle->head -
                      1);

        if (write_bytes > 0) {
            memcpy(&at_handle->ringbuf[at_handle->head + 1], buf->buf, write_bytes);
            at_handle->head += write_bytes - 1;
        }
    } else if (at_handle->head >= at_handle->tail) {
        uint32_t ringbuf_bytes_1st = at_handle->ringbuf_len - at_handle->head - 1;
        uint32_t ringbuf_bytes_2nd = at_handle->tail - 1;

        write_bytes = (buf->len <= (ringbuf_bytes_1st + ringbuf_bytes_2nd)) ? (buf->len) : ((
                                  ringbuf_bytes_1st + ringbuf_bytes_2nd));
        if (write_bytes < ringbuf_bytes_1st) {
            memcpy(&at_handle->ringbuf[at_handle->head + 1], buf->buf, write_bytes);
            at_handle->head += write_bytes;
        } else {
            uint32_t write_bytes_2nd = write_bytes - ringbuf_bytes_1st;

            memcpy(&at_handle->ringbuf[at_handle->head + 1], buf->buf, ringbuf_bytes_1st);
            memcpy(at_handle->ringbuf, &buf->buf[ringbuf_bytes_1st], write_bytes_2nd);
            at_handle->head = (write_bytes_2nd == 0) ? (at_handle->ringbuf_len - 1) : (write_bytes_2nd - 1);
        }
    }

    if (write_bytes < buf->len) {
        core_log1(at_handle->sysdep, STATE_AT_LOG_RINGBUF_OVERRUN, "at ringbuf overrun! ringbuf len: %d bytes\r\n",
                  (void *)&at_handle->ringbuf_len);
        if (write_bytes == 0) {
            return STATE_AT_RINGBUF_OVERRUN;
        }
    }

    return write_bytes;
}

static int32_t _core_at_find_handle_by_socket_id(char *socket_id, core_at_handle_t **at_handle)
{
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    core_at_handle_t *node = NULL;

    if (sysdep == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    core_list_for_each_entry(node, &g_at_global.at_handle_list, linked_node) {
        if (node->socket_id == NULL ||
            socket_id == NULL ||
            ((strlen(node->socket_id) == strlen(socket_id)) &&
             (memcmp(node->socket_id, socket_id, strlen(socket_id)) == 0))) {
            if (socket_id != NULL) {
                res = core_strdup(sysdep, &node->socket_id, socket_id, CORE_AT_MODULE_NAME);
            }
            if (res >= STATE_SUCCESS) {
                *at_handle = node;
            }
            break;
        }
    }

    return res;
}

int32_t aiot_at_set_send_handler(aiot_at_send_handler_t *handler)
{
    if (handler == NULL || handler->connect_handler == NULL ||
        handler->disconnect_handler == NULL || handler->send_handler == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    g_at_global.connect_handler = handler->connect_handler;
    g_at_global.send_handler = handler->send_handler;
    g_at_global.disconnect_handler = handler->disconnect_handler;

    return STATE_SUCCESS;
}

void *aiot_at_init(void)
{
    core_at_handle_t *at_handle = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return NULL;
    }

    at_handle = sysdep->core_sysdep_malloc(sizeof(core_at_handle_t), CORE_AT_MODULE_NAME);
    if (at_handle == NULL) {
        return NULL;
    }
    memset(at_handle, 0, sizeof(core_at_handle_t));

    at_handle->sysdep = sysdep;
    at_handle->ringbuf_len = CORE_AT_DEFAULT_RINGBUF_LEN;

    at_handle->ringbuf = sysdep->core_sysdep_malloc(at_handle->ringbuf_len, CORE_AT_MODULE_NAME);
    if (at_handle->ringbuf == NULL) {
        sysdep->core_sysdep_free(at_handle);
        return NULL;
    }
    memset(at_handle->ringbuf, 0, at_handle->ringbuf_len);

    at_handle->ringbuf_mutex = sysdep->core_sysdep_mutex_init();
    at_handle->data_mutex = sysdep->core_sysdep_mutex_init();
    CORE_INIT_LIST_HEAD(&at_handle->linked_node);

    if (core_list_empty(&g_at_global.at_handle_list)) {
        g_at_global.mutex = sysdep->core_sysdep_mutex_init();
    }

    core_list_add_tail(&at_handle->linked_node, &g_at_global.at_handle_list);

    return at_handle;
}

int32_t aiot_at_send(void *handle, aiot_at_send_option_t option, void *data)
{
    int32_t res = STATE_SUCCESS;
    core_at_handle_t *at_handle = (core_at_handle_t *)handle;

    if (at_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (option >= AIOT_ATSENDOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    switch (option) {
        case AIOT_ATSENDOPT_CONNECT_REQ: {
            if (g_at_global.connect_handler != NULL && data != NULL) {
                res = g_at_global.connect_handler(at_handle, (aiot_at_connect_t *)data);
            }
        }
        break;
        case AIOT_ATSENDOPT_BUF: {
            if (g_at_global.send_handler != NULL && data != NULL) {
                res = g_at_global.send_handler(at_handle->socket_id, (aiot_at_buf_t *)data);
            }
        }
        break;
        case AIOT_ATSENDOPT_DISCONNECT_REQ: {
            if (g_at_global.disconnect_handler != NULL) {
                res = g_at_global.disconnect_handler(at_handle->socket_id);
            }
        }
        break;
        default: {
            res = STATE_USER_INPUT_UNKNOWN_OPTION;
        }
    }

    return res;
}

int32_t aiot_at_recv(void *handle, aiot_at_recv_option_t option, void *data)
{
    int32_t res = STATE_SUCCESS;
    core_at_handle_t *at_handle = (core_at_handle_t *)handle;

    if (at_handle == NULL || data == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (option >= AIOT_ATRECVOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    at_handle->sysdep->core_sysdep_mutex_lock(at_handle->data_mutex);
    switch (option) {
        case AIOT_ATRECVOPT_CONNECT_RESP: {
            *(uint8_t *)data = at_handle->connect_response;
            at_handle->connect_response = 0;
        }
        break;
        case AIOT_ATRECVOPT_SEND_RESP: {
            *(uint8_t *)data = g_at_global.send_response;
            g_at_global.send_response = 0;
        }
        break;
        case AIOT_ATRECVOPT_BUF: {
            at_handle->sysdep->core_sysdep_mutex_unlock(at_handle->data_mutex);
            at_handle->sysdep->core_sysdep_mutex_lock(at_handle->ringbuf_mutex);
            res = _core_at_read_ringbuf(at_handle, (aiot_at_buf_t *)data);
            at_handle->sysdep->core_sysdep_mutex_unlock(at_handle->ringbuf_mutex);
            at_handle->sysdep->core_sysdep_mutex_lock(at_handle->data_mutex);
        }
        break;
        case AIOT_ATRECVOPT_DISCONNECT_RESP: {
            *(uint8_t *)data = at_handle->disconnect_response;
            at_handle->disconnect_response = 0;
        }
        break;
        default: {
            res = STATE_USER_INPUT_UNKNOWN_OPTION;
        }
    }
    at_handle->sysdep->core_sysdep_mutex_unlock(at_handle->data_mutex);

    return res;
}

int32_t aiot_at_deinit(void **handle)
{
    core_at_handle_t *at_handle = NULL;

    if (handle == NULL || *handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    at_handle = *(core_at_handle_t **)handle;

    core_list_del(&at_handle->linked_node);

    if (core_list_empty(&g_at_global.at_handle_list)) {
        at_handle->sysdep->core_sysdep_mutex_deinit(&g_at_global.mutex);
    }

    at_handle->sysdep->core_sysdep_mutex_deinit(&at_handle->ringbuf_mutex);
    at_handle->sysdep->core_sysdep_mutex_deinit(&at_handle->data_mutex);

    if (at_handle->ringbuf != NULL) {
        at_handle->sysdep->core_sysdep_free(at_handle->ringbuf);
        at_handle->ringbuf = NULL;
    }

    if (at_handle->socket_id != NULL) {
        at_handle->sysdep->core_sysdep_free(at_handle->socket_id);
        at_handle->socket_id = NULL;
    }

    at_handle->sysdep->core_sysdep_free(at_handle);

    *handle = NULL;

    return STATE_SUCCESS;
}

int32_t aiot_at_setopt(void *handle, aiot_at_option_t option, void *data)
{
    int32_t res = STATE_SUCCESS;
    core_at_handle_t *at_handle = (core_at_handle_t *)handle;

    if (at_handle == NULL || data == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (option >= AIOT_ATOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    at_handle->sysdep->core_sysdep_mutex_lock(at_handle->data_mutex);
    switch (option) {
        case AIOT_ATOPT_SOCKET_ID: {
            res = core_strdup(at_handle->sysdep, &at_handle->socket_id, data, CORE_AT_MODULE_NAME);
        }
        break;
        case AIOT_ATOPT_RING_BUF_LEN: {
            if (at_handle->ringbuf != NULL) {
                at_handle->sysdep->core_sysdep_free(at_handle->ringbuf);
                at_handle->ringbuf = NULL;
            }
            at_handle->ringbuf_len = *(uint32_t *)data;
            at_handle->ringbuf = at_handle->sysdep->core_sysdep_malloc(at_handle->ringbuf_len, CORE_AT_MODULE_NAME);
            if (at_handle->ringbuf == NULL) {
                at_handle->ringbuf_len = 0;
                res = STATE_SYS_DEPEND_MALLOC_FAILED;
            } else {
                memset(at_handle->ringbuf, 0, at_handle->ringbuf_len);
                at_handle->head = 0;
                at_handle->tail = 0;
                res = STATE_SUCCESS;
            }
        }
        break;
        default: {
            res = STATE_USER_INPUT_UNKNOWN_OPTION;
        }
    }
    at_handle->sysdep->core_sysdep_mutex_unlock(at_handle->data_mutex);

    return res;
}

int32_t aiot_at_input(char *socket_id, aiot_at_recv_option_t option, void *data)
{
    int32_t res = STATE_SUCCESS;
    core_at_handle_t *at_handle = NULL;

    if (data == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (option >= AIOT_ATRECVOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    res = _core_at_find_handle_by_socket_id(socket_id, &at_handle);
    if (res < STATE_SUCCESS) {
        return res;
    }

    at_handle->sysdep->core_sysdep_mutex_lock(at_handle->data_mutex);
    switch (option) {
        case AIOT_ATRECVOPT_CONNECT_RESP: {
            at_handle->connect_response = *(uint8_t *)data;
        }
        break;
        case AIOT_ATRECVOPT_SEND_RESP: {
            g_at_global.send_response = *(uint8_t *)data;
        }
        break;
        case AIOT_ATRECVOPT_BUF: {
            at_handle->sysdep->core_sysdep_mutex_unlock(at_handle->data_mutex);
            at_handle->sysdep->core_sysdep_mutex_lock(at_handle->ringbuf_mutex);
            res = _core_at_write_ringbuf(at_handle, (aiot_at_buf_t *)data);
            at_handle->sysdep->core_sysdep_mutex_unlock(at_handle->ringbuf_mutex);
            at_handle->sysdep->core_sysdep_mutex_lock(at_handle->data_mutex);
        }
        break;
        case AIOT_ATRECVOPT_DISCONNECT_RESP: {
            at_handle->disconnect_response = *(uint8_t *)data;
        }
        break;
        default: {
            res = STATE_USER_INPUT_UNKNOWN_OPTION;
        }
    }
    at_handle->sysdep->core_sysdep_mutex_unlock(at_handle->data_mutex);

    return res;
}