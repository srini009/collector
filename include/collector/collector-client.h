/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __COLLECTOR_CLIENT_H
#define __COLLECTOR_CLIENT_H

#include <margo.h>
#include <collector/collector-common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct collector_client* collector_client_t;
#define COLLECTOR_CLIENT_NULL ((collector_client_t)NULL)

/**
 * @brief Creates a COLLECTOR client.
 *
 * @param[in] mid Margo instance
 * @param[out] client COLLECTOR client
 *
 * @return COLLECTOR_SUCCESS or error code defined in collector-common.h
 */
collector_return_t collector_client_init(margo_instance_id mid, collector_client_t* client);

/**
 * @brief Finalizes a COLLECTOR client.
 *
 * @param[in] client COLLECTOR client to finalize
 *
 * @return COLLECTOR_SUCCESS or error code defined in collector-common.h
 */
collector_return_t collector_client_finalize(collector_client_t client);

#ifdef __cplusplus
}
#endif

#endif
