/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#define _GNU_SOURCE /* for asprintf */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bsd/sys/cdefs.h>

#include <evp/agent_config.h>

#include "network_manager.h"
#include "network_manager/network_manager_accessor_parameter_storage_manager.h"
#include "network_manager/network_manager_resource.h"
#include "parameter_storage_manager.h"
#include "system_manager.h"

#include "esf.h"

#define MQTT_TLS_CLIENT_CERT_MAX_SIZE 32768
#define MQTT_TLS_CLIENT_KEY_MAX_SIZE 4096

#define MQTT_TLS_CLIENT_CERT_KEY_DEFAULT_DIR "/etc/evp"

static const size_t g_max_sizes[] = {
    [EVP_CONFIG_TLS_CA_CERT] = ESF_SYSTEM_MANAGER_ROOT_CA_MAX_SIZE,
    [EVP_CONFIG_MQTT_HOST] = ESF_SYSTEM_MANAGER_EVP_HUB_URL_MAX_SIZE,
    [EVP_CONFIG_MQTT_PORT] = ESF_SYSTEM_MANAGER_EVP_HUB_PORT_MAX_SIZE,
    [EVP_CONFIG_MQTT_TLS_CA_CERT] = ESF_SYSTEM_MANAGER_ROOT_CA_MAX_SIZE,
    [EVP_CONFIG_MQTT_TLS_CLIENT_CERT] = MQTT_TLS_CLIENT_CERT_MAX_SIZE,
    [EVP_CONFIG_MQTT_TLS_CLIENT_KEY] = MQTT_TLS_CLIENT_KEY_MAX_SIZE,
    [EVP_CONFIG_HTTPS_CA_CERT] = ESF_SYSTEM_MANAGER_ROOT_CA_MAX_SIZE,
    [EVP_CONFIG_MQTT_PROXY_HOST] = ESF_NETWORK_MANAGER_PROXY_URL_LEN,
    [EVP_CONFIG_MQTT_PROXY_PORT] = ESF_NETWORK_MANAGER_PROXY_PORT_LEN,
    [EVP_CONFIG_MQTT_PROXY_USERNAME] = ESF_NETWORK_MANAGER_PROXY_USER_NAME_LEN,
    [EVP_CONFIG_MQTT_PROXY_PASSWORD] = ESF_NETWORK_MANAGER_PROXY_PASSWORD_LEN,
    [EVP_CONFIG_HTTP_PROXY_HOST] = ESF_NETWORK_MANAGER_PROXY_URL_LEN,
    [EVP_CONFIG_HTTP_PROXY_PORT] = ESF_NETWORK_MANAGER_PROXY_PORT_LEN,
    [EVP_CONFIG_HTTP_PROXY_USERNAME] = ESF_NETWORK_MANAGER_PROXY_USER_NAME_LEN,
    [EVP_CONFIG_HTTP_PROXY_PASSWORD] = ESF_NETWORK_MANAGER_PROXY_PASSWORD_LEN,
    [EVP_CONFIG_IOT_PLATFORM] = ESF_SYSTEM_MANAGER_IOT_PLATFORM_MAX_SIZE,
};

static struct proxy_cache {
    char *host;
    char port[ESF_NETWORK_MANAGER_PROXY_PORT_LEN];
    char *username;
    char *password;
} g_proxy_cache;

bool evp_agent_esf_is_tls_enabled(void)
{
    EsfSystemManagerResult result;
    EsfSystemManagerEvpTlsValue TlsData;

    result = EsfSystemManagerGetEvpTls(&TlsData);
    if (result != kEsfSystemManagerResultOk) {
        fprintf(stderr, "EsfSystemManagerGetEvpTls() failed\n");
        return true;
    }

    if (TlsData == kEsfSystemManagerEvpTlsDisable)
        return false;

    return true;
}

void evp_agent_esf_deinit_proxy_cache(void)
{
    if (g_proxy_cache.host != NULL) {
        free(g_proxy_cache.host);
        g_proxy_cache.host = NULL;
    }
    if (g_proxy_cache.username != NULL) {
        free(g_proxy_cache.username);
        g_proxy_cache.username = NULL;
    }
    if (g_proxy_cache.password != NULL) {
        free(g_proxy_cache.password);
        g_proxy_cache.password = NULL;
    }
}

