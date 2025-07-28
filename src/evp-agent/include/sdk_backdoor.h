/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#if !defined(__SDK_BACKDOOR_H__)
#define __SDK_BACKDOOR_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <wasm_export.h>

/* EVP_RESULT */
#include "evp/sdk.h"

/* evp_agent_status */
#include "evp/agent.h"
#include "evp/sdk_sys.h"

extern const char *sdk_backdoor_prefix;

/** @file */

struct SYS_client *EVP_Agent_register_sys_client(void);

int EVP_Agent_unregister_sys_client(struct SYS_client *c);

/** @brief Get the agent status.
 *
 * \verbatim embed:rst:leading-asterisk
 * .. warning::
 *      This function is provided as a temporary solution for a particular
 *      use case.
 *      The functionality is available only for particular builds of
 *      the software.
 *      The functionality might be removed from the future versions
 *      of the SDK.
 * \endverbatim
 *
 * When the connection mode is "connected", the status returned will be
 * one of:
 * EVP_AGENT_STATUS_READY
 * EVP_AGENT_STATUS_CONNECTING
 * EVP_AGENT_STATUS_CONNECTED
 *
 * When the connection mode is "disconnected", the status returned will be
 * one of:
 * EVP_AGENT_STATUS_DISCONNECTING
 * EVP_AGENT_STATUS_DISCONNECTED
 *
 * @returns evp_agent status
 */
enum evp_agent_status EVP_getAgentStatus(void);

/** @brief Undeploy all modules
 *
 * Undeploy all modules by replacing the current deployment with an empty one.
 * EVP_undeployModules() may be called from a separate NuttX task from
 * evp_agent_loop().
 *
 * @returns 1 if the applied deployment is in fact empty, zero otherwise.
 */
int EVP_undeployModules(void);

/** @brief wrapper for wasm_runtime_register_natives
 */
bool EVP_wasm_runtime_register_natives(const char *module_name,
				       NativeSymbol *native_symbols,
				       uint32_t n_native_symbols);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* !defined(__SDK_BACKDOOR_H__) */
