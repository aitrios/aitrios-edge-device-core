/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "log.h"

#include "utility_log.h"
#include "utility_log_module_id.h"

#define DLOG_UART_FORMAT(letter) \
    LOG_COLOR_##letter #letter " (%d) %s-%s-%d: %s" LOG_RESET_COLOR "\n"

#define MAX_ERROR_COUNT (10)
static uint8_t err_count = 0;

struct elog_entry {
    uint8_t component;
    uint8_t code;
    bool updated;
    const char *msg;
};
#define MAX_ELOG_ENTRY 128
struct elog_entry elog_array[MAX_ELOG_ENTRY];
static int last = 0;
pthread_mutex_t elog_entry_mutex = PTHREAD_MUTEX_INITIALIZER;

#define ELOG_ARRAY_SIZE (last >= MAX_ELOG_ENTRY ? MAX_ELOG_ENTRY : last)

static struct elog_entry *component_to_entry(uint8_t component)
{
    int i;
    struct elog_entry *result = NULL;

    for (i = 0; i < ELOG_ARRAY_SIZE; i++) {
        if (elog_array[i].component == component) {
            result = &elog_array[i];
            break;
        }
    }

    return result;
}

static void elog_lock(void)
{
    assert(pthread_mutex_lock(&elog_entry_mutex) == 0);
}

static void elog_unlock(void)
{
    assert(pthread_mutex_unlock(&elog_entry_mutex) == 0);
}

void SystemDlog(int priority, const char *tag, const char *file, int line, const char *fmt, ...)
{
    va_list list;
    char min_logstr[MIN_LOG_STR_LEN];
    char *logstr = min_logstr;
    size_t log_len = MIN_LOG_STR_LEN;

    char *max_logstr = malloc(MAX_LOG_STR_LEN);
    if (max_logstr) {
        logstr = max_logstr;
        log_len = MAX_LOG_STR_LEN;
    }

    va_start(list, fmt);
    vsnprintf(logstr, log_len, fmt, list);
    va_end(list);

    UtilityLogDlogLevel log_level;
    const char *log_level_str = NULL;

    switch (priority) {
        case LOG_ERR:
            log_level_str = "ERROR";
            log_level = kUtilityLogDlogLevelError;
            break;
        case LOG_WARNING:
            log_level_str = "WARNING";
            log_level = kUtilityLogDlogLevelWarn;
            break;
        case LOG_INFO:
            log_level_str = "INFO";
            log_level = kUtilityLogDlogLevelInfo;
            break;
        case LOG_DEBUG:
            log_level_str = "DEBUG";
            log_level = kUtilityLogDlogLevelDebug;
            break;
        default:
            log_level_str = "CRITICAL";
            log_level = kUtilityLogDlogLevelCritical;
            break;
    }

    if (UtilityLogWriteDLog(MODULE_ID_SYSTEM, log_level, "[%s] %s-%d: %s\n", log_level_str, file,
                            line, logstr) != kUtilityLogStatusOk) {
        if (err_count < MAX_ERROR_COUNT) {
            err_count++;
            fprintf(stderr, "[%s] %s-%d: %s\n", "ERROR", file, line, logstr);
        }
    }

    if (max_logstr) {
        free(max_logstr);
    }
}

int SystemRegElog(uint8_t component, uint8_t init_value, const char *msg)
{
    int ret = 0;

    elog_lock();
    if (last < MAX_ELOG_ENTRY) {
        elog_array[last].component = component;
        elog_array[last].updated = false;
        elog_array[last].code = init_value;
        elog_array[last].msg = msg;
        last++;
    }
    else {
        ret = -ENOMEM;
    }
    elog_unlock();

    return ret;
}

int SystemSetELog(uint8_t component, uint8_t code)
{
    int ret = 0;

    elog_lock();

    struct elog_entry *entry = component_to_entry(component);
    if (entry) {
        if (entry->code != code) {
            entry->code = code;
            entry->updated = true;

            uint16_t event_id = (((uint16_t)component << 8) | (uint16_t)code);
            if (UtilityLogWriteELog(MODULE_ID_SYSTEM, kUtilityLogElogLevelWarn, event_id) !=
                kUtilityLogStatusOk) {
                ret = -ENOENT;
            }

            entry->updated = false;
        }
    }
    else {
        ret = -ENOENT;
    }
    elog_unlock();

    return ret;
}

uint8_t SystemGetELog(uint8_t component)
{
    uint8_t code = ELOG_ERR;

    elog_lock();
    struct elog_entry *entry = component_to_entry(component);

    if (entry) {
        code = entry->code;
    }
    elog_unlock();

    return code;
}

void SystemSendElog(void)
{
    int i;

    elog_lock();
    for (i = 0; i < ELOG_ARRAY_SIZE; i++) {
        struct elog_entry *entry = &elog_array[i];

        if (entry->updated) {
            SystemDlog(LOG_DEBUG, "ELOG", __func__, __LINE__,
                       "component: %x, code: %x  updated: %s, "
                       "msg:%s \n",
                       entry->component, entry->code, entry->updated ? "true" : "false",
                       entry->msg);

            entry->updated = false;
        }
    }
    elog_unlock();
}

void evp_agent_dlog_handler(int lvl, const char *file, int line, const char *fmt, va_list ap,
                            void *user)
{
    UtilityLogDlogLevel log_level;

    // Convert EVP log level to UtilityLog level
    switch (lvl) {
        case 0: // TRACE
            log_level = kUtilityLogDlogLevelTrace;
            break;
        case 1: // DEBUG
            log_level = kUtilityLogDlogLevelDebug;
            break;
        case 2: // INFO
            log_level = kUtilityLogDlogLevelInfo;
            break;
        case 3: // WARNING
            log_level = kUtilityLogDlogLevelWarn;
            break;
        case 4: // ERROR
            log_level = kUtilityLogDlogLevelError;
            break;
        default: // FATAL (5 or higher)
            log_level = kUtilityLogDlogLevelCritical;
            break;
    }

    // Create log message with file and line information
    char buf[256]; // Sufficient for normal filename + line + message
    const char *filename = EVP_FILE_NAME(file);
    int prefix_len = snprintf(buf, sizeof(buf), "[%s:%d] ", filename, line);

    // Add the actual log message
    vsnprintf(buf + prefix_len, sizeof(buf) - prefix_len, fmt, ap);

    // Output log using UtilityLogWriteDLog
    UtilityLogWriteDLog(MODULE_ID_SYSTEM, log_level, "%s", buf);
}
