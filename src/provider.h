/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __PROVIDER_H
#define __PROVIDER_H

#include <margo.h>
#include <abt-io.h>
#include "uthash.h"
#include "types.h"

typedef struct collector_provider {
    /* Margo/Argobots/Mercury environment */
    margo_instance_id  mid;                 // Margo instance
    uint16_t           provider_id;         // Provider id
    ABT_pool           pool;                // Pool on which to post RPC requests
    abt_io_instance_id abtio;               // ABT-IO instance
    /* Resources and backend types */
    size_t               num_metrics;     // number of metrics
    collector_metric*      metrics;         // hash of metrics by id
    /* RPC identifiers for clients */
    hg_id_t list_metrics_id;
    hg_id_t metric_fetch_id;
    /* ... add other RPC identifiers here ... */
    uint8_t use_aggregator;
#ifdef USE_AGGREGATOR
    aggregator_client_t aggcl;
    aggregator_provider_handle_t aggphs;
#endif
} collector_provider;

collector_return_t collector_provider_metric_create(const char *ns, const char *name, collector_metric_type_t t, const char *desc, collector_taglist_t tl, collector_metric_t* m, collector_provider_t provider);

collector_return_t collector_provider_metric_destroy(collector_metric_t m, collector_provider_t provider);

collector_return_t collector_provider_destroy_all_metrics(collector_provider_t provider);
#endif
