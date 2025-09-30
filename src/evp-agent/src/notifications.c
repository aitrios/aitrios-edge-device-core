/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <evp/agent.h>
#include <evp/sdk.h>
#include <evp/agent_config.h>

#include "system_manager.h"
#include "led_manager.h"
#include "esf.h"
#include "notifications.h"
#include "log.h"

/* Bit7 - Bit4 */
#define MQTT_RECONNECT_CNT_MAX (0xF)
#define MQTT_RECONNECT_CNT_MASK (0xF0)
#define MQTT_RECONNECT_CNT_SHIFT (4)

/* Bit3 and Bit 2 */
#define MQTT_ERR_RECV_BUFF_MASK (0x08)
#define MQTT_ERR_SEND_BUFF_MASK (0x04)

/* Bit 1 */
#define INVALID_TIME_MASK (0x02)

#define ELOG_EVP_BLOB_NETWORK_STATUS (uint8_t)0xf0 /* evp_elog_code */
#define ELOG_EVP_MQTT_STATUS (uint8_t)0xf1         /* evp_elog_code2*/
#define ELOG_EVP_WASM_STATUS (uint8_t)0xf2         /* evp_elog_code3*/

static uint8_t g_mqtt_reconnect_cnt = 0;
static bool g_mqtt_sync_error_recv_buffer_too_small = false;
static bool g_mqtt_sync_error_send_buffer_is_full = false;
bool g_mqtt_sync_error_invalid_time = false;
bool g_tcp_timer_error_timeout = false;

/*
 * ELog Code Specification
 *
 * 1. evp_elog_code : BlobOperation or network status
 *
 *   ---------------------------------
 *   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *   ---------------------------------
 *     0   1                           BlobOperation success
 *             0   0   0   0             Success
 *     1   0                           BlobOperation fail by http
 *             0   0   0   1             200: OK
 *             0   0   1   0             400: Bad Request
 *             0   0   1   1             401: Unauthorized
 *             0   1   0   0             403: Forbidden
 *             0   1   0   1             404: Not Found
 *             0   1   1   0             500: Internal Server Error
 *             0   1   1   1             502: Bad Gateway
 *             1   0   0   0             503: Service Unavailable
 *             1   0   0   1             Other
 *     1   1                           BlobOperation fail by others
 *             0   0   0   1               1: EPERM
 *             0   0   1   0               5: EIO
 *             0   0   1   1               7: E2BIG
 *             0   1   0   0              12: ENOMEM
 *             0   1   0   1              22: EINVAL
 *             0   1   1   0              71: EPROTO
 *             0   1   1   1              92: ELOOP
 *             1   0   0   0             113: ECONNABORTED
 *             1   0   0   1             118: EHOSTUNREACH
 *             1   0   1   0             134: ENOTSUP
 *             1   0   1   1             Other
 *                             1       TCP close timeout
 *                                 1   BlobOperation ssl timeout
 *
 * 2. evp_elog_code2 :  MQTT Status
 *
 *   ---------------------------------
 *   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *   ---------------------------------
 *     x   x   x   x                   Reconnection count
 *                     1               Receiving buffer size is too small
 *                         1           Sending buffer is full
 *                             1       System time invalid
 *                                 1   System time forcely modified
 *
 * 3. evp_elog_code3 : Wasm error
 *   ---------------------------------
 *   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *   ---------------------------------
 *                             1       Wasm runtime error
 *                                 1   Failed to release VisonAppAgent resource
 *
 */

static int elog_handler_blob_success(void)
{
    int ret = 0;

    uint8_t code = SystemGetELog(ELOG_EVP_BLOB_NETWORK_STATUS);
    // Set to BlobOperation Success
    uint8_t tmp = 1 << 6;

    // Clear field of BlobOperation ssl timeout.
    code = (code & ~0x1);
    // Don't change "TCP close timeout" bit
    code = ((tmp & 0xFC) | (code & 0x3));

    ret = SystemSetELog(ELOG_EVP_BLOB_NETWORK_STATUS, code);
    if (ret < 0) {
        fprintf(stdout, "failed to handle mqtt result:%d\n", ret);
    }

    return 0;
}

/*
 * event is the http status code.
 */
