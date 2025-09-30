/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _SSF_ELOG_H__
#define _SSF_ELOG_H__

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>

#define MAX_LOG_STR_LEN 4096
#define MIN_LOG_STR_LEN 128

#define ELOG_ERR 0xFF

#define EVP_FILE_NAME(file) (strrchr(file, '/') ? strrchr(file, '/') + 1 : file)

/*
 * Priority defined in syslog.h
 *
 *LOG_EMERG     :  System is unusable
 *LOG_ALERT     :  Action must be taken immediately
 *LOG_CRIT      :  Critical conditions
 *LOG_ERR       :  Error conditions
 *LOG_WARNING   :  Warning conditions
 *LOG_NOTICE    :  Normal, but significant, condition
 *LOG_INFO      :  Informational message
 *LOG_DEBUG     :  Debug-level message
 */

void SystemDlog(int priority, const char *tag, const char *file, int line, const char *fmt, ...);

void evp_agent_dlog_handler(int lvl, const char *file, int line, const char *fmt, va_list ap,
                            void *user);

int SystemRegElog(uint8_t component, uint8_t init_value, const char *msg);
int SystemSetELog(uint8_t component, uint8_t code);
uint8_t SystemGetELog(uint8_t component);
void SystemSendElog(void);

#endif /* _SSF_ELOG_H__ */
