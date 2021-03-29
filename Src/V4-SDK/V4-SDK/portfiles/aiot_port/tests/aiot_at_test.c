#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "cu_test.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_at_api.h"

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

static pthread_t g_at_recv_thread;
static uint8_t g_at_recv_thread_running;
static char *text = "MQTT is a machine-to-machine (M2M)/\"Internet of Things\" connectivity protocol. "
                    "It was designed as an extremely lightweight publish/subscribe messaging transport. It is useful"
                    " for connections with remote locations where a small code footprint is required and/or network "
                    "bandwidth is at a premium. For example, it has been used in sensors communicating to a broker via"
                    " satellite link, over occasional dial-up connections with healthcare providers, and in a range of "
                    "home automation and small device scenarios. It is also ideal for mobile applications because of "
                    "its small size, low power usage, minimised data packets, and efficient distribution of information"
                    " to one or many receivers\n";

/* 日志回调函数, SDK的日志会从这里输出 */
int32_t aiot_at_test_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

DATA(PORTFILES_AT)
{
    void *at_handle;
};

int32_t demo_case_02_at_send_connect_handler(void *handle, aiot_at_connect_t *connect)
{
    return 0;
}

int32_t demo_case_02_at_send_buf_handler(char *socket_id, aiot_at_buf_t *buf)
{
    return 0;
}

int32_t demo_case_02_at_send_disconnect_handler(char *socket_id)
{
    return 0;
}

SETUP(PORTFILES_AT)
{
    aiot_at_send_handler_t send_handler = {
        .connect_handler = demo_case_02_at_send_connect_handler,
        .send_handler = demo_case_02_at_send_buf_handler,
        .disconnect_handler = demo_case_02_at_send_disconnect_handler
    };
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    aiot_state_set_logcb(aiot_at_test_logcb);
    aiot_at_set_send_handler(&send_handler);

    data->at_handle = aiot_at_init();
}

TEARDOWN(PORTFILES_AT)
{
    aiot_at_deinit(&data->at_handle);
}

typedef struct {
    void *at_handle;
    uint32_t recv_bytes;
} case_at_recv_data_t;

void *case_01_at_recv_thread(void *args)
{
    int32_t res = 0;
    uint8_t buf[512] = {0};
    aiot_at_buf_t recv_buf = {
        .buf = buf,
        .len = 512
    };
    case_at_recv_data_t *recv_data = (case_at_recv_data_t *)args;

    while (g_at_recv_thread_running) {
        memset(recv_buf.buf, 0, 512);

        res = aiot_at_recv(recv_data->at_handle, AIOT_ATRECVOPT_BUF, (void *)&recv_buf);
        if (res > 0) {
            recv_data->recv_bytes += res;
            printf("aiot_at_recv, res: %d\n", res);
            printf("%s", recv_buf.buf);
        }
        usleep(10 * 1000);
    }
    return NULL;
}


CASEs(PORTFILES_AT, case_01_ringbuf)
{
    int32_t res = 0;
    uint8_t init_count = 10, count = 0;
    uint32_t ringbuf_len = 1024;
    char *at_socket_id = "12345678";
    aiot_at_buf_t input_buf = {
        .buf = (uint8_t *)text,
        .len = (uint32_t)strlen(text)
    };
    case_at_recv_data_t recv_data = {
        .at_handle = data->at_handle,
        .recv_bytes = 0
    };

    printf("input_buf.len: %d\n", input_buf.len);

    aiot_at_setopt(data->at_handle, AIOT_ATOPT_SOCKET_ID, (void *)at_socket_id);
    aiot_at_setopt(data->at_handle, AIOT_ATOPT_RING_BUF_LEN, (void *)&ringbuf_len);

    g_at_recv_thread_running = 1;
    res = pthread_create(&g_at_recv_thread, NULL, case_01_at_recv_thread, (void *)&recv_data);
    if (res != 0) {
        printf("pthread_create failed\n");
        return;
    }

    count = init_count;
    while (count-- > 0) {
        res = aiot_at_input(NULL, AIOT_ATRECVOPT_BUF, (void *)&input_buf);
        if (res > 0) {
            printf("aiot_at_input, res: %d\n", res);
        } else {
            printf("aiot_at_input, res: -0x%04X\n", -res);
        }

        usleep(300 * 1000);
    }

    g_at_recv_thread_running = 0;
    pthread_join(g_at_recv_thread, NULL);

    ASSERT_EQ(recv_data.recv_bytes, init_count * strlen(text));
}

