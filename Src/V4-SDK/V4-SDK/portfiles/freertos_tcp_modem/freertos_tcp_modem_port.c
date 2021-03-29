#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "core_list.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_at_api.h"
//#include "freertos_linkkit.h"

#define AT_RINGBUF_ACCESS_INTERVAL_MS   (50)

typedef struct {
    void *at_handle;
    uint32_t connect_timeout_ms;
    core_sysdep_socket_type_t socket_type;
    aiot_sysdep_network_cred_t *cred;
    char *host;
    uint16_t port;
} core_network_handle_t;

void *core_sysdep_malloc(uint32_t size, char *name)
{
    return pvPortMalloc(size);
}

void core_sysdep_free(void *ptr)
{
    vPortFree(ptr);
}

uint64_t core_sysdep_time(void)
{
    return (uint64_t)(xTaskGetTickCount() * 1000)/configTICK_RATE_HZ;
}

void core_sysdep_sleep(uint64_t time_ms)
{
    vTaskDelay(time_ms / portTICK_PERIOD_MS);
}

void *core_sysdep_network_init(void)
{
    core_network_handle_t *network_handle = pvPortMalloc(sizeof(core_network_handle_t));
    if (network_handle == NULL) {
        return NULL;
    }
    memset(network_handle, 0, sizeof(core_network_handle_t));

    network_handle->at_handle = aiot_at_init();
    if (network_handle->at_handle == NULL) {
        vPortFree(network_handle);
        return NULL;
    }

    return network_handle;
}

int32_t core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == NULL || data == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (option >= CORE_SYSDEP_NETWORK_MAX) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    switch (option) {
        case CORE_SYSDEP_NETWORK_SOCKET_TYPE: {
            network_handle->socket_type = *(core_sysdep_socket_type_t *)data;
        }
        break;
        case CORE_SYSDEP_NETWORK_HOST: {
            network_handle->host = pvPortMalloc(strlen(data) + 1);
            if (network_handle->host == NULL) {
                printf("malloc failed\n");
                return STATE_PORT_MALLOC_FAILED;
            }
            memset(network_handle->host, 0, strlen(data) + 1);
            memcpy(network_handle->host, data, strlen(data));
        }
        break;
        case CORE_SYSDEP_NETWORK_PORT: {
            network_handle->port = *(uint16_t *)data;
        }
        break;
        case CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS: {
            network_handle->connect_timeout_ms = *(uint32_t *)data;
        }
        break;
        default: {
            printf("unknown option\n");
        }
    }

    return STATE_SUCCESS;
}

static int32_t _core_sysdep_network_wait_response(core_network_handle_t *network_handle, aiot_at_recv_option_t option,
        uint8_t *result)
{
    int32_t res = STATE_SUCCESS;
    uint64_t timestart_ms = 0;

    timestart_ms = core_sysdep_time();
    while (1) {
        if (timestart_ms > core_sysdep_time()) {
            timestart_ms = core_sysdep_time();
        }
        if (core_sysdep_time() - timestart_ms > network_handle->connect_timeout_ms) {
            res = STATE_PORT_NETWORK_CONNECT_TIMEOUT;
            break;
        }

        res = aiot_at_recv(network_handle->at_handle, option, result);
        if (res == STATE_SUCCESS && *result == 1) {
            res = STATE_SUCCESS;
            break;
        }
        vTaskDelay(AT_RINGBUF_ACCESS_INTERVAL_MS / portTICK_PERIOD_MS);
    }

    return res;
}

static int32_t _core_sysdep_network_tcp_establish(core_network_handle_t *network_handle)
{
    int32_t res = STATE_SUCCESS;
    aiot_at_connect_t connect;
    uint8_t result = 0;

    memset(&connect, 0, sizeof(aiot_at_connect_t));
    connect.domain = network_handle->host;
    connect.port = network_handle->port;

    res = aiot_at_send(network_handle->at_handle, AIOT_ATSENDOPT_CONNECT_REQ, (void *)&connect);
    if (res < STATE_SUCCESS) {
        return STATE_SYS_DEPEND_NWK_EST_FAILED;
    }

    result = 0;
    res = _core_sysdep_network_wait_response(network_handle, AIOT_ATRECVOPT_SEND_RESP, &result);
    if (res < STATE_SUCCESS) {
        return res;
    }
    if (result == 0) {
        return STATE_SYS_DEPEND_NWK_EST_FAILED;
    }

    result = 0;
    res = _core_sysdep_network_wait_response(network_handle, AIOT_ATRECVOPT_CONNECT_RESP, &result);
    if (res < STATE_SUCCESS) {
        return res;
    }
    if (result == 0) {
        return STATE_SYS_DEPEND_NWK_EST_FAILED;
    }

    return STATE_SUCCESS;
}

