/**
 * \file user_config.h
 *
 * \brief User configuration for mbedTLS
 */

#ifndef MBEDTLS_USER_CONFIG_H
#define MBEDTLS_USER_CONFIG_H

#undef MBEDTLS_SELF_TEST

#undef MBEDTLS_CMAC_C

#define MBEDTLS_THREADING_C       1
#define MBEDTLS_THREADING_PTHREAD 1

#undef MBEDTLS_SSL_PROTO_TLS1_3
#undef MBEDTLS_DEBUG_C

#endif /* MBEDTLS_USER_CONFIG_H */
