/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <config.h>
#include <errno.h>
#include <bsd/sys/queue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wasm_export.h>

#include <evp/agent.h>
#if defined(CONFIG_EVP_MODULE_IMPL_WASM)
extern void wasm_add_native_lib(const char *);
#endif

/* ESF Headers */
#include "memory_manager.h"
#include "power_manager.h"

/* Local Headers */
#include "esf.h"
#include "notifications.h"
#include "sdk_backdoor.h"

// Define CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_1 if it is not defined yet for Raspberry Pi
#ifndef CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_1
#define CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_1 (1)
#endif

#define EVP_SW_WDT_ID CONFIG_EXTERNAL_POWER_MANAGER_SW_WDT_ID_1
#define LIB_SENSCORD_WAMR_SO "/opt/senscord/lib/libsenscord_wamr.so"
static void evp_add_wasm_native_library(const char *fname);

struct SYS_client;

static struct {
	struct evp_agent_context *ctxt;
	enum {
		EVP_AGENT_LOOP,
		EVP_AGENT_UNDEPLOY_ALL,
	} cmd;
	volatile sig_atomic_t signalled;
	pthread_mutex_t lock;
	pthread_t thread;
	bool started;
	int ret;
} g_evp_agent = {
	.ctxt = NULL,
	.cmd  = EVP_AGENT_LOOP,
	.signalled = 0,
	.lock = PTHREAD_MUTEX_INITIALIZER,
	.started = false,
	.ret = 0,
};

struct native_symbol_entry {
	TAILQ_ENTRY(native_symbol_entry) q;
	const char *module_name;
	NativeSymbol *native_symbols;
	uint32_t n_native_symbols;
};

TAILQ_HEAD(native_symbol_entry_head, native_symbol_entry);

static struct {
	struct native_symbol_entry_head queue;
	pthread_mutex_t lock;
} g_wasm_native_symbols = {
	.queue = TAILQ_HEAD_INITIALIZER(g_wasm_native_symbols.queue),
	.lock = PTHREAD_MUTEX_INITIALIZER
};

static struct evp_agent_context *get_backdoor_context()
{
	struct evp_agent_context *ctxt;

	pthread_mutex_lock(&g_evp_agent.lock);
	ctxt = g_evp_agent.ctxt;
	pthread_mutex_unlock(&g_evp_agent.lock);

	return ctxt;
}

static void set_backdoor_context(struct evp_agent_context *ctxt)
{
	pthread_mutex_lock(&g_evp_agent.lock);
	g_evp_agent.ctxt = ctxt;
	pthread_mutex_unlock(&g_evp_agent.lock);
}

enum evp_agent_status EVP_getAgentStatus()
{
	struct evp_agent_context *ctxt = get_backdoor_context();
	if (ctxt == NULL) {
		return EVP_AGENT_STATUS_INIT;
	}
	return evp_agent_get_status(ctxt);
}

struct SYS_client *EVP_Agent_register_sys_client()
{
	struct evp_agent_context *ctxt = get_backdoor_context();

	if (ctxt == NULL) {
		return NULL;
	}

	return evp_agent_register_sys_client(ctxt);
}

int EVP_Agent_unregister_sys_client(struct SYS_client *c)
{
	struct evp_agent_context *ctxt = get_backdoor_context();

	if (ctxt == NULL) {
		return -1;
	}

	return evp_agent_unregister_sys_client(ctxt, c);
}

int EVP_undeployModules()
{
	enum evp_agent_status status = EVP_getAgentStatus();
	if (status == EVP_AGENT_STATUS_INIT) {
		return EVP_AGAIN;
	}

	struct evp_agent_context *ctxt = get_backdoor_context();
	evp_agent_undeploy_all(ctxt);
	return evp_agent_empty_deployment_has_completed(ctxt);
}

static int evp_agent_wasm_native_symbol_add(const char *module_name,
					    NativeSymbol *native_symbols,
					    uint32_t n_native_symbols)
{
	if (module_name == NULL) {
		fprintf(stderr, "module_name is NULL");
		return -EINVAL;
	}
	if (native_symbols == NULL) {
		fprintf(stderr, "native_symbols is NULL");
		return -EINVAL;
	}
	if (n_native_symbols == 0) {
		fprintf(stderr, "n_native_symbols is 0");
		return -EINVAL;
	}

	struct native_symbol_entry *entry =
		malloc(sizeof(struct native_symbol_entry));
	if (entry == NULL) {
		fprintf(stderr,
			"failed to allocate memory for native_symbol_entry");
		return -ENOMEM;
	}

	entry->module_name = module_name;
	entry->native_symbols = native_symbols;
	entry->n_native_symbols = n_native_symbols;

	pthread_mutex_lock(&g_wasm_native_symbols.lock);
	TAILQ_INSERT_TAIL(&g_wasm_native_symbols.queue, entry, q);
	pthread_mutex_unlock(&g_wasm_native_symbols.lock);

	return 0;
}