static int elog_handler_blob_http_error(unsigned int status)
{
    int ret = 0;
    // Set to BlobOperation http fail
    uint8_t tmp = 2 << 6;
    uint8_t code = SystemGetELog(ELOG_EVP_BLOB_NETWORK_STATUS);

    if (code == ELOG_ERR) {
        return EINVAL;
    }

    // Set http status
    switch (status) {
        case 200:
            /* OK */
            tmp |= (1 << 2);
            break;
        case 400:
            /* Bad Request */
            tmp |= (2 << 2);
            break;
        case 401:
            /* Unauthorized */
            tmp |= (3 << 2);
            break;
        case 403:
            /* Forbidden */
            tmp |= (4 << 2);
            break;
        case 404:
            /* Not Found */
            tmp |= (5 << 2);
            break;
        case 500:
            /* Internal Server Error */
            tmp |= (6 << 2);
            break;
        case 502:
            /* Bad Gateway */
            tmp |= (7 << 2);
            break;
        case 503:
            /* Service Unavailable */
            tmp |= (8 << 2);
            break;
        default:
            /* Other */
            tmp |= (9 << 2);
            break;
    }

    code = ((tmp & 0xFC) | (code & 0x3));
    ret = SystemSetELog(ELOG_EVP_BLOB_NETWORK_STATUS, code);
    if (ret < 0) {
        fprintf(stderr, "Failed to set ELOG: %d\n", ret);
    }

    return ret;
}

/*
 * event is errno.
 */
static int elog_handler_blob_other_error(int error)
{
    // Set to BlobOperation non-http fail
    uint8_t tmp = 3 << 6;
    uint8_t blob_network_code = SystemGetELog(ELOG_EVP_BLOB_NETWORK_STATUS);

    if (blob_network_code == ELOG_ERR) {
        return EINVAL;
    }

    switch (error) {
        case EPERM:
            tmp |= (1 << 2);
            break;
        case EIO:
            tmp |= (2 << 2);
            break;
        case E2BIG:
            tmp |= (3 << 2);
            break;
        case ENOMEM:
            tmp |= (4 << 2);
            break;
        case EINVAL:
            tmp |= (5 << 2);
            break;
        case EPROTO:
            tmp |= (6 << 2);
            break;
        case ELOOP:
            tmp |= (7 << 2);
            break;
        case ECONNABORTED:
            tmp |= (8 << 2);
            break;
        case EHOSTUNREACH:
            tmp |= (9 << 2);
            break;
        case ENOTSUP:
            tmp |= (10 << 2);
            break;
        default:
            tmp |= (11 << 2);
            break;
    }

    // Don't change "TCP close timeout" bit
    blob_network_code = ((tmp & 0xFC) | (blob_network_code & 0x3));
    SystemSetELog(ELOG_EVP_BLOB_NETWORK_STATUS, blob_network_code);

    return 0;
}

static int elog_handler_blob_result(const void *event, void *user_data)
{
    const struct evp_agent_notification_blob_result *notif = event;

    if (notif) {
        switch (notif->result) {
            case EVP_BLOB_RESULT_SUCCESS:
                elog_handler_blob_success();
                break;
            case EVP_BLOB_RESULT_ERROR_HTTP:
                elog_handler_blob_http_error(notif->http_status);
                break;
            case EVP_BLOB_RESULT_ERROR:
            default:
                elog_handler_blob_other_error(notif->error);
                break;
        }
    }

    return 0;
}