void *case_02_at_recv_thread(void *args)
{
    int32_t res = 0;
    uint8_t buf[512] = {0};
    aiot_at_buf_t recv_buf = {
        .buf = buf,
        .len = 512
    };
    case_at_recv_data_t *recv_data = (case_at_recv_data_t *)args;

    while (g_at_recv_thread_running) {
        memset(recv_buf.buf, 0, 512);

        res = aiot_at_recv(recv_data->at_handle, AIOT_ATRECVOPT_BUF, (void *)&recv_buf);
        if (res > 0) {
            recv_data->recv_bytes += res;
            printf("aiot_at_recv, res: %d\n", res);
            printf("%s", recv_buf.buf);
        }
        usleep(10 * 1000);
        break;
    }
    return NULL;
}

CASEs(PORTFILES_AT, case_02_ringbuf_head_le_tail)
{
    int32_t res = 0;
    uint8_t count = 0;
    uint32_t ringbuf_len = 1024;
    char *at_socket_id = "12345678";
    aiot_at_buf_t input_buf = {
        .buf = (uint8_t *)text,
        .len = (uint32_t)strlen(text)
    };
    case_at_recv_data_t recv_data = {
        .at_handle = data->at_handle,
        .recv_bytes = 0
    };

    printf("input_buf.len: %d\n", input_buf.len);

    aiot_at_setopt(data->at_handle, AIOT_ATOPT_SOCKET_ID, (void *)at_socket_id);
    aiot_at_setopt(data->at_handle, AIOT_ATOPT_RING_BUF_LEN, (void *)&ringbuf_len);

    g_at_recv_thread_running = 1;
    res = pthread_create(&g_at_recv_thread, NULL, case_02_at_recv_thread, (void *)&recv_data);
    if (res != 0) {
        printf("pthread_create failed\n");
        return;
    }

    count = 3;
    while (count-- > 0) {
        res = aiot_at_input(at_socket_id, AIOT_ATRECVOPT_BUF, (void *)&input_buf);
        if (res > 0) {
            printf("aiot_at_input, res: %d\n", res);
        } else {
            printf("aiot_at_input, res: -0x%04X\n", -res);
        }

        usleep(300 * 1000);
    }

    g_at_recv_thread_running = 0;
    pthread_join(g_at_recv_thread, NULL);

    ASSERT_EQ(recv_data.recv_bytes, 512);
}

CASEs(PORTFILES_AT, case_03_aiot_at_send)
{
    int32_t res = STATE_SUCCESS;
    char *payload = "123";
    aiot_at_connect_t connect;
    aiot_at_buf_t buf = {
        .buf = (uint8_t *)payload,
        .len = (uint32_t)strlen(payload)
    };

    memset(&connect, 0, sizeof(aiot_at_connect_t));
    connect.domain = "www.baidu.com";
    connect.port = 123;

    res = aiot_at_send(data->at_handle, AIOT_ATSENDOPT_CONNECT_REQ, (void *)&connect);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_at_send(data->at_handle, AIOT_ATSENDOPT_BUF, &buf);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_at_send(data->at_handle, AIOT_ATSENDOPT_DISCONNECT_REQ, NULL);
    ASSERT_EQ(res, STATE_SUCCESS);
}
CASEs(PORTFILES_AT, case_04_aiot_at_input_recv)
{
    int32_t res = STATE_SUCCESS;
    uint8_t input_value = 1;
    uint8_t recv_value = 0;

    res = aiot_at_input(data->at_handle, AIOT_ATRECVOPT_CONNECT_RESP, (void *)&input_value);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_at_input(data->at_handle, AIOT_ATRECVOPT_SEND_RESP, (void *)&input_value);
    ASSERT_EQ(res, STATE_SUCCESS);

    res = aiot_at_input(data->at_handle, AIOT_ATRECVOPT_DISCONNECT_RESP, (void *)&input_value);
    ASSERT_EQ(res, STATE_SUCCESS);

    recv_value = 0;
    res = aiot_at_recv(data->at_handle, AIOT_ATRECVOPT_CONNECT_RESP, (void *)&recv_value);
    ASSERT_EQ(res, STATE_SUCCESS);
    ASSERT_EQ(recv_value, input_value);

    recv_value = 0;
    res = aiot_at_recv(data->at_handle, AIOT_ATRECVOPT_SEND_RESP, (void *)&recv_value);
    ASSERT_EQ(res, STATE_SUCCESS);
    ASSERT_EQ(recv_value, input_value);

    recv_value = 0;
    res = aiot_at_recv(data->at_handle, AIOT_ATRECVOPT_DISCONNECT_RESP, (void *)&recv_value);
    ASSERT_EQ(res, STATE_SUCCESS);
    ASSERT_EQ(recv_value, input_value);
}

SUITE(PORTFILES_AT) = {
    ADD_CASE(PORTFILES_AT, case_01_ringbuf),
    ADD_CASE(PORTFILES_AT, case_02_ringbuf_head_le_tail),
    ADD_CASE(PORTFILES_AT, case_03_aiot_at_send),
    ADD_CASE(PORTFILES_AT, case_04_aiot_at_input_recv),
    ADD_CASE_NULL
};