int evp_agent_esf_init_proxy_cache(void)
{
    int ret;

    EsfNetworkManagerParameter *prm = NULL;
    EsfNetworkManagerParameterMask mask;

    /*
	 * EsfNetworkManagerParameter size is about 1KB.
	 * use malloc for reducing stack consumption.
	 */
    prm = malloc(sizeof(*prm));
    if (prm == NULL) {
        fprintf(stderr, "failed to allocate memory for EsfNetworkManagerParameter");
        ret = -ENOMEM;
        goto end;
    }

    /* Load proxy settings */
    ESF_NETWORK_MANAGER_MASK_DISABLE_ALL(&mask);
    mask.proxy.url = 1;
    mask.proxy.port = 1;
    mask.proxy.username = 1;
    mask.proxy.password = 1;
    ret = EsfNetworkManagerLoadParameter(&mask, prm);
    if (ret != kEsfNetworkManagerResultSuccess) {
        fprintf(stderr, "EsfNetworkManagerLoadParameter failed (%u)", ret);
        ret = false;
        goto end;
    }

    if (prm->proxy.url[0] != '\0') {
        /* Save proxy host to cache */
        if (!(g_proxy_cache.host = strdup(prm->proxy.url))) {
            fprintf(stderr, "failed to allocate memory for proxy url");
            ret = -ENOMEM;
            goto end;
        }

        /* Save proxy port to cache */
        ret = snprintf(g_proxy_cache.port, sizeof(g_proxy_cache.port), "%d", prm->proxy.port);

        if (ret < 0 || ret >= sizeof(g_proxy_cache.port)) {
            fprintf(stderr, "snprintf(3) failed with %d", ret);
            goto end;
        }
    }

    if (prm->proxy.username[0] != '\0') {
        /* Save proxy username to cache */
        if (!(g_proxy_cache.username = strdup(prm->proxy.username))) {
            fprintf(stderr,
                    "failed to allocate memory for proxy "
                    "username");
            ret = -ENOMEM;
            goto end;
        }
    }

    if (prm->proxy.password[0] != '\0') {
        /* Save proxy password to cache */
        if (!(g_proxy_cache.password = strdup(prm->proxy.password))) {
            fprintf(stderr,
                    "failed to allocate memory for proxy "
                    "password");
            ret = -ENOMEM;
            goto end;
        }
    }

end:
    free(prm);

    if (ret) {
        evp_agent_esf_deinit_proxy_cache();
    }

    return ret;
}

static int get_cert_key_path(enum config_key key, char **cert_key_path)
{
    int ret = -ENOENT;
    DIR *dir = NULL;

    const char *suffix;
    if (key == EVP_CONFIG_MQTT_TLS_CLIENT_CERT) {
        suffix = "_cert.pem";
    }
    else {
        suffix = "_key.pem";
    }

    char *dir_path = getenv("EVP_CERT_KEY_DIR_PATH");
    if (!dir_path) {
        dir_path = MQTT_TLS_CLIENT_CERT_KEY_DEFAULT_DIR;
    }

    dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Failed to open %s directory, errno=%d\n", dir_path, errno);
        ret = -EIO;
        goto end;
    }

    for (struct dirent *dp = readdir(dir); dp; dp = readdir(dir)) {
        if (strstr(dp->d_name, suffix)) {
            if (asprintf(cert_key_path, "%s/%s", dir_path, dp->d_name) == -1) {
                fprintf(stderr, "asprintf failed\n");
                ret = -ENOMEM;
            }
            else {
                ret = 0;
            }

            break;
        }
    }

    if (ret == -ENOENT) {
        fprintf(stderr, "Failed to found %s/*%s\n", dir_path, suffix);
    }

end:
    closedir(dir);

    return ret;
}

static int read_cert_key(enum config_key key, char *buf, size_t buf_size)
{
    int ret = 0, fd = -1;
    char *cert_key_path = NULL;

    ret = get_cert_key_path(key, &cert_key_path);
    if (ret != 0) {
        fprintf(stderr, "get_cert_key_path() failed");
        goto end;
    }

    fd = open(cert_key_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "failed to open %s, errno=%d\n", cert_key_path, errno);
        ret = -EIO;
        goto end;
    }

    ssize_t nread = read(fd, buf, buf_size - 1);
    if (nread <= 0) {
        fprintf(stderr, "failed to read from %s, nread=%zd errno=%d", cert_key_path, nread, errno);
        ret = -EIO;
        goto end;
    }

    buf[buf_size - 1] = '\0';