/* Time may be invalid because of ntp failure */
static int check_sys_time(void)
{
    bool is_invalid_time = false;
    struct timespec tp;

    int rv = clock_gettime(CLOCK_REALTIME, &tp);
    if (rv != 0) {
        fprintf(stdout, "could not get time information");
        is_invalid_time = true;
    }
    else {
        struct tm ltime = {0};
        localtime_r(&tp.tv_sec, &ltime);
        if (ltime.tm_year + 1900 <= 2000) {
            fprintf(stdout, "year <= 2000. %04d/%02d/%02d %02d:%02d:%02d", ltime.tm_year + 1900,
                    ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
            is_invalid_time = true;
        }
    }

    if (is_invalid_time) {
        g_mqtt_sync_error_invalid_time = true;
    }

    return 0;
}

/*
 * Send all elog when MQTT is recovered.
 *
 */
static int elog_handler_mqtt_sync_success(const void *event, void *user_data)
{
    uint8_t mqtt_code = SystemGetELog(ELOG_EVP_MQTT_STATUS);
    if (mqtt_code == ELOG_ERR) {
        return -EINVAL;
    }

    uint8_t blob_network_code = SystemGetELog(ELOG_EVP_BLOB_NETWORK_STATUS);
    if (blob_network_code == ELOG_ERR) {
        return -EINVAL;
    }

    /* Reset the field of MQTT reconnect counter */
    mqtt_code &= ~MQTT_RECONNECT_CNT_MASK;
    mqtt_code |= ((g_mqtt_reconnect_cnt << MQTT_RECONNECT_CNT_SHIFT) & MQTT_RECONNECT_CNT_MASK);

    if (g_mqtt_sync_error_recv_buffer_too_small) {
        mqtt_code |= MQTT_ERR_RECV_BUFF_MASK;
    }
    if (g_mqtt_sync_error_send_buffer_is_full) {
        mqtt_code |= MQTT_ERR_SEND_BUFF_MASK;
    }
    if (g_mqtt_sync_error_invalid_time) {
        mqtt_code |= INVALID_TIME_MASK;
    }
    else {
        mqtt_code &= ~INVALID_TIME_MASK;
    }

    fprintf(stdout, "mqtt_code =0x%x", mqtt_code);

    /* set field of tcp_timer error flag */
    if (g_tcp_timer_error_timeout) {
        blob_network_code |= 0x2;
    }
    else {
        blob_network_code &= ~0x2;
    }

    /* clear internal counter & flags */
    g_mqtt_reconnect_cnt = 0;
    g_mqtt_sync_error_recv_buffer_too_small = false;
    g_mqtt_sync_error_send_buffer_is_full = false;
    g_mqtt_sync_error_invalid_time = false;
    g_tcp_timer_error_timeout = false;

    SystemSetELog(ELOG_EVP_MQTT_STATUS, mqtt_code);
    SystemSetELog(ELOG_EVP_BLOB_NETWORK_STATUS, blob_network_code);

    fprintf(stdout,
            "MQTT SYNC SUCCESS Update ELOG blob_network_code:%x "
            "mqtt_code:%x\n",
            blob_network_code, mqtt_code);

    return 0;
}

static int elog_handler_mqtt_sync_fail(const void *event, void *user_data)
{
    int ret = 0;
    uint8_t mqtt_code = SystemGetELog(ELOG_EVP_MQTT_STATUS);
    if (mqtt_code == ELOG_ERR) {
        fprintf(stderr, "Failed to get ELOG ELOG_EVP_MQTT_STATUS");
        return -EINVAL;
    }

    /* check NTP time if we are failed */
    check_sys_time();

    if (event) {
        const char *reason = event;

        const char *check = "MQTT_ERROR_RECV_BUFFER_TOO_SMALL";
        if (!strncmp(reason, check, strlen(check))) {
            g_mqtt_sync_error_recv_buffer_too_small = true;
        }

        check = "MQTT_ERROR_SEND_BUFFER_IS_FULL";
        if (!strncmp(reason, check, strlen(check))) {
            g_mqtt_sync_error_send_buffer_is_full = true;
        }
    }

    if (g_mqtt_reconnect_cnt == 0) {
        /* clear field of MQTT reconnect
		 * counter */
        mqtt_code &= ~MQTT_RECONNECT_CNT_MASK;

        /* clear field of MQTT error flags */
        if (g_mqtt_sync_error_recv_buffer_too_small) {
            mqtt_code &= ~MQTT_ERR_RECV_BUFF_MASK;
        }
        if (g_mqtt_sync_error_send_buffer_is_full) {
            mqtt_code &= ~MQTT_ERR_SEND_BUFF_MASK;
        }

        ret = SystemSetELog(ELOG_EVP_MQTT_STATUS, mqtt_code);
        if (ret < 0) {
            fprintf(stderr,
                    "Failed to set ELOG ELOG_EVP_MQTT_STATUS: "
                    "%d\n",
                    ret);
        }
    }

    /* only 4 bit is for g_mqtt_reconnect_cnt in evp_elog_code2 */
    if (g_mqtt_reconnect_cnt < MQTT_RECONNECT_CNT_MAX) {
        /* update internal counter */
        g_mqtt_reconnect_cnt++;
    }

    return ret;
}

/*
 * This notification is called only there is a status change happened in MQTT.
 * The event is the string returned by mqtt_error_str() such as
 * "MQTT_ERROR_RECONNECTING".
 *
 * That means:
 * if MQTT_OK is received, it is the first time connection/reconnection
 * succeeds.
 * if an error like MQTT_ERROR_RECONNECTING received, it is the first
 * time such error happens.
 *
 */

static int elog_handler_mqtt_sync_result(const void *event, void *user_data)
{
    int ret = 0;
    const char *reason = event;

    const char *check = "MQTT_OK";
    if (!strncmp(reason, check, strlen(check))) {
        ret = elog_handler_mqtt_sync_success(event, user_data);
    }
    else {
        ret = elog_handler_mqtt_sync_fail(event, user_data);
    }

    if (ret < 0) {
        fprintf(stdout, "failed to handle mqtt result:%d\n", ret);
    }

    /* Only for elog issue, no need to return failure to wedge-agent */
    return 0;
}

static int elog_handler_network_error(const void *event, void *user_data)
{
    int ret = 0;
    uint8_t blob_network_code = SystemGetELog(ELOG_EVP_BLOB_NETWORK_STATUS);
    if (blob_network_code == ELOG_ERR) {
        return -EINVAL;
    }

    const char *reason = event;
    const char *check = "ssl_timeout";

    if (!strncmp(reason, check, strlen(check))) {
        blob_network_code |= 0x01;
    }

    ret = SystemSetELog(ELOG_EVP_BLOB_NETWORK_STATUS, blob_network_code);
    if (ret < 0) {
        fprintf(stdout, "failed to handle mqtt result:%d\n", ret);
    }

    return 0;
}

/* === Agent Status Handler Implementation === */

#define PROVISIONING_SERVICE_URL "provision.aitrios.sony-semicon.com"

typedef enum {
    EvpAgentLedIndexConnectedWithTLS,
    EvpAgentLedIndexConnectedWithoutTLS,
    EvpAgentLedIndexDisconnectedConnectingWithTLS,
    EvpAgentLedIndexDisconnectedConnectingWithoutTLS,
    EvpAgentLedIndexMax
} EvpAgentLedIndex;

static bool is_tls_connection(void)
{
    return evp_agent_esf_is_tls_enabled();
}

static bool check_provisioning_service_mqtt_host(void)
{
    bool is_provisioning_service = false;
    struct config *cfg;

    cfg = evp_agent_esf_read_config(EVP_CONFIG_MQTT_HOST);
    if (cfg != NULL) {
        if (strcmp(cfg->value, PROVISIONING_SERVICE_URL) == 0) {
            is_provisioning_service = true;
        }
        cfg->free(cfg);
    }

    return is_provisioning_service;
}

static bool check_project_id_and_register_token_valid(void)
{
    bool is_valid = false;
    char *project_id = NULL;
    char *register_token = NULL;
    size_t size;
    EsfSystemManagerResult sys_mgr_ret;

    size = ESF_SYSTEM_MANAGER_PROJECT_ID_MAX_SIZE;
    project_id = malloc(size);
    if (project_id == NULL) {
        SystemDlog(LOG_ERR, "agent_status", __FILE__, __LINE__,
                   "failed to allocate memory for project_id");
        goto end;
    }
    sys_mgr_ret = EsfSystemManagerGetProjectId(project_id, &size);
    if (sys_mgr_ret != kEsfSystemManagerResultOk) {
        SystemDlog(LOG_ERR, "agent_status", __FILE__, __LINE__,
                   "EsfSystemManagerGetProjectId failed (%d)", (int)sys_mgr_ret);
        goto end;
    }

    size = ESF_SYSTEM_MANAGER_REGISTER_TOKEN_MAX_SIZE;
    register_token = malloc(size);
    if (register_token == NULL) {
        SystemDlog(LOG_ERR, "agent_status", __FILE__, __LINE__,
                   "failed to allocate memory for register_token");
        goto end;
    }
    sys_mgr_ret = EsfSystemManagerGetRegisterToken(register_token, &size);
    if (sys_mgr_ret != kEsfSystemManagerResultOk) {
        SystemDlog(LOG_ERR, "agent_status", __FILE__, __LINE__,
                   "EsfSystemManagerGetRegisterToken failed (%d)", (int)sys_mgr_ret);
        goto end;
    }

    if (project_id[0] != '\0' && register_token[0] != '\0') {
        is_valid = true;
    }

end:
    free(project_id);
    free(register_token);

    return is_valid;
}

static bool is_enrollment_mode(void)
{
    if (check_provisioning_service_mqtt_host()) {
        /*
		 * connecting to ProvisioningService
		 * -> enrollment mode
		 */
        return true;
    }

    if (check_project_id_and_register_token_valid()) {
        /*
		 * Both of ProjectId and RegisterToken are set
		 * -> enrollment mode
		 */
        return true;
    }

    /*
	 * Not enrollment mode (connecting to console)
	 */
    return false;
}

/*
 * Handler for agent connection status changes.
 * Manages LED status based on agent connection state and TLS configuration.
 * args: "connected" or "disconnected"
 */
int agent_status_handler(const void *args, void *user_data)
{
    if (args == NULL) {
        SystemDlog(LOG_ERR, "agent_status", __FILE__, __LINE__, "%s: args is NULL", __func__);
        return -1;
    }

    const char *agent_status = (const char *)args;
    EsfLedManagerResult led_mgr_ret;
    EsfLedManagerLedStatusInfo led_status[EvpAgentLedIndexMax];

    for (int i = 0; i < EvpAgentLedIndexMax; i++) {
        led_status[i].led = kEsfLedManagerTargetLedService;

        switch (i) {
            case EvpAgentLedIndexConnectedWithTLS:
                led_status[i].status = kEsfLedManagerLedStatusConnectedWithTLS;
                break;
            case EvpAgentLedIndexConnectedWithoutTLS:
                led_status[i].status = kEsfLedManagerLedStatusConnectedWithoutTLS;
                break;
            case EvpAgentLedIndexDisconnectedConnectingWithTLS:
                led_status[i].status = kEsfLedManagerLedStatusDisconnectedConnectingWithTLS;
                break;
            case EvpAgentLedIndexDisconnectedConnectingWithoutTLS:
                led_status[i].status = kEsfLedManagerLedStatusDisconnectedConnectingWithoutTLS;
                break;
        }
        led_status[i].enabled = false;
    }

    if (strcmp(agent_status, "connected") == 0) {
        if (is_enrollment_mode()) {
            /*
			 * Do nothing "connected" in enrollment mode
			 */
            return 0;
        }

        if (is_tls_connection()) {
            led_status[EvpAgentLedIndexConnectedWithTLS].enabled = true;
        }
        else {
            led_status[EvpAgentLedIndexConnectedWithoutTLS].enabled = true;
        }
    }
    else if (strcmp(agent_status, "disconnected") == 0) {
        if (is_tls_connection()) {
            led_status[EvpAgentLedIndexDisconnectedConnectingWithTLS].enabled = true;
        }
        else {
            led_status[EvpAgentLedIndexDisconnectedConnectingWithoutTLS].enabled = true;
        }
    }
    else {
        SystemDlog(LOG_ERR, "agent_status", __FILE__, __LINE__, "%s: unknown agent/status: %s",
                   __func__, agent_status);
        return -1;
    }

    for (int i = 0; i < EvpAgentLedIndexMax; i++) {
        led_mgr_ret = EsfLedManagerSetStatus(&led_status[i]);
        if (led_mgr_ret != kEsfLedManagerSuccess) {
            SystemDlog(LOG_ERR, "agent_status", __FILE__, __LINE__,
                       "%s: EsfLedManagerSetStatus failed, result=%u "
                       "status=%u",
                       __func__, led_mgr_ret, led_status[i].status);
            return -1;
        }
    }

    return 0;
}

int evp_agent_notifications_register(struct evp_agent_context *ctxt)
{
    int ret = 0;

    ret = evp_agent_notification_subscribe(ctxt, "blob/result", elog_handler_blob_result, NULL);
    if (ret) {
        fprintf(stderr, "%s(): Failed to subscribe to blob/result\n", __func__);
        return ret;
    }

    ret = evp_agent_notification_subscribe(ctxt, "mqtt/sync/err", elog_handler_mqtt_sync_result,
                                           NULL);
    if (ret) {
        fprintf(stderr, "%s(): Failed to subscribe to mqtt/sync/err\n", __func__);
        return ret;
    }

    ret = evp_agent_notification_subscribe(ctxt, "network/error", elog_handler_network_error, NULL);
    if (ret) {
        fprintf(stderr, "%s(): Failed to subscribe to network/error\n", __func__);
        return ret;
    }

    ret = evp_agent_notification_subscribe(ctxt, "agent/status", agent_status_handler, NULL);
    if (ret) {
        fprintf(stderr, "%s(): Failed to subscribe to agent/status\n", __func__);
        return ret;
    }

    return 0;
}
