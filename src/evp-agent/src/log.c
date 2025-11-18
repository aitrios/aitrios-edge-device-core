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
#define LOG_BUFFER_SIZE (4096)
#define LOG_CHUNK_SIZE (256)

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

    UtilityLogWriteDLog(MODULE_ID_SYSTEM, log_level, "[%s] %s-%d: %s\n", log_level_str, file, line,
                        logstr);

    free(max_logstr);
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

    // Calculate required buffer size
    const char *filename = EVP_FILE_NAME(file);
    int prefix_len = snprintf(NULL, 0, "[%s:%d] ", filename, line);

    va_list ap_copy;
    va_copy(ap_copy, ap);
    int message_len = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);

    // Check for snprintf errors
    if (prefix_len < 0 || message_len < 0) {
        EVP_AGENT_ERR("Log formatting error (vsnprintf failed)");
        return;
    }

    int total_len = prefix_len + message_len + 1; // +1 for null terminator
    char *buf = NULL;
    int buffer_size = total_len;
    bool log_truncated = false;

    if (total_len > LOG_BUFFER_SIZE) {
        buffer_size = LOG_BUFFER_SIZE;
        log_truncated = true;
    }

    buf = malloc(buffer_size);

    // check for malloc failure
    if (buf == NULL) {
        EVP_AGENT_ERR("Memory allocation failed");
        return;
    }

    // Create Log message
    if (vsnprintf(buf, buffer_size, fmt, ap) < 0) {
        EVP_AGENT_ERR("Log formatting error (vsnprintf failed)");
        free(buf);
        return;
    }

    int buf_len = strlen(buf);

    // Output log in chunks
    for (int i = 0; i < buf_len; i += LOG_CHUNK_SIZE) {
        int remaining = buf_len - i;
        int chunk_len = (remaining > LOG_CHUNK_SIZE) ? LOG_CHUNK_SIZE : remaining;

        UtilityLogWriteDLog(MODULE_ID_SYSTEM, log_level, "[%s:%d] %.*s", filename, line, chunk_len,
                            buf + i);
    }

    // Send DLOG message if log was truncated due to buffer size limit
    if (log_truncated) {
        UtilityLogWriteDLog(MODULE_ID_SYSTEM, log_level,
                            "[%s:%d] Log message truncated: message size %d bytes, buffer size "
                            "%d bytes, truncated %d bytes",
                            filename, line, total_len, LOG_BUFFER_SIZE,
                            total_len - LOG_BUFFER_SIZE);
    }

    free(buf);
}
