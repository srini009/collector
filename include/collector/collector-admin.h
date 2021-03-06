/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __COLLECTOR_ADMIN_H
#define __COLLECTOR_ADMIN_H

#include <margo.h>
#include <collector/collector-common.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct collector_admin* collector_admin_t;
#define COLLECTOR_ADMIN_NULL ((collector_admin_t)NULL)

#define COLLECTOR_METRIC_ID_IGNORE ((collector_metric_id_t*)NULL)

/**
 * @brief Creates a COLLECTOR admin.
 *
 * @param[in] mid Margo instance
 * @param[out] admin COLLECTOR admin
 *
 * @return COLLECTOR_SUCCESS or error code defined in collector-common.h
 */
collector_return_t collector_admin_init(margo_instance_id mid, collector_admin_t* admin);

/**
 * @brief Finalizes a COLLECTOR admin.
 *
 * @param[in] admin COLLECTOR admin to finalize
 *
 * @return COLLECTOR_SUCCESS or error code defined in collector-common.h
 */
collector_return_t collector_admin_finalize(collector_admin_t admin);

#endif
