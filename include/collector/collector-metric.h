/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __COLLECTOR_METRIC_H
#define __COLLECTOR_METRIC_H

#include <margo.h>
#include <collector/collector-common.h>
#include <collector/collector-client.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct collector_metric_handle *collector_metric_handle_t;
typedef struct collector_metric *collector_metric_t;
typedef enum collector_metric_type collector_metric_type_t;
typedef struct collector_provider* collector_provider_t;
typedef struct collector_taglist* collector_taglist_t;
typedef struct collector_metric_sample* collector_metric_buffer;
typedef struct collector_metric_sample collector_metric_sample;
typedef void (*func)();
#define COLLECTOR_METRIC_HANDLE_NULL ((collector_metric_handle_t)NULL)

/* APIs for providers to record performance data */
collector_return_t collector_taglist_create(collector_taglist_t *taglist, int num_tags, ...);
collector_return_t collector_taglist_destroy(collector_taglist_t taglist);
collector_return_t collector_metric_create(const char *ns, const char *name, collector_metric_type_t t, const char *desc, collector_taglist_t taglist, collector_metric_t* metric_handle, collector_provider_t provider);
collector_return_t collector_metric_destroy(collector_metric_t m, collector_provider_t provider);
collector_return_t collector_metric_destroy_all(collector_provider_t provider);
collector_return_t collector_metric_update(collector_metric_t m, double val);
collector_return_t collector_metric_update_gauge_by_fixed_amount(collector_metric_t m, double diff);
collector_return_t collector_metric_dump_histogram(collector_metric_t m, const char *filename, size_t num_buckets);
collector_return_t collector_metric_dump_raw_data(collector_metric_t m, const char *filename);
collector_return_t collector_metric_class_register_retrieval_callback(char *ns, func f);

/* APIs for remote clients to request for performance data */
collector_return_t collector_remote_metric_get_id(char *ns, char *name, collector_taglist_t taglist, collector_metric_id_t* metric_id);
collector_return_t collector_remote_metric_handle_create(collector_client_t client, hg_addr_t addr, uint16_t provider_id, collector_metric_id_t metric_id, collector_metric_handle_t* handle);
collector_return_t collector_remote_metric_handle_ref_incr(collector_metric_handle_t handle);
collector_return_t collector_remote_metric_handle_release(collector_metric_handle_t handle);
collector_return_t collector_remote_metric_fetch(collector_metric_handle_t handle, int64_t *num_samples_requested, collector_metric_buffer *buf, char **name, char **ns);
collector_return_t collector_remote_list_metrics(collector_client_t client, hg_addr_t addr, uint16_t provider_id, collector_metric_id_t** ids, size_t* count);

#ifdef __cplusplus
}
#endif

#endif
