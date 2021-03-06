/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __COLLECTOR_SERVER_H
#define __COLLECTOR_SERVER_H

#include <collector/collector-common.h>
#include <margo.h>
#include <abt-io.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COLLECTOR_ABT_POOL_DEFAULT ABT_POOL_NULL

typedef struct collector_provider* collector_provider_t;
#define COLLECTOR_PROVIDER_NULL ((collector_provider_t)NULL)
#define COLLECTOR_PROVIDER_IGNORE ((collector_provider_t*)NULL)

struct collector_provider_args {
    uint8_t            push_finalize_callback;
    const char*        token;  // Security token
    const char*        config; // JSON configuration
    ABT_pool           pool;   // Pool used to run RPCs
    abt_io_instance_id abtio;  // ABT-IO instance
    // ...
};

#define COLLECTOR_PROVIDER_ARGS_INIT { \
    .push_finalize_callback = 1,\
    .token = NULL, \
    .config = NULL, \
    .pool = ABT_POOL_NULL, \
    .abtio = ABT_IO_INSTANCE_NULL \
}

/**
 * @brief Creates a new COLLECTOR provider. If COLLECTOR_PROVIDER_IGNORE
 * is passed as last argument, the provider will be automatically
 * destroyed when calling margo_finalize.
 *
 * @param[in] mid Margo instance
 * @param[in] provider_id provider id
 * @param[in] args argument structure
 * @param[out] provider provider
 *
 * @return COLLECTOR_SUCCESS or error code defined in collector-common.h
 */
int collector_provider_register(
        margo_instance_id mid,
        uint16_t provider_id,
        const struct collector_provider_args* args,
        collector_provider_t* provider);

/**
 * @brief Destroys the Alpha provider and deregisters its RPC.
 *
 * @param[in] provider Alpha provider
 *
 * @return COLLECTOR_SUCCESS or error code defined in collector-common.h
 */
int collector_provider_destroy(
        collector_provider_t provider);

#ifdef __cplusplus
}
#endif

#endif