end:
    if (fd >= 0) {
        close(fd);
    }

    free(cert_key_path);
    return ret;
}

struct config *evp_agent_esf_read_config(enum config_key key)
{
    EsfSystemManagerResult sys_mgr_ret;
    size_t max_size;
    char *buf = NULL;
    struct config *config = NULL;

    if (key >= __arraycount(g_max_sizes)) {
        fprintf(stderr, "No known size for key\n");
        return NULL;
    }

    max_size = g_max_sizes[key];
    buf = malloc(max_size + 1);
    if (!buf) {
        fprintf(stderr, "Failed to allocate memory buffer for config\n");
        return NULL;
    }

    bool is_success = true;
    switch (key) {
        case EVP_CONFIG_MQTT_PROXY_HOST:
        case EVP_CONFIG_HTTP_PROXY_HOST:
            if (g_proxy_cache.host != NULL) {
                strncpy(buf, g_proxy_cache.host, max_size);
                buf[max_size - 1] = '\0';
            }
            else {
                is_success = false;
            }
            break;
        case EVP_CONFIG_MQTT_PROXY_PORT:
        case EVP_CONFIG_HTTP_PROXY_PORT:
            if (g_proxy_cache.port != NULL) {
                strncpy(buf, g_proxy_cache.port, max_size);
                buf[max_size - 1] = '\0';
            }
            else {
                is_success = false;
            }
            break;
        case EVP_CONFIG_MQTT_PROXY_USERNAME:
        case EVP_CONFIG_HTTP_PROXY_USERNAME:
            if (g_proxy_cache.username != NULL) {
                strncpy(buf, g_proxy_cache.username, max_size);
                buf[max_size - 1] = '\0';
            }
            else {
                is_success = false;
            }
            break;
        case EVP_CONFIG_HTTP_PROXY_PASSWORD:
        case EVP_CONFIG_MQTT_PROXY_PASSWORD:
            if (g_proxy_cache.password != NULL) {
                strncpy(buf, g_proxy_cache.password, max_size);
                buf[max_size - 1] = '\0';
            }
            else {
                is_success = false;
            }
            break;
        case EVP_CONFIG_TLS_CA_CERT:
        case EVP_CONFIG_MQTT_TLS_CA_CERT:
        case EVP_CONFIG_HTTPS_CA_CERT:
            sys_mgr_ret = EsfSystemManagerGetRootCa(buf, &max_size);
            if (sys_mgr_ret != kEsfSystemManagerResultOk) {
                fprintf(stderr, "EsfSystemManagerGetRootCa() failed\n");
                is_success = false;
            }
            break;
        case EVP_CONFIG_MQTT_HOST:
            sys_mgr_ret = EsfSystemManagerGetEvpHubUrl(buf, &max_size);
            if (sys_mgr_ret != kEsfSystemManagerResultOk) {
                fprintf(stderr, "EsfSystemManagerGetEvpHubUrl() failed\n");
                is_success = false;
            }
            break;
        case EVP_CONFIG_MQTT_PORT:
            sys_mgr_ret = EsfSystemManagerGetEvpHubPort(buf, &max_size);
            if (sys_mgr_ret != kEsfSystemManagerResultOk) {
                fprintf(stderr, "EsfSystemManagerGetEvpHubPort() failed\n");
                is_success = false;
            }
            break;
        case EVP_CONFIG_IOT_PLATFORM:
            sys_mgr_ret = EsfSystemManagerGetEvpIotPlatform(buf, &max_size);
            if (sys_mgr_ret != kEsfSystemManagerResultOk) {
                fprintf(stderr, "EsfSystemManagerGetEvpIotPlatform() failed\n");
                is_success = false;
            }
            break;
        case EVP_CONFIG_MQTT_TLS_CLIENT_CERT:
        case EVP_CONFIG_MQTT_TLS_CLIENT_KEY:
            if (read_cert_key(key, buf, max_size + 1) != 0) {
                fprintf(stderr, "read_cert_key() failed\n");
                is_success = false;
            }
            break;
        default:
            is_success = false;
            break;
    }
    if (!is_success) {
        free(buf);
        return NULL;
    }

    config = malloc(sizeof(*config));
    if (!config) {
        fprintf(stderr, "Failed to allocate memory for config struct\n");
        free(buf);
        return NULL;
    }
    config->key = key;
    config->value = buf;
    config->size = strlen(buf) + 1;
    config->free = free;
    return config;
}