int32_t core_sysdep_network_establish(void *handle)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        if (network_handle->host == NULL) {
            return STATE_PORT_MISSING_HOST;
        }
        if (network_handle->cred == NULL) {
            return _core_sysdep_network_tcp_establish(network_handle);
        } else {
            if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE) {
                return _core_sysdep_network_tcp_establish(network_handle);
            }
        }

    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        return STATE_PORT_UDP_CLIENT_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return STATE_PORT_UDP_SERVER_NOT_IMPLEMENT;
    }

    printf("unknown nwk type or tcp host absent\n");

    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}

static int32_t _core_sysdep_network_tcp_recv(core_network_handle_t *network_handle, uint8_t *buffer, uint32_t len,
        uint32_t timeout_ms)
{
    int32_t res = STATE_SUCCESS;
    uint32_t recv_bytes = 0;
    uint64_t timestart_ms = 0, timenow_ms = 0;
    aiot_at_buf_t buf;

    memset(&buf, 0, sizeof(aiot_at_buf_t));

    timestart_ms = core_sysdep_time();
    do {
        timenow_ms = core_sysdep_time();
        if (timeout_ms - (timenow_ms - timestart_ms) > timeout_ms) {
            timestart_ms = timenow_ms;
        }

        buf.buf = buffer + recv_bytes;
        buf.len = len - recv_bytes;
        res = aiot_at_recv(network_handle->at_handle, AIOT_ATRECVOPT_BUF, &buf);
        if (res < STATE_SUCCESS) {
            return STATE_PORT_NETWORK_RECV_CONNECTION_CLOSED;
        } else {
            recv_bytes += res;
        }
        vTaskDelay(AT_RINGBUF_ACCESS_INTERVAL_MS / portTICK_PERIOD_MS);
    } while ((timenow_ms - timestart_ms < timeout_ms) && (recv_bytes < len));

    return recv_bytes;
}

int32_t core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                 core_sysdep_addr_t *addr)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == NULL || buffer == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (len == 0 || timeout_ms == 0) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        if (network_handle->cred == NULL) {
            return _core_sysdep_network_tcp_recv(network_handle, buffer, len, timeout_ms);
        } else {
            if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE) {
                return _core_sysdep_network_tcp_recv(network_handle, buffer, len, timeout_ms);
            }
        }
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        return STATE_PORT_UDP_CLIENT_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return STATE_PORT_UDP_SERVER_NOT_IMPLEMENT;
    }

    printf("unknown nwk type\n");

    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}

int32_t _core_sysdep_network_tcp_send(core_network_handle_t *network_handle, uint8_t *buffer, uint32_t len,
                                      uint32_t timeout_ms)
{
    int32_t res = STATE_SUCCESS;
    uint8_t result = 0;
    uint32_t send_bytes = 0;
    uint64_t timestart_ms = 0, timenow_ms = 0;
    aiot_at_buf_t buf;

    memset(&buf, 0, sizeof(aiot_at_buf_t));

    timestart_ms = core_sysdep_time();
    do {
        timenow_ms = core_sysdep_time();
        if (timeout_ms - (timenow_ms - timestart_ms) > timeout_ms) {
            timestart_ms = timenow_ms;
        }

        buf.buf = buffer + send_bytes;
        buf.len = len - send_bytes;
        res = aiot_at_send(network_handle->at_handle, AIOT_ATSENDOPT_BUF, &buf);
        if (res < STATE_SUCCESS) {
            return STATE_PORT_NETWORK_SEND_CONNECTION_CLOSED;
        } else {
            send_bytes += res;
        }
        vTaskDelay(AT_RINGBUF_ACCESS_INTERVAL_MS / portTICK_PERIOD_MS);
    } while ((timenow_ms - timestart_ms < timeout_ms) && (send_bytes < len));

    res = _core_sysdep_network_wait_response(network_handle, AIOT_ATRECVOPT_SEND_RESP, &result);
    if (res < STATE_SUCCESS) {
        return res;
    }
    if (result == 0) {
        return STATE_PORT_NETWORK_SEND_CONNECTION_CLOSED;
    }

    return send_bytes;
}

