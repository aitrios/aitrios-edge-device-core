/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __EVP_ESF_H__
#define __EVP_ESF_H__

enum config_key;
struct config;

bool evp_agent_esf_is_tls_enabled(void);
void evp_agent_esf_deinit_proxy_cache(void);
int evp_agent_esf_init_proxy_cache(void);
struct config *evp_agent_esf_read_config(enum config_key key);

#endif /* __EVP_ESF_H__ */