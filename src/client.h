/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef _CLIENT_H
#define _CLIENT_H

#include "types.h"
#include "collector/collector-client.h"
#include "collector/collector-metric.h"

typedef struct collector_client {
   margo_instance_id mid;
   hg_id_t           metric_fetch_id;
   hg_id_t           list_metrics_id;
   uint64_t          num_metric_handles;
} collector_client;

typedef struct collector_metric_handle {
    collector_client_t      client;
    hg_addr_t           addr;
    uint16_t            provider_id;
    uint64_t            refcount;
    collector_metric_id_t metric_id;
} collector_metric_handle;

#endif
