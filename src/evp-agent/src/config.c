/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <evp/agent_config.h>

#include <mbedtls/platform_util.h>

#include "esf.h"

int load_config_impl(struct config *config, void **vpp, size_t *sizep)
{
    *vpp = config->value;
    *sizep = config->size;
    return 0;
}

void unload_config_impl(struct config *config, void *vp0, size_t size)
{
    mbedtls_platform_zeroize(vp0, size);
}

struct config *get_config_impl(enum config_key key)
{
    /* When the user set Insecure Mode, ignore TLS configurations */
    if (((key == EVP_CONFIG_MQTT_TLS_CLIENT_KEY) || (key == EVP_CONFIG_MQTT_TLS_CLIENT_CERT) ||
         (key == EVP_CONFIG_MQTT_TLS_CA_CERT)) &&
        !evp_agent_esf_is_tls_enabled()) {
        return NULL;
    }

    return evp_agent_esf_read_config(key);
}

bool config_is_pk_file(enum config_key key)
{
    return key == EVP_CONFIG_PK_FILE;
}