int32_t core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                 core_sysdep_addr_t *addr)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == NULL || buffer == NULL) {
        printf("invalid parameter\n");
        return STATE_PORT_INPUT_NULL_POINTER;
    }
    if (len == 0 || timeout_ms == 0) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        if (network_handle->cred == NULL) {
            return _core_sysdep_network_tcp_send(network_handle, buffer, len, timeout_ms);
        } else {
            if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE) {
                return _core_sysdep_network_tcp_send(network_handle, buffer, len, timeout_ms);
            }
        }
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        return STATE_PORT_UDP_CLIENT_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return STATE_PORT_UDP_SERVER_NOT_IMPLEMENT;
    }

    printf("unknown nwk type\n");

    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}

static void _core_sysdep_network_tcp_disconnect(core_network_handle_t *network_handle)
{
    int32_t res = STATE_SUCCESS;
    uint64_t timestart_ms = 0;
    uint8_t result = 0;

    aiot_at_send(network_handle->at_handle, AIOT_ATSENDOPT_DISCONNECT_REQ, NULL);

    while (1) {
        if (timestart_ms > core_sysdep_time()) {
            timestart_ms = core_sysdep_time();
        }
        if (core_sysdep_time() - timestart_ms > network_handle->connect_timeout_ms) {
            break;
        }

        res = aiot_at_recv(network_handle->at_handle, AIOT_ATRECVOPT_DISCONNECT_RESP, &result);
        if (res == STATE_SUCCESS && result == 1) {
            res = STATE_SUCCESS;
            break;
        }
        vTaskDelay(AT_RINGBUF_ACCESS_INTERVAL_MS / portTICK_PERIOD_MS);
    }

    aiot_at_deinit(&network_handle->at_handle);
}

int32_t core_sysdep_network_deinit(void **handle)
{
    core_network_handle_t *network_handle = *(core_network_handle_t **)handle;

    if (handle == NULL || *handle == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    /* Shutdown both send and receive operations. */
    if (network_handle->socket_type == 0 && network_handle->host != NULL) {
        if (network_handle->cred == NULL) {
            _core_sysdep_network_tcp_disconnect(network_handle);
        } else {
            if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE) {
                _core_sysdep_network_tcp_disconnect(network_handle);
            }
        }
    }


    if (network_handle->host != NULL) {
        vPortFree(network_handle->host);
        network_handle->host = NULL;
    }
    if (network_handle->cred != NULL) {
        vPortFree(network_handle->cred);
        network_handle->cred = NULL;
    }

    vPortFree(network_handle);
    *handle = NULL;

    return 0;
}

void core_sysdep_rand(uint8_t *output, uint32_t output_len)
{
    // uint32_t idx = 0, bytes = 0, rand_num = 0;
    // struct timeval time;

    // memset(&time, 0, sizeof(struct timeval));
    // gettimeofday(&time, NULL);

    // srand((unsigned int)(time.tv_sec * 1000 + time.tv_usec / 1000) + rand());

    // for (idx = 0;idx < output_len;) {
    //  if (output_len - idx < 4) {
    //      bytes = output_len - idx;
    //  }else{
    //      bytes = 4;
    //  }
    //  rand_num = rand();
    //  while(bytes-- > 0) {
    //      output[idx++] = (uint8_t)(rand_num >> bytes * 8);
    //  }
    // }
}

void *core_sysdep_mutex_init(void)
{
    return (void *)xSemaphoreCreateMutex();
}

void core_sysdep_mutex_lock(void *mutex)
{
    xSemaphoreTake((SemaphoreHandle_t)mutex, portMAX_DELAY);
}

void core_sysdep_mutex_unlock(void *mutex)
{
    xSemaphoreGive((SemaphoreHandle_t)mutex);
}

void core_sysdep_mutex_deinit(void **mutex)
{
    if (mutex == NULL || *mutex == NULL) {
        return;
    }
    vSemaphoreDelete((SemaphoreHandle_t)*mutex);
}

aiot_sysdep_portfile_t g_aiot_sysdep_portfile = {
    core_sysdep_malloc,
    core_sysdep_free,
    core_sysdep_time,
    core_sysdep_sleep,
    core_sysdep_network_init,
    core_sysdep_network_setopt,
    core_sysdep_network_establish,
    core_sysdep_network_recv,
    core_sysdep_network_send,
    core_sysdep_network_deinit,
    core_sysdep_rand,
    core_sysdep_mutex_init,
    core_sysdep_mutex_lock,
    core_sysdep_mutex_unlock,
    core_sysdep_mutex_deinit
};