bool EVP_wasm_runtime_register_natives(const char *module_name,
				       NativeSymbol *native_symbols,
				       uint32_t n_native_symbols)
{
	int ret;

	if (g_evp_agent.started) {
		fprintf(stderr, "EVP Agent has already started\n");
		return false;
	}

	ret = evp_agent_wasm_native_symbol_add(module_name, native_symbols,
					       n_native_symbols);
	if (ret)
		return false;

	return true;
}

static int evp_agent_flush_wasm_native_symbols()
{
	struct native_symbol_entry *entry, *tmp;
	int ret = 0;

	TAILQ_FOREACH_SAFE(entry, &g_wasm_native_symbols.queue, q, tmp) {
		if (!ret) {
			if (!wasm_runtime_register_natives(entry->module_name,
							   entry->native_symbols,
							   entry->n_native_symbols)) {
				fprintf(stderr, "Failed to register native symbols\n");
				ret = -1;
			}
		}

		TAILQ_REMOVE(&g_wasm_native_symbols.queue, entry, q);
		free(entry);
	}

	return ret;
}

static void *evp_agent_thread(void *data)
{
	int ret;
	EsfPwrMgrError wdt_err;

	g_evp_agent.started = true;

	ret = pthread_setname_np(pthread_self(), "EVP Agent");
	if (ret) {
		fprintf(stderr, "%s(): failed to set threadname\n", __func__);
		g_evp_agent.ret = ret;
		pthread_exit(&g_evp_agent.ret);
	}

	/*
	 * No error check because the library guarantees either success or a
	 * call to exit()
	 */
	struct evp_agent_context *ctxt = evp_agent_setup("evp_agent");

	ret = agent_status_handler("disconnected", NULL);
	if (ret)
		goto out_free_evp_agent;

	ret = evp_agent_esf_init_proxy_cache();
	if (ret)
		goto out_free_evp_agent;

	ret = evp_agent_notifications_register(ctxt);
	if (ret)
		goto out_deinit_proxy_cache;

	ret = evp_agent_start(ctxt);
	if (ret)
		goto out_deinit_proxy_cache;

	ret = evp_agent_flush_wasm_native_symbols();
	if (ret)
		goto out_stop_evp_agent;

	ret = evp_agent_connect(ctxt);
	if (ret)
		goto out_stop_evp_agent;

	wdt_err = EsfPwrMgrSwWdtStart(EVP_SW_WDT_ID);
	if (wdt_err != kEsfPwrMgrOk)
		goto out_disconnect_evp_agent;

	set_backdoor_context(ctxt);

	while (ret == 0) {
		wdt_err = EsfPwrMgrSwWdtKeepalive(EVP_SW_WDT_ID);
		if (wdt_err != kEsfPwrMgrOk) {
			fprintf(stderr, "%s(): EsfPwrMgrSwWdtKeepalive failed: %d\n", __func__, wdt_err);
		}
		ret = evp_agent_loop(ctxt);
		if (g_evp_agent.signalled) {
			break;
		}
	}

	wdt_err = EsfPwrMgrSwWdtStop(EVP_SW_WDT_ID);
	if (wdt_err != kEsfPwrMgrOk) {
		fprintf(stderr, "%s(): EsfPwrMgrSwWdtStop failed: %d\n", __func__, wdt_err);
	}

out_disconnect_evp_agent:
	evp_agent_disconnect(ctxt);
out_stop_evp_agent:
	evp_agent_stop(ctxt);
out_deinit_proxy_cache:
	evp_agent_esf_deinit_proxy_cache();
out_free_evp_agent:
	evp_agent_free(ctxt);

	g_evp_agent.ret = ret;
	pthread_exit(&g_evp_agent.ret);
	return NULL;
}

void evp_agent_shutdown()
{
	g_evp_agent.signalled = true;
	pthread_join(g_evp_agent.thread, NULL);
}

static void
evp_add_wasm_native_library(const char *fname)
{
#if !defined(CONFIG_EVP_MODULE_IMPL_WASM)
	// Exit (xlog_abort): runtime error
	fprintf(stderr, "command line dynamic libraries is only enabled with wasm "
			"module implementation");
#else
	wasm_add_native_lib(fname);
#endif
}

int evp_agent_startup()
{
	int ret;

	ret = pthread_create(&g_evp_agent.thread, NULL, evp_agent_thread, NULL);
	if (ret)
		return ret;

	ret = pthread_setname_np(g_evp_agent.thread, "evp-agent");
	if (ret)
		goto err_join_thread;

	evp_add_wasm_native_library(LIB_SENSCORD_WAMR_SO);

	return 0;

err_join_thread:
	g_evp_agent.signalled = true;
	pthread_join(g_evp_agent.thread, NULL);

	return ret;
}
